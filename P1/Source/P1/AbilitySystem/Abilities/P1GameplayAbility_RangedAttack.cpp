// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/P1GameplayAbility_RangedAttack.h"
#include "P1.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystem/Projectiles/P1Projectile.h"
#include "Characters/P1CharacterBase.h"
#include "GameFramework/PlayerController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

FGameplayTag UP1GameplayAbility_RangedAttack::GetUICooldownTag() const
{
	return TAG_Cooldown_Ability_BasicAttack;
}

UP1GameplayAbility_RangedAttack::UP1GameplayAbility_RangedAttack()
{
	FGameplayTagContainer NewAssetTags;
	NewAssetTags.AddTag(TAG_Ability_BasicAttack);
	SetAssetTags(NewAssetTags);

	// State.Attacking 소유/차단은 베이스 UP1GameplayAbility에서 전 어빌리티 공통으로 처리한다.
	// 이 어빌리티를 쓰는 영웅(Dekker 등)은 MeleeAttack 대신 이 클래스를 자신의 유일한 BasicAttack으로
	// 등록한다 — 같은 InputTag.Ability.BasicAttack에 두 어빌리티가 경쟁하는 구조를 만들지 않는다
	// (Sacred Oath 변종과 동일한 설계 원칙, 상세 배경은 CLAUDE.md 참고).
}

void UP1GameplayAbility_RangedAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogP1, Log, TEXT("[RangedAttack] ActivateAbility called"));
	// 최초 활성화는 TryActivateAbility 경로 → InputPressed() 콜백이 없다. 활성화 = 버튼을 눌러
	// 트리거된 것이므로 held 상태로 시작해야 연사가 동작한다(MeleeAttack과 동일한 이유).
	bInputHeld = true;
	PlayAttackMontage();
}

void UP1GameplayAbility_RangedAttack::InputPressed(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
	bInputHeld = true;
}

void UP1GameplayAbility_RangedAttack::InputReleased(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	bInputHeld = false;
}

void UP1GameplayAbility_RangedAttack::PlayAttackMontage()
{
	bContinuingFire = false;

	if (!AttackMontage)
	{
		UE_LOG(LogP1, Warning, TEXT("[RangedAttack] AttackMontage가 null입니다 — GA BP에서 Attack Montage를 설정해주세요."));
		EndAttack();
		return;
	}

	// 이전 발사분의 히트 이벤트 태스크가 남아 있으면 명시적으로 종료해 중복 감지를 방지한다.
	if (ActiveFireEventTask)
	{
		ActiveFireEventTask->EndTask();
		ActiveFireEventTask = nullptr;
	}

	const float PlayRate = GetComputedMontagePlayRate();

	// 스킬 아이콘 UI 전용 "다음 공격까지 남은 시간" 타이머 — MeleeAttack과 동일한 패턴.
	if (CurrentActorInfo && CurrentActorInfo->IsNetAuthority() && AttackTimerEffectClass && PlayRate > 0.0f)
	{
		const float SwingDuration = AttackMontage->GetPlayLength() / PlayRate;
		ApplyEffectToSelf(AttackTimerEffectClass, TAG_Data_CooldownDuration, SwingDuration);
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage, PlayRate);
	MontageTask->OnBlendOut.AddDynamic(this, &UP1GameplayAbility_RangedAttack::OnMontageBlendingOut);
	MontageTask->OnCompleted.AddDynamic(this, &UP1GameplayAbility_RangedAttack::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UP1GameplayAbility_RangedAttack::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UP1GameplayAbility_RangedAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	// OnlyTriggerOnce=true: 발사 1회당 투사체 스폰 1회만 허용.
	ActiveFireEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, TAG_Event_Montage_BasicAttackHit, nullptr, true);
	ActiveFireEventTask->EventReceived.AddDynamic(this, &UP1GameplayAbility_RangedAttack::OnFireEventReceived);
	ActiveFireEventTask->ReadyForActivation();
}

void UP1GameplayAbility_RangedAttack::OnFireEventReceived(FGameplayEventData Payload)
{
	// 투사체 스폰은 서버에서만 — 클라 예측 인스턴스에서도 AnimNotify가 발화되므로 체크 없이는
	// 투사체가 두 번(로컬 예측 + 서버 authoritative) 스폰된다.
	if (!CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	if (!ProjectileClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[RangedAttack] ProjectileClass가 설정되지 않았습니다 — GA BP에서 지정해주세요."));
		return;
	}

	AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
	if (!IsValid(SourceCharacter))
	{
		return;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	bool bFoundRange = false;
	const float AttackRange = ASC->GetGameplayAttributeValue(UP1AttributeSet::GetAttackRangeAttribute(), bFoundRange);
	const float MaxRange = bFoundRange ? AttackRange : 0.0f;

	FVector SpawnLocation = SourceCharacter->GetActorLocation();
	if (!MuzzleSocketName.IsNone())
	{
		if (const USkeletalMeshComponent* Mesh = SourceCharacter->GetMesh())
		{
			if (Mesh->DoesSocketExist(MuzzleSocketName))
			{
				SpawnLocation = Mesh->GetSocketLocation(MuzzleSocketName);
			}
		}
	}

	const FVector AimDirection = GetAimDirection();
	const FRotator SpawnRotation = AimDirection.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceCharacter;
	SpawnParams.Instigator = SourceCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AP1Projectile* Projectile = GetWorld()->SpawnActor<AP1Projectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!Projectile)
	{
		UE_LOG(LogP1, Warning, TEXT("[RangedAttack] Projectile 스폰 실패"));
		return;
	}

	Projectile->InitializeProjectile(ProjectileSpeed, MaxRange, ProjectileRadius);
	Projectile->OnProjectileHit.AddDynamic(this, &UP1GameplayAbility_RangedAttack::OnProjectileHit);

	UE_LOG(LogP1, Log, TEXT("[RangedAttack] Projectile 스폰: %s (사거리=%.0f)"), *Projectile->GetName(), MaxRange);
}

void UP1GameplayAbility_RangedAttack::OnProjectileHit(AActor* HitActor, const FHitResult& HitResult)
{
	if (!CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
	AP1CharacterBase* TargetCharacter = Cast<AP1CharacterBase>(HitActor);
	if (!IsValid(SourceCharacter) || !IsValid(TargetCharacter))
	{
		return;
	}

	if (AP1CharacterBase::IsSameTeam(SourceCharacter, TargetCharacter))
	{
		return;
	}

	UE_LOG(LogP1, Log, TEXT("[RangedAttack] Projectile 명중: %s"), *TargetCharacter->GetName());
	ApplyDamageToTarget(TargetCharacter, 1.0f);
}

void UP1GameplayAbility_RangedAttack::OnMontageBlendingOut()
{
	// 블렌드아웃 시작 시점에 다음 발사를 즉시 실행 — 이전 몽타주 잔여 프레임과 겹쳐져 틈이 없어진다.
	if (bInputHeld)
	{
		bContinuingFire = true;
		PlayAttackMontage();
	}
}

void UP1GameplayAbility_RangedAttack::OnMontageCompleted()
{
	// OnBlendOut에서 이미 다음 발사를 시작했으면 여기서는 아무것도 하지 않는다.
	if (!bContinuingFire)
	{
		EndAttack();
	}
}

void UP1GameplayAbility_RangedAttack::OnMontageCancelled()
{
	EndAttack();
}

void UP1GameplayAbility_RangedAttack::EndAttack()
{
	bInputHeld = false;

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

FVector UP1GameplayAbility_RangedAttack::GetAimDirection() const
{
	const AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
	if (!IsValid(SourceCharacter))
	{
		return FVector::ForwardVector;
	}

	// 논타겟 스킬샷 컨벤션 — 캐릭터 정면이 아니라 카메라(조준) 방향 기준으로 발사한다.
	if (const APlayerController* PC = Cast<APlayerController>(SourceCharacter->GetController()))
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
		return ViewRotation.Vector();
	}

	return SourceCharacter->GetActorForwardVector();
}

float UP1GameplayAbility_RangedAttack::GetComputedMontagePlayRate() const
{
	// PlayRate = (몽타주 실제 길이 / BasicAttackTime) × (AttackSpeed / 100) — MeleeAttack과 동일한 공식.
	if (!AttackMontage)
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

	return (AttackMontage->GetPlayLength() / BasicAttackTime) * (AttackSpeed / 100.0f);
}
