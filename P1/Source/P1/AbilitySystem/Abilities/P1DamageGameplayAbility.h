// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "ScalableFloat.h"
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
	// BonusFlat/BonusPhysicalPowerCoeff/BonusMagicalPowerCoeff/BonusTargetMaxHealthPctCoeff/BonusSourceMaxHealthPctCoeff:
	// 이번 호출 한정으로 클래스 기본 계수 위에 얹는 추가 계수 (예: 버프로 강화된 한 방만 추가 데미지를 얹는 경우).
	// 기본값 0 = 평소와 동일. ExecCalc_Damage가 SetByCaller로 이 값들을 읽어 최종 데미지를 산출한다.
	void ApplyDamageToTarget(AActor* TargetActor, float DamageMultiplier = 1.0f,
		float BonusFlat = 0.0f, float BonusPhysicalPowerCoeff = 0.0f,
		float BonusMagicalPowerCoeff = 0.0f, float BonusTargetMaxHealthPctCoeff = 0.0f,
		float BonusSourceMaxHealthPctCoeff = 0.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 스탯 무관 고정 데미지 (Data.Damage.Flat 채널). 어빌리티 레벨에 따라 값이 달라질 수 있어
	// FScalableFloat(Value * CurveTable[Level])로 관리 — Curve 미지정 시 기존처럼 고정값으로 동작.
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	FScalableFloat FlatDamage = FScalableFloat(0.0f);

	// PhysicalPower 계수 (Data.Damage.PhysicalPower 채널). ExecCalc에서 Source.PhysicalPower와 곱해진다.
	// 기본공격=1.0, 강한 스킬일수록 높게. BP 서브클래스에서 조정.
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float PhysicalPowerCoefficient = 1.0f;

	// MagicalPower 계수 (Data.Damage.MagicalPower 채널). 물리 딜러는 대부분 0(미사용).
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float MagicalPowerCoefficient = 0.0f;

	// Target MaxHealth 계수 (Data.Damage.TargetMaxHealthPct 채널). 대부분 어빌리티는 0(미사용).
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float TargetMaxHealthPctCoefficient = 0.0f;

	// 시전자(Source) MaxHealth 계수 (Data.Damage.SourceMaxHealthPct 채널). 대부분 어빌리티는 0(미사용).
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float SourceMaxHealthPctCoefficient = 0.0f;

	// 시전자(어빌리티 아바타) 주변 반경 내 적을 찾는 공용 헬퍼 — 구 오버랩 + 같은 팀 제외 + 높이 필터만 수행.
	// 전방 반원처럼 방향성 필터가 추가로 필요한 경우(MeleeAttack 등) 호출자가 반환된 목록에 덧씌운다.
	TArray<AActor*> GetEnemiesInRadius(const FVector& Center, float Radius, float HalfHeight) const;
};
