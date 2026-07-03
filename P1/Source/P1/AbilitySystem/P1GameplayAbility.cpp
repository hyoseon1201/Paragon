// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1GameplayAbility.h"
#include "Characters/P1CharacterBase.h"
#include "Player/P1PlayerController.h"

UP1GameplayAbility::UP1GameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 이미 활성화된 어빌리티에 대한 press/release 입력을 클라 → 서버로 직접 복제한다.
	// 이게 false면 커스텀 입력 라우팅에서 서버 어빌리티의 InputPressed/InputReleased가
	// 절대 호출되지 않아, 홀드 콤보가 서버에서 종료되지 않고 한 타 더 나간다.
	bReplicateInputDirectly = true;
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
