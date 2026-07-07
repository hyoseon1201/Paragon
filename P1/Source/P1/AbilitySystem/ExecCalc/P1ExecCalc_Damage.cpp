// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/ExecCalc/P1ExecCalc_Damage.h"
#include "P1.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1GameplayTags.h"

// 캡처할 어트리뷰트 정의. Source(공격자)에서 Power류/Penetration/MaxHealth, Target(피격자)에서 Armor/MaxHealth를 읽는다.
struct FP1DamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MagicalPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalPenetration);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalArmor);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);

	// MaxHealth는 Target(위 매크로)과 Source 양쪽에서 캡처해야 하는데, DEFINE_ATTRIBUTE_CAPTUREDEF는
	// 프로퍼티 이름을 그대로 멤버 이름에 써서(GET_MEMBER_NAME_CHECKED) 같은 이름을 두 번 매크로로
	// 선언할 수 없다 — 그래서 Source쪽은 매크로 없이 직접 생성자로 구성한다.
	FGameplayEffectAttributeCaptureDefinition SourceMaxHealthDef;

	FP1DamageStatics()
	{
		// 마지막 인자 bSnapshot: Source 스탯은 스펙 생성 시점 값으로 스냅샷, Target 스탯(방어력/최대체력)은 적용 시점 값 사용.
		DEFINE_ATTRIBUTE_CAPTUREDEF(UP1AttributeSet, PhysicalPower, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UP1AttributeSet, MagicalPower, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UP1AttributeSet, PhysicalPenetration, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UP1AttributeSet, PhysicalArmor, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UP1AttributeSet, MaxHealth, Target, false);

		SourceMaxHealthDef = FGameplayEffectAttributeCaptureDefinition(
			UP1AttributeSet::GetMaxHealthAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
	}
};

static const FP1DamageStatics& DamageStatics()
{
	static FP1DamageStatics Statics;
	return Statics;
}

UP1ExecCalc_Damage::UP1ExecCalc_Damage()
{
	RelevantAttributesToCapture.Add(DamageStatics().PhysicalPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicalPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().PhysicalPenetrationDef);
	RelevantAttributesToCapture.Add(DamageStatics().PhysicalArmorDef);
	RelevantAttributesToCapture.Add(DamageStatics().MaxHealthDef);
	RelevantAttributesToCapture.Add(DamageStatics().SourceMaxHealthDef);
}

void UP1ExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float Power = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PhysicalPowerDef, EvalParams, Power);

	float MagicalPower = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MagicalPowerDef, EvalParams, MagicalPower);

	float Penetration = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PhysicalPenetrationDef, EvalParams, Penetration);

	float Armor = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PhysicalArmorDef, EvalParams, Armor);
	Armor = FMath::Max(0.0f, Armor);

	float TargetMaxHealth = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MaxHealthDef, EvalParams, TargetMaxHealth);

	float SourceMaxHealth = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().SourceMaxHealthDef, EvalParams, SourceMaxHealth);

	// 데미지 계수 채널. 어빌리티가 채우지 않은 채널은 0(=기여 없음).
	// 배율만 미지정 시 1.0 (감쇠 없음).
	const float FlatDamage = Spec.GetSetByCallerMagnitude(TAG_Data_Damage_Flat, false, 0.0f);
	const float PhysicalPowerCoeff = Spec.GetSetByCallerMagnitude(TAG_Data_Damage_PhysicalPower, false, 0.0f);
	const float MagicalPowerCoeff = Spec.GetSetByCallerMagnitude(TAG_Data_Damage_MagicalPower, false, 0.0f);
	const float TargetMaxHealthPctCoeff = Spec.GetSetByCallerMagnitude(TAG_Data_Damage_TargetMaxHealthPct, false, 0.0f);
	const float SourceMaxHealthPctCoeff = Spec.GetSetByCallerMagnitude(TAG_Data_Damage_SourceMaxHealthPct, false, 0.0f);
	const float Multiplier = Spec.GetSetByCallerMagnitude(TAG_Data_DamageMultiplier, false, 1.0f);

	const float Raw = FlatDamage
		+ PhysicalPowerCoeff * Power
		+ MagicalPowerCoeff * MagicalPower
		+ TargetMaxHealthPctCoeff * TargetMaxHealth
		+ SourceMaxHealthPctCoeff * SourceMaxHealth;
	const float PreMitigation = Raw * Multiplier;

	// 관통은 방어력을 flat 차감. 이후 방어 감산 공식 적용.
	const float EffectiveArmor = FMath::Max(0.0f, Armor - Penetration);
	constexpr float ArmorConstant = 100.0f;
	const float PassThrough = ArmorConstant / (EffectiveArmor + ArmorConstant);

	const float FinalDamage = FMath::Max(0.0f, PreMitigation * PassThrough);

	UE_LOG(LogP1, Log, TEXT("[ExecCalc_Damage] Flat=%.1f PhysCoeff=%.2f Power=%.1f MagCoeff=%.2f MagPower=%.1f TargetMaxHPCoeff=%.2f TargetMaxHP=%.1f SourceMaxHPCoeff=%.2f SourceMaxHP=%.1f Mult=%.2f Raw=%.1f | Armor=%.1f Pen=%.1f EffArmor=%.1f PassThrough=%.2f → Final=%.1f"),
		FlatDamage, PhysicalPowerCoeff, Power, MagicalPowerCoeff, MagicalPower, TargetMaxHealthPctCoeff, TargetMaxHealth,
		SourceMaxHealthPctCoeff, SourceMaxHealth, Multiplier, Raw, Armor, Penetration, EffectiveArmor, PassThrough, FinalDamage);

	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(UP1AttributeSet::GetDamageAttribute(), EGameplayModOp::Additive, FinalDamage));
}
