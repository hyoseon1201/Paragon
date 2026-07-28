// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Dekker/P1GameplayAbility_IonStrike.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystem/TargetActors/P1TargetActor_GroundDecal_Deferred.h"
#include "Characters/P1CharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayEffect.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

UP1GameplayAbility_IonStrike::UP1GameplayAbility_IonStrike()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_IonStrike);
	SetAssetTags(Tags);

	InputTag = TAG_InputTag_Ability_R;
}

void UP1GameplayAbility_IonStrike::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!TargetActorClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[IonStrike] TargetActorClass 미설정 — GA BP에서 지정하세요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// InstancedPerActor라 재활성화마다 같은 인스턴스가 재사용된다 — 지난 캐스트의 bCommitted가
	// 남아있으면 이번엔 조준만 하고 취소했는데도 EndAbility의 쿨다운 폴백이 잘못 걸릴 수 있다.
	bCommitted = false;

	BeginTargeting();
}

void UP1GameplayAbility_IonStrike::BeginTargeting()
{
	SetTargetingState(true);

	if (AimMontage)
	{
		AimMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AimMontage, 1.0f);
		AimMontageTask->ReadyForActivation();
	}

	UAbilityTask_WaitTargetData* Task = UAbilityTask_WaitTargetData::WaitTargetData(
		this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	if (!Task)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	Task->ValidData.AddDynamic(this, &UP1GameplayAbility_IonStrike::OnTargetDataReady);
	Task->Cancelled.AddDynamic(this, &UP1GameplayAbility_IonStrike::OnTargetCancelled);

	AGameplayAbilityTargetActor* SpawnedActor = nullptr;
	const bool bSpawned = Task->BeginSpawningActor(this, TargetActorClass, SpawnedActor);
	if (bSpawned)
	{
		if (AP1TargetActor_GroundDecal_Deferred* DeferredDecal = Cast<AP1TargetActor_GroundDecal_Deferred>(SpawnedActor))
		{
			DeferredDecal->Configure(MaxRange, EffectRadius);
		}
		Task->FinishSpawningActor(this, SpawnedActor);
	}

	Task->ReadyForActivation();
}

void UP1GameplayAbility_IonStrike::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
	SetTargetingState(false);
	StopAimMontage();

	ConfirmedLocation = UAbilitySystemBlueprintLibrary::GetTargetDataEndPoint(Data, 0);

	// 쿨다운은 지속시간이 끝나야 시작되므로(MakeWay와 동일 이유), 여기서는 코스트만 커밋한다.
	if (!CommitAbilityCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		UE_LOG(LogP1, Log, TEXT("[IonStrike] 코스트 커밋 실패 — 취소"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	CurrentTick = 0;
	bCooldownApplied = false;
	bCommitted = true;
	TotalTicks = FMath::Max(1, FMath::RoundToInt(Duration.GetValueAtLevel(GetAbilityLevel()) / TickPeriod));

	if (CurrentActorInfo->IsNetAuthority())
	{
		AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
		if (IsValid(SourceCharacter) && ArrowRainEffectTemplate)
		{
			SourceCharacter->MulticastSetPersistentParticleEffectAtLocation(ArrowRainEffectTemplate, ConfirmedLocation, ArrowRainEffectRotation, ArrowRainEffectScale);
		}

		GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this,
			&UP1GameplayAbility_IonStrike::OnRainTick, TickPeriod, true);

		UE_LOG(LogP1, Log, TEXT("[IonStrike] 발동 — %.1f초 간격 %d틱 @ %s"), TickPeriod, TotalTicks, *ConfirmedLocation.ToString());
	}

	if (FireMontage)
	{
		UAbilityTask_PlayMontageAndWait* FireTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, FireMontage, 1.0f);
		// 지속 판정(OnRainTick)과 별개로 재생만 하고 끝 — 어빌리티 종료는 타이머 소진 시점이 담당한다.
		FireTask->ReadyForActivation();
	}
}

void UP1GameplayAbility_IonStrike::OnRainTick()
{
	++CurrentTick;

	// 데미지/디버프 판정은 서버에서만 수행.
	if (CurrentActorInfo->IsNetAuthority())
	{
		AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
		if (IsValid(SourceCharacter))
		{
			const TArray<AActor*> Enemies = GetEnemiesInRadius(ConfirmedLocation, EffectRadius, HalfHeight);

			UE_LOG(LogP1, Log, TEXT("[IonStrike] Tick %d/%d — EnemyCount=%d"), CurrentTick, TotalTicks, Enemies.Num());

			for (AActor* Enemy : Enemies)
			{
				ApplyDamageToTarget(Enemy, 1.0f);

				if (MagicalArmorShredDebuffEffectClass)
				{
					if (IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(Enemy))
					{
						if (UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent())
						{
							if (UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo())
							{
								FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
								Ctx.AddSourceObject(SourceCharacter);
								const FGameplayEffectSpecHandle DebuffSpec = SourceASC->MakeOutgoingSpec(
									MagicalArmorShredDebuffEffectClass, GetAbilityLevel(), Ctx);
								if (DebuffSpec.IsValid())
								{
									DebuffSpec.Data->SetSetByCallerMagnitude(TAG_Data_DebuffMagnitude,
										MagicalArmorShredPerTick.GetValueAtLevel(GetAbilityLevel()));
									SourceASC->ApplyGameplayEffectSpecToTarget(*DebuffSpec.Data.Get(), TargetASC);
								}
							}
						}
					}
				}
			}

#if ENABLE_DRAW_DEBUG
			if (bShowDebug)
			{
				DrawDebugSphere(GetWorld(), ConfirmedLocation, EffectRadius, 24, FColor::Purple, false, TickPeriod * 0.9f, 0, 2.0f);
			}
#endif
		}
	}

	if (CurrentTick >= TotalTicks)
	{
		GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);

		if (!bCooldownApplied)
		{
			const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
			if (CooldownGE)
			{
				ApplyEffectToSelf(CooldownGE->GetClass());
			}
			bCooldownApplied = true;
		}

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UP1GameplayAbility_IonStrike::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& Data)
{
	UE_LOG(LogP1, Log, TEXT("[IonStrike] 조준 취소 — 무소모 종료"));
	SetTargetingState(false);
	StopAimMontage();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UP1GameplayAbility_IonStrike::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);
	}
	SetTargetingState(false);
	StopAimMontage();

	// 정상 종료(전 틱 소진)든 조기 취소/중단(스턴 등)이든 항상 이 경로를 거치므로, 화살비도 여기서
	// 일괄 정리한다 — bAutoDestroy=false로 스폰했으므로 이 호출 없이는 절대 안 멈춘다.
	if (ActorInfo->IsNetAuthority())
	{
		if (AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo())
		{
			SourceCharacter->MulticastStopPersistentParticleEffectAtLocation();
		}
	}

	// 지속시간을 다 못 채우고 중간에 취소/중단(스턴 등)됐어도 코스트는 이미 나갔으므로 쿨다운은
	// 걸어준다 — 단, 조준만 하고 취소한 경우(bCommitted=false, 코스트 자체가 안 나감)까지 쿨다운이
	// 걸리면 안 되므로 bCommitted로 구분한다(PhotonDisruptor에서 겪은 것과 동일한 버그 패턴).
	if (bCommitted && !bCooldownApplied)
	{
		const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
		if (CooldownGE)
		{
			ApplyEffectToSelf(CooldownGE->GetClass());
		}
		bCooldownApplied = true;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UP1GameplayAbility_IonStrike::SetTargetingState(bool bEnable)
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

void UP1GameplayAbility_IonStrike::StopAimMontage()
{
	if (AimMontageTask)
	{
		AimMontageTask->EndTask();
		AimMontageTask = nullptr;
	}

	// EndTask만으로는 루프 몽타주(에셋 자체 Loop 설정)가 확실히 멎는다는 보장이 없어 명시적으로도 정지시킨다.
	if (AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo())
	{
		if (USkeletalMeshComponent* Mesh = SourceCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				if (AimMontage && AnimInstance->Montage_IsPlaying(AimMontage))
				{
					AnimInstance->Montage_Stop(0.25f, AimMontage);
				}
			}
		}
	}
}
