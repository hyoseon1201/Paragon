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
	// DamageMultiplier: 1.0 = 100% 데미지 (주목표), Cleave/100 = 범위 데미지.
	// ExecCalc_Damage가 구현되면 이 값을 SetByCaller로 읽어 최종 데미지에 곱한다.
	void ApplyDamageToTarget(AActor* TargetActor, float DamageMultiplier = 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};
