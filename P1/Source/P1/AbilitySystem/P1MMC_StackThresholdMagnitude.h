// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "P1MMC_StackThresholdMagnitude.generated.h"

// 스택형 버프 GE가 "특정 스택 수 이상 도달했을 때만" 추가 보너스를 주는 패턴을 위한 범용 MMC(예: 더스트
// 데빌 "매너스"의 6스택 도달 시 이동속도 보너스). RequiredStackCount 미만이면 0, 도달하면 BonusMagnitude를
// 그대로 반환 — 선형 스케일링(UP1MMC_StackCountMagnitude)과 짝을 이루는 임계값 버전.
UCLASS()
class P1_API UP1MMC_StackThresholdMagnitude : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Stacking")
	int32 RequiredStackCount = 1;

	// 임계값 도달 시 반환할 값 — Modifier Op에 맞게 지정할 것(Additive면 +10 같은 델타, Multiply로
	// "10% 증가"를 표현하려면 1.10).
	UPROPERTY(EditDefaultsOnly, Category = "Stacking")
	float BonusMagnitude = 0.0f;

	// 임계값 미달 시 반환할 값 — **Additive 모디파이어면 0.0(기본값 그대로 두면 됨), Multiply
	// 모디파이어면 반드시 1.0으로 지정할 것**(곱셈의 항등원이 1이라 0으로 두면 스탯 전체가 0으로
	// 곱해져 사라지는 심각한 버그가 된다 — 예: 더스트 데빌 매너스의 6스택 이동속도 보너스는 Multiply
	// 모디파이어라 이 값을 1.0으로 지정해야 한다).
	UPROPERTY(EditDefaultsOnly, Category = "Stacking")
	float BelowThresholdMagnitude = 0.0f;
};
