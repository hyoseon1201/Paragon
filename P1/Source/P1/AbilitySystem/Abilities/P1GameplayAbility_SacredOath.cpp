// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/P1GameplayAbility_SacredOath.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "GameplayEffect.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"

UP1GameplayAbility_SacredOath::UP1GameplayAbility_SacredOath()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_SacredOath);
	SetAssetTags(Tags);

	InputTag = TAG_InputTag_Ability_E;
}

void UP1GameplayAbility_SacredOath::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (BuffEffectClass)
	{
		const FActiveGameplayEffectHandle BuffHandle = ApplyEffectToSelf(BuffEffectClass);
		UE_LOG(LogP1, Log, TEXT("[SacredOath] 버프 적용됨 | Auth=%d BuffHandle.Valid=%d"),
			ActorInfo->IsNetAuthority() ? 1 : 0, BuffHandle.IsValid() ? 1 : 0);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[SacredOath] BuffEffectClass 미설정 — GA BP에서 지정하세요."));
	}

	if (CastMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, CastMontage, 1.0f);
		MontageTask->OnCompleted.AddDynamic(this, &UP1GameplayAbility_SacredOath::OnCastMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &UP1GameplayAbility_SacredOath::OnCastMontageFinished);
		MontageTask->OnCancelled.AddDynamic(this, &UP1GameplayAbility_SacredOath::OnCastMontageFinished);
		MontageTask->ReadyForActivation();
	}
	else
	{
		// 몽타주 미설정 시 기존처럼 즉시 종료.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UP1GameplayAbility_SacredOath::OnCastMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
