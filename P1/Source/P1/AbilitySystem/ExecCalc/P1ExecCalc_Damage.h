// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "P1ExecCalc_Damage.generated.h"

// 물리 데미지 공식을 한 곳에서 계산하는 ExecutionCalculation.
// 모든 데미지 어빌리티(기본공격 + 스킬)가 공유하는 GE_Damage가 이 Exec를 실행한다.
//
// 계수는 이름 붙은 채널(SetByCaller)로 전달되어 "어떤 스탯 기반인지" 명시된다.
// 어빌리티는 자기가 쓰는 채널만 채우고, 나머지는 0으로 무시된다.
//
// Raw            = Data.Damage.Flat + Data.Damage.PhysicalPower * Source.PhysicalPower
// PreMitigation  = Raw * Data.DamageMultiplier
// EffectiveArmor = max(0, Target.PhysicalArmor - Source.PhysicalPenetration)
// Mitigation     = ArmorConstant / (EffectiveArmor + ArmorConstant)   // 데미지가 통과하는 비율
// FinalDamage    = PreMitigation * Mitigation → Target.Damage(메타)에 누적
//
// 미래 확장: MagicalPower/TargetMaxHealthPct 채널 추가, damage-type 태그로 물리/마법 방어 선택.
UCLASS()
class P1_API UP1ExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UP1ExecCalc_Damage();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
