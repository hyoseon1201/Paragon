// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1GameplayAbility.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Characters/P1CharacterBase.h"
#include "Player/P1PlayerController.h"
#include "GameplayEffect.h"

UP1GameplayAbility::UP1GameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 이미 활성화된 어빌리티에 대한 press/release 입력을 클라 → 서버로 직접 복제한다.
	// 이게 false면 커스텀 입력 라우팅에서 서버 어빌리티의 InputPressed/InputReleased가
	// 절대 호출되지 않아, 홀드 콤보가 서버에서 종료되지 않고 한 타 더 나간다.
	bReplicateInputDirectly = true;

	// 게임 전역 규칙: 어빌리티 사용 중에는 다른 어빌리티를 발동할 수 없다 (WASD 이동은 어빌리티가 아니라
	// 영향 없음). State.Attacking을 모든 어빌리티가 공유해 상호 배타적으로 만든다 — 개별 어빌리티마다
	// 반복 설정할 필요 없이 베이스에서 한 번에 처리.
	ActivationOwnedTags.AddTag(TAG_State_Attacking);
	ActivationBlockedTags.AddTag(TAG_State_Attacking);
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

FActiveGameplayEffectHandle UP1GameplayAbility::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass,
	FGameplayTag SetByCallerTag, float SetByCallerMagnitude) const
{
	if (!EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(EffectClass, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	if (SetByCallerTag.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, SetByCallerMagnitude);
	}

	return ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
}
