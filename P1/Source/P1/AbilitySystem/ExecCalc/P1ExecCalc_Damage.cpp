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
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageReduction);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamage);

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
		DEFINE_ATTRIBUTE_CAPTUREDEF(UP1AttributeSet, DamageReduction, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UP1AttributeSet, MaxHealth, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UP1AttributeSet, CriticalChance, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UP1AttributeSet, CriticalDamage, Source, true);

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
	RelevantAttributesToCapture.Add(DamageStatics().DamageReductionDef);
	RelevantAttributesToCapture.Add(DamageStatics().MaxHealthDef);
	RelevantAttributesToCapture.Add(DamageStatics().SourceMaxHealthDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalDamageDef);
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

	float DamageReduction = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageReductionDef, EvalParams, DamageReduction);
	DamageReduction = FMath::Clamp(DamageReduction, 0.0f, 1.0f);

	float TargetMaxHealth = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MaxHealthDef, EvalParams, TargetMaxHealth);

	float SourceMaxHealth = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().SourceMaxHealthDef, EvalParams, SourceMaxHealth);

	float CriticalChance = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalChanceDef, EvalParams, CriticalChance);

	float CriticalDamage = 1.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalDamageDef, EvalParams, CriticalDamage);

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

	// 크리티컬은 기본공격에서만 발생해야 한다(스킬은 크리티컬 대상이 아님) — 기본공격 여부는
	// CapturedSourceTags에 실린 Ability.BasicAttack 태그로 판별(ApplyDamageToTarget/ApplyComboHitDamage가
	// 어빌리티의 Asset Tags를 스펙에 채워 넣는 경로, UP1DamageGameplayAbility 참고). 이 게이트가 없으면
	// 스킬 데미지에도 CriticalChance/CriticalDamage가 그대로 굴러 크리티컬이 발생하는 버그가 생긴다
	// (실제 발견된 버그) — Q/E/RMB/R 등 모든 스킬은 CriticalChance가 몇 %든 항상 크리티컬 불가.
	const bool bIsBasicAttack = EvalParams.SourceTags && EvalParams.SourceTags->HasTag(TAG_Ability_BasicAttack);

	// 크리티컬 판정 — ExecCalc는 서버(권한 보유 측)에서만 실행되므로 여기서 한 번만 굴려도 안전하다
	// (클라이언트/서버가 각자 굴려서 어긋날 여지가 없음).
	const bool bIsCriticalHit = bIsBasicAttack && (FMath::FRand() < CriticalChance);
	const float CriticalMultiplier = bIsCriticalHit ? CriticalDamage : 1.0f;

	const float PreMitigation = Raw * Multiplier * CriticalMultiplier;

	// 고정 피해(Data.DamageType.True)면 방어력 감산 자체를 건너뛴다 — 관통(Penetration)으로 무효화되는
	// 게 아니라 애초에 방어력 계산이 없는 별개 축이라 PassThrough=1.0으로 취급(DamageReduction은 그대로 적용됨).
	const bool bIsTrueDamage = EvalParams.SourceTags && EvalParams.SourceTags->HasTag(TAG_Data_DamageType_True);

	// 관통은 방어력을 flat 차감. 이후 방어 감산 공식 적용.
	const float EffectiveArmor = FMath::Max(0.0f, Armor - Penetration);
	constexpr float ArmorConstant = 100.0f;
	const float PassThrough = bIsTrueDamage ? 1.0f : (ArmorConstant / (EffectiveArmor + ArmorConstant));

	// DamageReduction은 방어력 감산 이후의 최종 승산 레이어 — 관통(Penetration)으로 무효화되지 않는다
	// (방어력과는 별개 축이라는 게 이 스탯의 존재 이유, 사면(아이템) "용기" 참고).
	const float FinalDamage = FMath::Max(0.0f, PreMitigation * PassThrough * (1.0f - DamageReduction));

	// 누가(어떤 어빌리티로) 누구에게 얼마를 입혔는지 — 버그 검증용(예: 크리티컬이 기본공격에만 붙는지)으로
	// 액터 이름+소스 태그(어느 어빌리티인지는 CapturedSourceTags에 실린 Ability.* 태그로 식별)를 함께 남긴다.
	const AActor* SourceActor = ExecutionParams.GetSourceAbilitySystemComponent()
		? ExecutionParams.GetSourceAbilitySystemComponent()->GetAvatarActor() : nullptr;
	const AActor* TargetActor = ExecutionParams.GetTargetAbilitySystemComponent()
		? ExecutionParams.GetTargetAbilitySystemComponent()->GetAvatarActor() : nullptr;
	const FString SourceTagsStr = EvalParams.SourceTags ? EvalParams.SourceTags->ToStringSimple() : TEXT("(none)");

	UE_LOG(LogP1, Log, TEXT("[ExecCalc_Damage] %s → %s | SourceTags=[%s] | Flat=%.1f PhysCoeff=%.2f Power=%.1f MagCoeff=%.2f MagPower=%.1f TargetMaxHPCoeff=%.2f TargetMaxHP=%.1f SourceMaxHPCoeff=%.2f SourceMaxHP=%.1f Mult=%.2f Raw=%.1f | Crit=%d(BasicAttack=%d, %.0f%% 확률, %.2fx) | True=%d Armor=%.1f Pen=%.1f EffArmor=%.1f PassThrough=%.2f DamageReduction=%.2f → Final=%.1f"),
		SourceActor ? *SourceActor->GetName() : TEXT("null"), TargetActor ? *TargetActor->GetName() : TEXT("null"), *SourceTagsStr,
		FlatDamage, PhysicalPowerCoeff, Power, MagicalPowerCoeff, MagicalPower, TargetMaxHealthPctCoeff, TargetMaxHealth,
		SourceMaxHealthPctCoeff, SourceMaxHealth, Multiplier, Raw, bIsCriticalHit, bIsBasicAttack, CriticalChance * 100.0f, CriticalMultiplier,
		bIsTrueDamage, Armor, Penetration, EffectiveArmor, PassThrough, DamageReduction, FinalDamage);

	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(UP1AttributeSet::GetDamageAttribute(), EGameplayModOp::Additive, FinalDamage));

	// 크리티컬 판정 결과를 메타 어트리뷰트로 함께 출력(Override — 누적이 아니라 이번 히트의 값 그대로) —
	// PostGameplayEffectExecute가 Damage와 같은 타이밍에 읽고 즉시 0으로 리셋한다. 온-히트 크리티컬
	// 반응 아이템(더스트 데빌의 "매너스" 등)이 이 값을 필요로 한다.
	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(UP1AttributeSet::GetCriticalHitFlagAttribute(), EGameplayModOp::Override, bIsCriticalHit ? 1.0f : 0.0f));
}
