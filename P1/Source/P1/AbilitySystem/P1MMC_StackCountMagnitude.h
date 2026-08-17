// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "P1MMC_StackCountMagnitude.generated.h"

// 스택형 버프 GE의 Modifier Magnitude를 "스택당 고정값 × 현재 스택 수"로 계산하는 범용 MMC. GE 자체는
// Stacking Type=AggregateByTarget으로 스택 수를 관리하고, 이 클래스는 FGameplayEffectSpec::GetStackCount()를
// 읽어 그 스택 수에 비례한 값을 반환한다 — 스택형 아이템 효과가 여러 개 생길 것으로 예상돼(예: 더스트
// 데빌의 공격속도 스택) 아이템 하나에 종속된 이름 대신 범용 이름으로 만들어 재사용한다.
UCLASS()
class P1_API UP1MMC_StackCountMagnitude : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

	// 스택 1개당 부여할 값(예: 공격속도 +3 = 3.0, AttackSpeed는 100=기준 퍼센트 스케일이므로 3%면 3.0).
	// 이 클래스를 상속하는 BP(예: MMC_DustDevil_AttackSpeedPerStack)의 Class Defaults에서 지정 — 네이티브
	// C++ 클래스 CDO는 콘텐츠 브라우저에서 값을 편집할 방법이 없으므로(다른 MMC들과 동일한 이유) 반드시
	// BP 서브클래스를 만들어야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "Stacking")
	float MagnitudePerStack = 1.0f;
};
