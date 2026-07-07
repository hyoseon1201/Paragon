// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/P1GameplayAbility_MakeWay.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Characters/P1CharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

UP1GameplayAbility_MakeWay::UP1GameplayAbility_MakeWay()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_MakeWay);
	SetAssetTags(Tags);

	InputTag = TAG_InputTag_Ability_Q;
}

void UP1GameplayAbility_MakeWay::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 쿨다운은 지속시간이 끝나야 시작되므로, 여기서는 코스트만 커밋한다 (표준 CommitAbility 미사용).
	if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentTick = 0;
	bCooldownApplied = false;

	GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this,
		&UP1GameplayAbility_MakeWay::OnWhirlwindTick, TickPeriod, true);

	// 코스메틱 Multicast는 서버에서만 발동 (LocalPredicted라 클라 예측 인스턴스에서도 이 코드가 실행됨).
	if (ActorInfo->IsNetAuthority() && WhirlwindParticleTemplate)
	{
		if (AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo())
		{
			SourceCharacter->MulticastSetAttachedParticleEffect(WhirlwindParticleTemplate, WhirlwindSocketName);
		}
	}

	UE_LOG(LogP1, Log, TEXT("[MakeWay] 발동 — %.1f초 간격 %d틱"), TickPeriod, TotalTicks);
}

void UP1GameplayAbility_MakeWay::OnWhirlwindTick()
{
	++CurrentTick;

	// 데미지/디버프 판정은 서버에서만 수행.
	if (CurrentActorInfo->IsNetAuthority())
	{
		AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
		if (IsValid(SourceCharacter))
		{
			const FVector Center = SourceCharacter->GetActorLocation();
			const TArray<AActor*> Enemies = GetEnemiesInRadius(Center, WhirlwindRadius, HalfHeight);

			UE_LOG(LogP1, Log, TEXT("[MakeWay] Tick %d/%d — EnemyCount=%d"), CurrentTick, TotalTicks, Enemies.Num());

			for (AActor* Enemy : Enemies)
			{
				ApplyDamageToTarget(Enemy, 1.0f);

				if (ArmorShredDebuffEffectClass)
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
									ArmorShredDebuffEffectClass, GetAbilityLevel(), Ctx);
								if (DebuffSpec.IsValid())
								{
									DebuffSpec.Data->SetSetByCallerMagnitude(TAG_Data_DebuffMagnitude, ArmorShredMagnitude);
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
				DrawDebugSphere(GetWorld(), Center, WhirlwindRadius, 24, FColor::Orange, false, TickPeriod * 0.9f, 0, 2.0f);
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

void UP1GameplayAbility_MakeWay::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);
	}

	// 정상 종료(4틱 완료)든 조기 취소든 이 경로를 항상 거치므로, 회오리 지속 파티클도 여기서 일괄 정리.
	if (ActorInfo->IsNetAuthority())
	{
		if (AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo())
		{
			SourceCharacter->MulticastStopAttachedParticleEffect();
		}
	}

	// 중간에 취소/중단되어 4틱을 다 못 채웠어도 코스트는 이미 소모됐으므로 쿨다운은 적용한다.
	if (!bCooldownApplied)
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
