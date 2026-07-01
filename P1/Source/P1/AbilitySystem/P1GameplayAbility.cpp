// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1GameplayAbility.h"
#include "Characters/P1CharacterBase.h"
#include "Player/P1PlayerController.h"

UP1GameplayAbility::UP1GameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

AP1CharacterBase* UP1GameplayAbility::GetP1CharacterFromActorInfo() const
{
	return Cast<AP1CharacterBase>(GetAvatarActorFromActorInfo());
}

AP1PlayerController* UP1GameplayAbility::GetP1PlayerControllerFromActorInfo() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	return ActorInfo ? Cast<AP1PlayerController>(ActorInfo->PlayerController.Get()) : nullptr;
}
