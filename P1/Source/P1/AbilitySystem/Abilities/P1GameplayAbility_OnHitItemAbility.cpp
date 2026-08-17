// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/P1GameplayAbility_OnHitItemAbility.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "P1.h"

UP1GameplayAbility_OnHitItemAbility::UP1GameplayAbility_OnHitItemAbility()
{
	// 백그라운드 반응형 패시브 — 다른 어빌리티(스킬)를 쓰는 중이어도 기본 공격 자체는 여전히 나갈 수
	// 있으므로 State.Attacking 상호배타 규칙에서 제외한다(StoicismDeflect와 동일한 이유).
	ActivationBlockedTags.RemoveTag(TAG_State_Attacking);
	ActivationOwnedTags.RemoveTag(TAG_State_Attacking);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = TAG_Event_Character_BasicAttackHitDealt;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// 순수 서버 로직 — 클라 예측이 필요 없다(StoicismDeflect와 동일).
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UP1GameplayAbility_OnHitItemAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TriggerEventData)
	{
		const bool bWasCritical = TriggerEventData->EventMagnitude > 0.5f;
		OnBasicAttackHitDealt(const_cast<AActor*>(TriggerEventData->Target.Get()), bWasCritical);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
