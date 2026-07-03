// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "P1DamageGameplayAbility.generated.h"

class UGameplayEffect;

// 데미지를 입히는 모든 어빌리티의 공통 베이스.
// 타겟 ASC에 DamageEffectClass를 적용하는 ApplyDamageToTarget() 헬퍼를 제공하며,
// MeleeAttack / RangedAttack 및 스킬샷 계열 데미지 어빌리티가 이 클래스를 상속한다.
UCLASS(Abstract)
class P1_API UP1DamageGameplayAbility : public UP1GameplayAbility
{
	GENERATED_BODY()

protected:
	// DamageMultiplier: 1.0 = 100% 데미지 (주목표), Cleave = 범위 데미지.
	// ExecCalc_Damage가 SetByCaller로 이 값과 DamageCoefficient를 읽어 최종 데미지를 산출한다.
	void ApplyDamageToTarget(AActor* TargetActor, float DamageMultiplier = 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 스탯 무관 고정 데미지 (Data.Damage.Flat 채널).
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float FlatDamage = 0.0f;

	// PhysicalPower 계수 (Data.Damage.PhysicalPower 채널). ExecCalc에서 Source.PhysicalPower와 곱해진다.
	// 기본공격=1.0, 강한 스킬일수록 높게. BP 서브클래스에서 조정.
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float PhysicalPowerCoefficient = 1.0f;
};
