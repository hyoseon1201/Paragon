// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1GameplayAbility_OnHitItemAbility.h"
#include "P1GameplayAbility_Item_ChronoStrike.generated.h"

// 애쉬브링어(아이템) 고유효과 "크로노 스트라이크" — 기본 공격이 적중할 때마다 자신의 Q/E/RMB 쿨다운을
// 대상 유형(영웅/정글 몬스터)에 따라 다른 비율로 감소시킨다. 실제 감소는 범용
// UP1AbilitySystemComponent::ReduceCooldownByInputTag()에 위임 — 이 어빌리티는 "얼마나 줄일지"만 안다.
UCLASS()
class P1_API UP1GameplayAbility_Item_ChronoStrike : public UP1GameplayAbility_OnHitItemAbility
{
	GENERATED_BODY()

protected:
	virtual void OnBasicAttackHitDealt(AActor* Target, bool bWasCritical) override;

	UPROPERTY(EditDefaultsOnly, Category = "ChronoStrike")
	float HeroCDRPercent = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category = "ChronoStrike")
	float MonsterCDRPercent = 0.04f;
};
