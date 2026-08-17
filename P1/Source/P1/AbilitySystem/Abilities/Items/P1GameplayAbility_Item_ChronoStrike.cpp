// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Items/P1GameplayAbility_Item_ChronoStrike.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Player/P1PlayerState.h"
#include "P1.h"

void UP1GameplayAbility_Item_ChronoStrike::OnBasicAttackHitDealt(AActor* Target, bool bWasCritical)
{
	// 대상이 AP1PlayerState로 캐스트되면 영웅, 실패하면 정글 몬스터(ASC를 Pawn이 직접 호스팅하므로) —
	// HandleKillRewards 등 이 프로젝트 전반에서 쓰는 것과 동일한 판별 관례.
	const bool bTargetIsMonster = Cast<AP1PlayerState>(Target) == nullptr;
	const float Percent = bTargetIsMonster ? MonsterCDRPercent : HeroCDRPercent;

	UP1AbilitySystemComponent* P1ASC = Cast<UP1AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (!P1ASC)
	{
		return;
	}

	P1ASC->ReduceCooldownByInputTag(TAG_InputTag_Ability_Q, Percent);
	P1ASC->ReduceCooldownByInputTag(TAG_InputTag_Ability_E, Percent);
	P1ASC->ReduceCooldownByInputTag(TAG_InputTag_Ability_RMB, Percent);

	UE_LOG(LogP1, Log, TEXT("[ChronoStrike] 적중 — 대상=%s(몬스터=%d) %.0f%% 쿨감 적용"),
		Target ? *Target->GetName() : TEXT("null"), bTargetIsMonster, Percent * 100.0f);
}
