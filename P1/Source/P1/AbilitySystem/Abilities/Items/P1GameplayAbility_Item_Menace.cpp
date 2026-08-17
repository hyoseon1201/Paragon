// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Items/P1GameplayAbility_Item_Menace.h"
#include "Player/P1PlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "P1.h"

void UP1GameplayAbility_Item_Menace::OnBasicAttackHitDealt(AActor* Target, bool bWasCritical)
{
	// 영웅에게만 발동 — 정글 몬스터는 제외(애쉬브링어의 영웅/몬스터 이중 티어와 달리 매너스는 영웅 전용).
	if (!Target || Cast<AP1PlayerState>(Target) == nullptr)
	{
		return;
	}

	if (!MenaceBuffEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(MenaceBuffEffectClass, 1.0f, EffectContext);
	if (SpecHandle.IsValid())
	{
		// 크리티컬 적중이면 2스택, 아니면 1스택 요청 — 이미 활성 스택이 있으면 GAS가 그 위에 이만큼
		// 더해준다(Stack Limit에서 자동 클램프, FGameplayEffectSpec::SetStackCount 표준 사용법).
		SpecHandle.Data->SetStackCount(bWasCritical ? 2 : 1);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	UE_LOG(LogP1, Log, TEXT("[Menace] 영웅 적중 — %s (크리티컬=%d, %d스택 요청)"),
		*Target->GetName(), bWasCritical, bWasCritical ? 2 : 1);
}
