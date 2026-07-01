// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/P1GameplayAbility_MeleeAttack.h"
#include "P1.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Characters/P1CharacterBase.h"
#include "GameFramework/PlayerController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"

UP1GameplayAbility_MeleeAttack::UP1GameplayAbility_MeleeAttack()
{
	FGameplayTagContainer NewAssetTags;
	NewAssetTags.AddTag(TAG_Ability_BasicAttack);
	SetAssetTags(NewAssetTags);

	ActivationBlockedTags.AddTag(TAG_State_Attacking);
	ActivationOwnedTags.AddTag(TAG_State_Attacking);
}

void UP1GameplayAbility_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogP1, Log, TEXT("[MeleeAttack] ActivateAbility called"));
	CurrentComboIndex = 0;
	PlayCurrentComboMontage();
}

void UP1GameplayAbility_MeleeAttack::InputPressed(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (bComboWindowOpen)
	{
		bNextComboQueued = true;
	}
}

void UP1GameplayAbility_MeleeAttack::PlayCurrentComboMontage()
{
	bComboWindowOpen = false;
	bNextComboQueued = false;
	bContinuingCombo = false;

	if (!ComboMontages.IsValidIndex(CurrentComboIndex) || !ComboMontages[CurrentComboIndex])
	{
		UE_LOG(LogP1, Warning, TEXT("[MeleeAttack] ComboMontages[%d] is null or missing — ComboMontages.Num()=%d. GA BP에서 ComboMontages 배열을 채워주세요."), CurrentComboIndex, ComboMontages.Num());
		EndAttack();
		return;
	}

	// 이전 콤보 스텝의 히트 이벤트 태스크가 남아 있으면 명시적으로 종료해 중복 감지를 방지한다.
	if (ActiveHitEventTask)
	{
		ActiveHitEventTask->EndTask();
		ActiveHitEventTask = nullptr;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, ComboMontages[CurrentComboIndex], GetComputedMontagePlayRate(ComboMontages[CurrentComboIndex]));
	MontageTask->OnBlendOut.AddDynamic(this, &UP1GameplayAbility_MeleeAttack::OnMontageBlendingOut);
	MontageTask->OnCompleted.AddDynamic(this, &UP1GameplayAbility_MeleeAttack::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UP1GameplayAbility_MeleeAttack::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UP1GameplayAbility_MeleeAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	// OnlyTriggerOnce=true: 콤보 스텝 하나당 히트 판정 1회만 허용.
	ActiveHitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, TAG_Event_Montage_BasicAttackHit, nullptr, true);
	ActiveHitEventTask->EventReceived.AddDynamic(this, &UP1GameplayAbility_MeleeAttack::OnHitEventReceived);
	ActiveHitEventTask->ReadyForActivation();
}

void UP1GameplayAbility_MeleeAttack::OnHitEventReceived(FGameplayEventData Payload)
{
	UE_LOG(LogP1, Log, TEXT("[MeleeAttack] OnHitEventReceived"));

	bComboWindowOpen = true;

	// Spec->InputPressed는 서버에서 InputReleased RPC가 아직 미도착인 경우 stale 상태일 수 있어
	// 여기서 체크하면 탭(단발) 공격이 서버에서 콤보로 오판된다.
	// hold-to-combo는 InputPressed()에서만 처리하고, 여기서는 체크하지 않는다.

	// 데미지 판정은 서버에서만 수행.
	if (!CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
	if (!IsValid(SourceCharacter))
	{
		return;
	}

	// AttackRange 어트리뷰트를 구 반지름으로 사용해 오버랩.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	bool bFoundRange = false;
	const float AttackRange = ASC->GetGameplayAttributeValue(UP1AttributeSet::GetAttackRangeAttribute(), bFoundRange);
	if (!bFoundRange || AttackRange <= 0.0f)
	{
		return;
	}

	const FVector SourceLocation = SourceCharacter->GetActorLocation();

	// ECC_Pawn 채널로 쿼리 후 Cast<AP1CharacterBase>로 걸러냄.
	// ObjectType 기반 쿼리는 콜리전 프리셋에 따라 캐릭터를 놓칠 수 있어 채널 기반이 더 안정적.
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(SourceCharacter);

	TArray<FOverlapResult> OverlapResults;
	GetWorld()->OverlapMultiByChannel(OverlapResults, SourceLocation, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(AttackRange), Params);

	// 캐릭터 정면 180° + 높이 범위 필터링 → 반원 판정.
	const FVector CharForward = SourceCharacter->GetActorForwardVector();
	TArray<AActor*> EnemyTargets;
	for (const FOverlapResult& Result : OverlapResults)
	{
		// P1CharacterBase 계열만 대상으로 인정 — 지형/소품은 캐스트 실패로 자동 제외.
		AP1CharacterBase* HitCharacter = Cast<AP1CharacterBase>(Result.GetActor());
		if (!IsValid(HitCharacter))
		{
			continue;
		}

		const bool bSameTeam = AP1CharacterBase::IsSameTeam(SourceCharacter, HitCharacter);
		UE_LOG(LogP1, Log, TEXT("[MeleeAttack] Overlap: %s | SameTeam=%d"), *HitCharacter->GetName(), bSameTeam ? 1 : 0);
		if (bSameTeam)
		{
			continue;
		}

		const FVector ToTarget = HitCharacter->GetActorLocation() - SourceLocation;

		// 정면 반원 — 수평 성분 기준 dot > 0인 대상만 포함.
		const FVector ToTargetH = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();
		const float ForwardDot = FVector::DotProduct(CharForward, ToTargetH);
		if (ForwardDot <= 0.0f)
		{
			UE_LOG(LogP1, Log, TEXT("[MeleeAttack] %s filtered by forward (dot=%.2f)"),
				*HitCharacter->GetName(), ForwardDot);
			continue;
		}

		// 높이 범위 필터.
		if (FMath::Abs(ToTarget.Z) > AttackHalfHeight)
		{
			UE_LOG(LogP1, Log, TEXT("[MeleeAttack] %s filtered by height (dZ=%.1f, limit=%.1f)"),
				*HitCharacter->GetName(), ToTarget.Z, AttackHalfHeight);
			continue;
		}

		UE_LOG(LogP1, Log, TEXT("[MeleeAttack] %s added to EnemyTargets"), *HitCharacter->GetName());
		EnemyTargets.Add(HitCharacter);
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebugAttack)
	{
		// 정면 반원(파이 조각) 형태로 그리기.
		const int32 NumSegments = 16;
		const FVector LeftEdge = CharForward.RotateAngleAxis(-90.0f, FVector::UpVector) * AttackRange;
		FVector PrevPoint = SourceLocation + LeftEdge;
		DrawDebugLine(GetWorld(), SourceLocation, PrevPoint, FColor::Orange, false, 1.0f, 0, 1.5f);
		for (int32 i = 1; i <= NumSegments; i++)
		{
			const float Angle = -90.0f + (180.0f / NumSegments) * i;
			const FVector NewPoint = SourceLocation + CharForward.RotateAngleAxis(Angle, FVector::UpVector) * AttackRange;
			DrawDebugLine(GetWorld(), PrevPoint, NewPoint, FColor::Orange, false, 1.0f, 0, 1.5f);
			PrevPoint = NewPoint;
		}
		DrawDebugLine(GetWorld(), SourceLocation, PrevPoint, FColor::Orange, false, 1.0f, 0, 1.5f);
	}
#endif

	if (EnemyTargets.IsEmpty())
	{
		return;
	}

	// 카메라 forward 기준으로 주목표 선정 — 화면 중앙에 가장 가까운 적이 주목표.
	FVector ViewLocation;
	FRotator ViewRotation;
	if (const APlayerController* PC = Cast<APlayerController>(SourceCharacter->GetController()))
	{
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		ViewLocation = SourceLocation;
		ViewRotation = SourceCharacter->GetActorRotation();
	}

	const FVector CameraForward = ViewRotation.Vector();

	AActor* PrimaryTarget = nullptr;
	float MaxDot = -2.0f;
	for (AActor* Enemy : EnemyTargets)
	{
		const FVector ToEnemy = (Enemy->GetActorLocation() - SourceLocation).GetSafeNormal();
		const float Dot = FVector::DotProduct(CameraForward, ToEnemy);
		if (Dot > MaxDot)
		{
			MaxDot = Dot;
			PrimaryTarget = Enemy;
		}
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebugAttack)
	{
		for (AActor* Enemy : EnemyTargets)
		{
			const FColor TargetColor = (Enemy == PrimaryTarget) ? FColor::Red : FColor::Yellow;
			DrawDebugSphere(GetWorld(), Enemy->GetActorLocation(), 40.0f, 12, TargetColor, false, 1.0f, 0, 2.0f);
		}
	}
#endif

	// Cleave 어트리뷰트 — 0이면 주목표 이외 데미지 없음.
	bool bFoundCleave = false;
	// Cleave는 0.0~1.0 범위의 소수값 (예: 0.2 = 20%). /100 불필요.
	const float CleavePct = ASC->GetGameplayAttributeValue(UP1AttributeSet::GetCleaveAttribute(), bFoundCleave);

	UE_LOG(LogP1, Log, TEXT("[MeleeAttack] PrimaryTarget=%s | EnemyCount=%d | CleavePct=%.2f"),
		PrimaryTarget ? *PrimaryTarget->GetName() : TEXT("null"), EnemyTargets.Num(), CleavePct);

	for (AActor* Enemy : EnemyTargets)
	{
		const float Multiplier = (Enemy == PrimaryTarget) ? 1.0f : CleavePct;
		UE_LOG(LogP1, Log, TEXT("[MeleeAttack] Damage loop: %s | IsPrimary=%d | Multiplier=%.2f"),
			*Enemy->GetName(), (Enemy == PrimaryTarget) ? 1 : 0, Multiplier);
		if (Multiplier > 0.0f)
		{
			ApplyDamageToTarget(Enemy, Multiplier);
		}
	}
}

void UP1GameplayAbility_MeleeAttack::OnMontageBlendingOut()
{
	// 블렌드아웃 시작 시점에 다음 콤보를 즉시 실행 — 이전 몽타주 잔여 프레임과 겹쳐져 틈이 없어진다.
	if (bNextComboQueued)
	{
		bContinuingCombo = true;
		CurrentComboIndex = (CurrentComboIndex + 1) % ComboMontages.Num();
		PlayCurrentComboMontage();
	}
}

void UP1GameplayAbility_MeleeAttack::OnMontageCompleted()
{
	// OnBlendOut에서 이미 다음 콤보를 시작했으면 여기서는 아무것도 하지 않는다.
	if (!bContinuingCombo)
	{
		EndAttack();
	}
}

void UP1GameplayAbility_MeleeAttack::OnMontageCancelled()
{
	EndAttack();
}

void UP1GameplayAbility_MeleeAttack::EndAttack()
{
	CurrentComboIndex = 0;

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

float UP1GameplayAbility_MeleeAttack::GetComputedMontagePlayRate(const UAnimMontage* Montage) const
{
	// PlayRate = (몽타주 실제 길이 / BasicAttackTime) × (AttackSpeed / 100)
	// → 몽타주 길이와 무관하게 항상 BasicAttackTime 초 안에 재생 완료.
	if (!IsValid(Montage))
	{
		return 1.0f;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return 1.0f;
	}

	bool bFoundAttackSpeed = false;
	bool bFoundBasicAttackTime = false;
	const float AttackSpeed = ASC->GetGameplayAttributeValue(UP1AttributeSet::GetAttackSpeedAttribute(), bFoundAttackSpeed);
	const float BasicAttackTime = ASC->GetGameplayAttributeValue(UP1AttributeSet::GetBasicAttackTimeAttribute(), bFoundBasicAttackTime);

	if (!bFoundAttackSpeed || !bFoundBasicAttackTime || BasicAttackTime <= 0.0f || AttackSpeed <= 0.0f)
	{
		return 1.0f;
	}

	return (Montage->GetPlayLength() / BasicAttackTime) * (AttackSpeed / 100.0f);
}
