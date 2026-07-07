// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/P1GameplayAbility_AssaultTheGates.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystem/P1AnimNotify_SendGameplayEvent.h"
#include "AbilitySystem/TargetActors/P1TargetActor_GroundDecal.h"
#include "Characters/P1CharacterBase.h"
#include "Characters/P1HeroCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "MotionWarpingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

UP1GameplayAbility_AssaultTheGates::UP1GameplayAbility_AssaultTheGates()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_AssaultTheGates);
	SetAssetTags(Tags);

	InputTag = TAG_InputTag_Ability_RMB;

	// State.Attacking 소유/차단은 베이스 UP1GameplayAbility에서 전 어빌리티 공통으로 처리한다
	// (조준~도약~착지 전체 구간 동안 다른 어빌리티 발동을 막아, 인터럽트로 인한 잔여속도 버그를 방지).
}

void UP1GameplayAbility_AssaultTheGates::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// CanActivate(코스트/쿨다운)는 TryActivate에서 이미 통과. 여기서는 커밋하지 않고 조준 단계로 진입.
	if (!TargetActorClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[AssaultTheGates] TargetActorClass 미설정 — GA BP에서 지정하세요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogP1, Log, TEXT("[AssaultTheGates] ActivateAbility — 조준 시작"));
	BeginTargeting();
}

void UP1GameplayAbility_AssaultTheGates::BeginTargeting()
{
	SetTargetingState(true);

	UAbilityTask_WaitTargetData* Task = UAbilityTask_WaitTargetData::WaitTargetData(
		this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	if (!Task)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	Task->ValidData.AddDynamic(this, &UP1GameplayAbility_AssaultTheGates::OnTargetDataReady);
	Task->Cancelled.AddDynamic(this, &UP1GameplayAbility_AssaultTheGates::OnTargetCancelled);

	// 스폰된 타겟액터에 사거리/반경 주입 (BeginSpawningActor는 소유 클라에서만 true).
	AGameplayAbilityTargetActor* SpawnedActor = nullptr;
	const bool bSpawned = Task->BeginSpawningActor(this, TargetActorClass, SpawnedActor);
	if (bSpawned)
	{
		if (AP1TargetActor_GroundDecal* Decal = Cast<AP1TargetActor_GroundDecal>(SpawnedActor))
		{
			Decal->Configure(MaxRange, AOERadius);
		}
		Task->FinishSpawningActor(this, SpawnedActor);
	}
	UE_LOG(LogP1, Warning, TEXT("[AssaultTheGates][Targeting] BeginTargeting — Auth=%d Local=%d | BeginSpawningActor=%d SpawnedActor=%s | TargetingTag=%d"),
		CurrentActorInfo->IsNetAuthority() ? 1 : 0, CurrentActorInfo->IsLocallyControlled() ? 1 : 0,
		bSpawned ? 1 : 0, SpawnedActor ? *SpawnedActor->GetName() : TEXT("NULL"), bTargetingActive ? 1 : 0);

	Task->ReadyForActivation();
}

void UP1GameplayAbility_AssaultTheGates::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
	SetTargetingState(false);

	ConfirmedLocation = UAbilitySystemBlueprintLibrary::GetTargetDataEndPoint(Data, 0);

	// 확정 시 코스트 소모. 감당 못하면 무취소 종료.
	if (!CommitAbilityCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 쿨다운은 서버 권위로 적용(복제). 기본 지속시간으로 시작하고, 영웅/보스 적중 시 감소.
	if (CurrentActorInfo->IsNetAuthority())
	{
		ApplyCooldownWithDuration(BaseCooldown.GetValueAtLevel(GetAbilityLevel()));
	}

	UE_LOG(LogP1, Log, TEXT("[AssaultTheGates] 확정 → 도약 @ %s"), *ConfirmedLocation.ToString());
	ExecuteLeap(ConfirmedLocation);
}

void UP1GameplayAbility_AssaultTheGates::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& Data)
{
	UE_LOG(LogP1, Log, TEXT("[AssaultTheGates] 조준 취소 — 무소모 종료"));
	SetTargetingState(false);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UP1GameplayAbility_AssaultTheGates::ExecuteLeap(const FVector& TargetLocation)
{
	AP1CharacterBase* Character = GetP1CharacterFromActorInfo();
	if (!IsValid(Character))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const FVector StartLoc = Character->GetActorLocation();
	const int32 MoveMode = Character->GetCharacterMovement() ? (int32)Character->GetCharacterMovement()->MovementMode : -1;
	UE_LOG(LogP1, Warning, TEXT("[AssaultTheGates][Leap] Start=%s Target=%s Dist=%.0f | Auth=%d Local=%d MovementMode=%d | LeapMontage=%s"),
		*StartLoc.ToString(), *TargetLocation.ToString(), FVector::Dist(StartLoc, TargetLocation),
		CurrentActorInfo->IsNetAuthority() ? 1 : 0, CurrentActorInfo->IsLocallyControlled() ? 1 : 0,
		MoveMode, LeapMontage ? *LeapMontage->GetName() : TEXT("NULL"));

	// 목표를 향해 포물선 도약. 애니메이션 루트모션에 수직 성분이 없어 MotionWarping으론 아크가
	// 안 나오므로, 코드로 아크를 만드는 ApplyRootMotionJumpForce를 사용한다.
	FVector ToTarget = TargetLocation - StartLoc;
	FVector ToTargetH(ToTarget.X, ToTarget.Y, 0.0f);
	const float HorizontalDist = ToTargetH.Size();
	const FRotator JumpRot = ToTargetH.IsNearlyZero() ? Character->GetActorRotation() : ToTargetH.Rotation();

	// 시전 방향으로 몸을 고정 회전.
	Character->SetActorRotation(FRotator(0.0f, JumpRot.Yaw, 0.0f));

	// 도약 시간은 몽타주 "전체 길이"가 아니라 Land 노티파이가 실제로 찍힌 시점에 맞춘다.
	// (Land 노티파이는 보통 스윙 임팩트 직후 몽타주 중간쯤에 있고, 그 뒤에 후속 동작이 더 있을 수 있음.
	//  전체 길이에 맞추면 캐릭터가 아직 도약 초반인데 임팩트가 터져버리는 버그가 생긴다.)
	float ActualLeapDuration = LeapMontage ? LeapMontage->GetPlayLength() : LeapDuration;
	if (LeapMontage)
	{
		for (const FAnimNotifyEvent& NotifyEvent : LeapMontage->Notifies)
		{
			const UP1AnimNotify_SendGameplayEvent* SendEventNotify = Cast<UP1AnimNotify_SendGameplayEvent>(NotifyEvent.Notify);
			if (SendEventNotify && SendEventNotify->EventTag == TAG_Event_Montage_AssaultTheGates_Land)
			{
				ActualLeapDuration = NotifyEvent.GetTriggerTime();
				break;
			}
		}
	}
	ActualLeapDuration = FMath::Max(ActualLeapDuration, 0.1f);

	UAbilityTask_ApplyRootMotionJumpForce* JumpTask = UAbilityTask_ApplyRootMotionJumpForce::ApplyRootMotionJumpForce(
		this, NAME_None, JumpRot, HorizontalDist, LeapHeight, ActualLeapDuration,
		/*MinimumLandedTriggerTime*/ 0.0f, /*bFinishOnLanded*/ false,
		ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity, FVector::ZeroVector, 0.0f,
		/*PathOffsetCurve*/ nullptr, /*TimeMappingCurve*/ nullptr);
	JumpTask->ReadyForActivation();

	UE_LOG(LogP1, Warning, TEXT("[AssaultTheGates][Leap] JumpForce 적용: Dist=%.0f Height=%.0f Dur=%.2f(LandNotifyTime, MontageLength=%.2f) Yaw=%.0f"),
		HorizontalDist, LeapHeight, ActualLeapDuration, LeapMontage ? LeapMontage->GetPlayLength() : -1.0f, JumpRot.Yaw);

	if (!LeapMontage)
	{
		UE_LOG(LogP1, Warning, TEXT("[AssaultTheGates] LeapMontage 미설정 — 즉시 착지 판정만 수행"));
		PerformLandDamage(TargetLocation);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, LeapMontage, 1.0f);
	MontageTask->OnBlendOut.AddDynamic(this, &UP1GameplayAbility_AssaultTheGates::OnLeapMontageBlendOut);
	MontageTask->OnCompleted.AddDynamic(this, &UP1GameplayAbility_AssaultTheGates::OnLeapMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UP1GameplayAbility_AssaultTheGates::OnLeapMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UP1GameplayAbility_AssaultTheGates::OnLeapMontageCancelled);
	MontageTask->ReadyForActivation();

	LogMontageEvent(TEXT("PlayMontageAndWait ReadyForActivation"));

	// 착지 프레임 이벤트 대기 (몽타주당 1회).
	LandEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, TAG_Event_Montage_AssaultTheGates_Land, nullptr, true);
	LandEventTask->EventReceived.AddDynamic(this, &UP1GameplayAbility_AssaultTheGates::OnLandEventReceived);
	LandEventTask->ReadyForActivation();

	// 디버그: 이 몽타주의 슬롯 세그먼트(애니메이션) 구성을 한 번 나열하고, 재생 중 어느 세그먼트를
	// 지나고 있는지 주기적으로 로그해 각 애니메이션이 실제로 재생되는지 확인한다.
	for (const FSlotAnimationTrack& SlotTrack : LeapMontage->SlotAnimTracks)
	{
		for (const FAnimSegment& Segment : SlotTrack.AnimTrack.AnimSegments)
		{
			UE_LOG(LogP1, Warning, TEXT("[AssaultTheGates][Segment] 슬롯 '%s' 세그먼트: %s (%.3f~%.3f)"),
				*SlotTrack.SlotName.ToString(),
				Segment.GetAnimReference() ? *Segment.GetAnimReference()->GetName() : TEXT("NULL"),
				Segment.StartPos, Segment.GetEndPos());
		}
	}
	LastLoggedSegmentName = NAME_None;
	GetWorld()->GetTimerManager().SetTimer(MontageSegmentTimerHandle, this,
		&UP1GameplayAbility_AssaultTheGates::LogCurrentMontageSegment, 0.05f, true);
}

void UP1GameplayAbility_AssaultTheGates::LogCurrentMontageSegment()
{
	const ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : nullptr;
	UAnimInstance* AnimInst = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInst || !LeapMontage || !AnimInst->Montage_IsPlaying(LeapMontage))
	{
		return;
	}

	const float Pos = AnimInst->Montage_GetPosition(LeapMontage);
	FName CurrentSegmentName = NAME_None;
	float SegStart = 0.f;
	float SegEnd = 0.f;

	for (const FSlotAnimationTrack& SlotTrack : LeapMontage->SlotAnimTracks)
	{
		for (const FAnimSegment& Segment : SlotTrack.AnimTrack.AnimSegments)
		{
			if (Segment.IsInRange(Pos))
			{
				CurrentSegmentName = Segment.GetAnimReference() ? Segment.GetAnimReference()->GetFName() : NAME_None;
				SegStart = Segment.StartPos;
				SegEnd = Segment.GetEndPos();
				break;
			}
		}
		if (CurrentSegmentName != NAME_None)
		{
			break;
		}
	}

	// 세그먼트가 바뀐 시점에만 로그 (스팸 방지) — 각 애니메이션 진입 여부를 명확히 보여준다.
	if (CurrentSegmentName != LastLoggedSegmentName)
	{
		UE_LOG(LogP1, Warning, TEXT("[AssaultTheGates][Segment] Position=%.3f → 재생 진입: %s (%.3f~%.3f) | Auth=%d Local=%d"),
			Pos, *CurrentSegmentName.ToString(), SegStart, SegEnd,
			CurrentActorInfo->IsNetAuthority() ? 1 : 0, CurrentActorInfo->IsLocallyControlled() ? 1 : 0);
		LastLoggedSegmentName = CurrentSegmentName;
	}
}

void UP1GameplayAbility_AssaultTheGates::OnLandEventReceived(FGameplayEventData Payload)
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	const FVector AvatarLoc = Avatar ? Avatar->GetActorLocation() : FVector::ZeroVector;
	// 착지 시점 실제 캐릭터 위치 vs 의도한 착지 위치 — 어긋나면 워프가 목표에 못 닿은 것.
	UE_LOG(LogP1, Warning, TEXT("[AssaultTheGates][Land] AvatarLoc=%s | ConfirmedLoc=%s | 오차=%.0f"),
		*AvatarLoc.ToString(), *ConfirmedLocation.ToString(), FVector::Dist(AvatarLoc, ConfirmedLocation));
	PerformLandDamage(ConfirmedLocation);
}

void UP1GameplayAbility_AssaultTheGates::PerformLandDamage(const FVector& Center)
{
	// 데미지/보상은 서버에서만.
	if (!CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AP1CharacterBase* Source = GetP1CharacterFromActorInfo();
	if (!IsValid(Source))
	{
		return;
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Source);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(AOERadius), Params);

	bool bHitHeroOrBoss = false;
	for (const FOverlapResult& Result : Overlaps)
	{
		AP1CharacterBase* Hit = Cast<AP1CharacterBase>(Result.GetActor());
		if (!IsValid(Hit))
		{
			continue;
		}
		if (AP1CharacterBase::IsSameTeam(Source, Hit))
		{
			continue;
		}
		if (FMath::Abs(Hit->GetActorLocation().Z - Center.Z) > AOEHalfHeight)
		{
			continue;
		}

		// 범위 내 모든 적에게 전체 데미지 (클리브 없음).
		ApplyDamageToTarget(Hit, 1.0f);

		if (Hit->IsHeroOrBoss())
		{
			bHitHeroOrBoss = true;
		}
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		DrawDebugSphere(GetWorld(), Center, AOERadius, 24, FColor::Red, false, 2.0f, 0, 2.0f);
	}
#endif

	if (!bHitHeroOrBoss)
	{
		return;
	}

	UE_LOG(LogP1, Log, TEXT("[AssaultTheGates] 영웅/보스 적중 — 이속버프 + 쿨다운 %d%% 감소"), FMath::RoundToInt(CooldownReductionOnHeroHit * 100.0f));

	// 이동속도 버프 (자신).
	if (MoveSpeedBuffEffectClass)
	{
		ApplyEffectToSelf(MoveSpeedBuffEffectClass);
	}

	// 쿨다운 감소.
	ReduceCooldown(CooldownReductionOnHeroHit);
}

void UP1GameplayAbility_AssaultTheGates::LogMontageEvent(const TCHAR* EventName) const
{
	float Position = -1.0f;
	float Length = -1.0f;
	if (const ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (const USkeletalMeshComponent* Mesh = Char->GetMesh())
		{
			if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
			{
				if (LeapMontage && AnimInst->Montage_IsPlaying(LeapMontage))
				{
					Position = AnimInst->Montage_GetPosition(LeapMontage);
				}
				Length = LeapMontage ? LeapMontage->GetPlayLength() : -1.0f;
			}
		}
	}

	UE_LOG(LogP1, Warning, TEXT("[AssaultTheGates][Montage] %s | Auth=%d Local=%d | Position=%.3f / Length=%.3f | Time=%.3f"),
		EventName, CurrentActorInfo->IsNetAuthority() ? 1 : 0, CurrentActorInfo->IsLocallyControlled() ? 1 : 0,
		Position, Length, GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f);
}

void UP1GameplayAbility_AssaultTheGates::OnLeapMontageBlendOut()
{
	LogMontageEvent(TEXT("OnBlendOut"));
	// BlendOut은 재생 종료를 의미하지 않을 수 있으므로 EndAbility는 호출하지 않는다 (관찰용).
}

void UP1GameplayAbility_AssaultTheGates::OnLeapMontageCompleted()
{
	LogMontageEvent(TEXT("OnCompleted"));
	HandleMontageEnd();
}

void UP1GameplayAbility_AssaultTheGates::OnLeapMontageInterrupted()
{
	LogMontageEvent(TEXT("OnInterrupted"));
	HandleMontageEnd();
}

void UP1GameplayAbility_AssaultTheGates::OnLeapMontageCancelled()
{
	LogMontageEvent(TEXT("OnCancelled"));
	HandleMontageEnd();
}

void UP1GameplayAbility_AssaultTheGates::HandleMontageEnd()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UP1GameplayAbility_AssaultTheGates::ApplyCooldownWithDuration(float Duration)
{
	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		UE_LOG(LogP1, Warning, TEXT("[AssaultTheGates] CooldownGameplayEffectClass 미설정 — 쿨다운 없음"));
		return;
	}

	ApplyEffectToSelf(CooldownGE->GetClass(), TAG_Data_CooldownDuration, Duration);
}

void UP1GameplayAbility_AssaultTheGates::ReduceCooldown(float Percent)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FGameplayTagContainer CooldownTags;
	CooldownTags.AddTag(TAG_Cooldown_Ability_AssaultTheGates);

	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);
	const TArray<float> Remaining = ASC->GetActiveEffectsTimeRemaining(Query);

	float MaxRemaining = 0.0f;
	for (const float R : Remaining)
	{
		MaxRemaining = FMath::Max(MaxRemaining, R);
	}
	if (MaxRemaining <= 0.0f)
	{
		return;
	}

	// 남은 시간을 (1-Percent)로 재적용해 감소.
	ASC->RemoveActiveEffectsWithGrantedTags(CooldownTags);
	ApplyCooldownWithDuration(MaxRemaining * (1.0f - Percent));
}

void UP1GameplayAbility_AssaultTheGates::SetTargetingState(bool bEnable)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	if (bEnable && !bTargetingActive)
	{
		ASC->AddLooseGameplayTag(TAG_State_TargetingAbility);
		bTargetingActive = true;
	}
	else if (!bEnable && bTargetingActive)
	{
		ASC->RemoveLooseGameplayTag(TAG_State_TargetingAbility);
		bTargetingActive = false;
	}
}

void UP1GameplayAbility_AssaultTheGates::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogP1, Warning, TEXT("[AssaultTheGates][End] Cancelled=%d Auth=%d Local=%d bTargetingActive=%d"),
		bWasCancelled ? 1 : 0, ActorInfo ? (ActorInfo->IsNetAuthority() ? 1 : 0) : -1,
		ActorInfo ? (ActorInfo->IsLocallyControlled() ? 1 : 0) : -1, bTargetingActive ? 1 : 0);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(MontageSegmentTimerHandle);
	}

	SetTargetingState(false);

	if (LandEventTask)
	{
		LandEventTask->EndTask();
		LandEventTask = nullptr;
	}

	if (const AP1HeroCharacter* Hero = Cast<AP1HeroCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (Hero->MotionWarpingComponent)
		{
			Hero->MotionWarpingComponent->RemoveWarpTarget(WarpTargetName);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
