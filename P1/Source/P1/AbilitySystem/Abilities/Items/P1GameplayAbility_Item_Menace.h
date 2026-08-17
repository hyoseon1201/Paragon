// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1GameplayAbility_OnHitItemAbility.h"
#include "P1GameplayAbility_Item_Menace.generated.h"

class UGameplayEffect;

// 더스트 데빌(아이템) 고유효과 "매너스" — 기본 공격이 영웅에게 적중할 때마다(정글 몬스터는 제외) 자신에게
// 스택형 버프(MenaceBuffEffectClass)를 적용한다. 크리티컬 적중이면 1스택이 아니라 2스택을 요청 —
// 실제 수치(스택당 공격속도%, 스택 상한, 상한 도달 시 이동속도%)는 전부 그 GE 자체의 Modifier(Custom
// Calculation Class로 스택 수 기반 계산, UP1MMC_StackCountMagnitude/UP1MMC_StackThresholdMagnitude 참고)에 있다.
UCLASS()
class P1_API UP1GameplayAbility_Item_Menace : public UP1GameplayAbility_OnHitItemAbility
{
	GENERATED_BODY()

protected:
	virtual void OnBasicAttackHitDealt(AActor* Target, bool bWasCritical) override;

	UPROPERTY(EditDefaultsOnly, Category = "Menace")
	TSubclassOf<UGameplayEffect> MenaceBuffEffectClass;
};
