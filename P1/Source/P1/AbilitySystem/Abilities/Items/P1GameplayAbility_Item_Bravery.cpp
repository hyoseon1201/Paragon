// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Items/P1GameplayAbility_Item_Bravery.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Characters/P1CharacterBase.h"
#include "GameplayEffect.h"
#include "P1.h"

UP1GameplayAbility_Item_Bravery::UP1GameplayAbility_Item_Bravery()
{
	// Event.Character.Immobilized로만 트리거 — 플레이어 입력으로 직접 발동하지 않음.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = TAG_Event_Character_Immobilized;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 백그라운드 반응형 패시브 — 다른 어빌리티 사용 중이어도 발동 가능해야 하므로 State.Attacking 상호배타
	// 규칙에서 제외(StoicismDeflect와 동일 패턴). **State.Stunned도 반드시 제외해야 한다** — 이 어빌리티는
	// 정확히 "스턴(이동 불가)에 걸리는 순간" 발동해야 하는데, 베이스 생성자가 기본으로 ActivationBlockedTags에
	// State.Stunned를 걸어두므로 이걸 빼지 않으면 정작 필요한 순간에 발동 자체가 막혀버린다.
	ActivationBlockedTags.RemoveTag(TAG_State_Attacking);
	ActivationOwnedTags.RemoveTag(TAG_State_Attacking);
	ActivationBlockedTags.RemoveTag(TAG_State_Stunned);
}

void UP1GameplayAbility_Item_Bravery::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// "지금 사용 가능한지"는 CooldownGameplayEffectClass를 통해 엔진의 CanActivateAbility()가 이 함수가
	// 호출되기 전에 이미 확인했으므로(쿨다운 중이면 애초에 여기 도달하지 않음), 여기선 그냥 버프를
	// 걸고 쿨다운을 커밋하면 된다.
	if (BuffEffectClass)
	{
		ApplyEffectToSelf(BuffEffectClass);
	}

	if (AP1CharacterBase* Character = GetP1CharacterFromActorInfo())
	{
		Character->MulticastPlayNiagaraEffect(ShieldEffect, ShieldEffectSocketName);
	}

	if (const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect())
	{
		ApplyEffectToSelf(CooldownGE->GetClass());
	}

	UE_LOG(LogP1, Log, TEXT("[Bravery] 이동 불가 감지 — 버프 적용 및 쿨다운 커밋 (%s)"),
		ActorInfo && ActorInfo->AvatarActor.IsValid() ? *ActorInfo->AvatarActor->GetName() : TEXT("null"));

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
