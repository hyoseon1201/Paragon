// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "P1GameplayAbility_StoicismVitality.generated.h"

class UGameplayEffect;

// Stoicism(패시브) — 재생+방어력 절반. 초당 최대체력 비례 회복 + 고정 방어력을 항상 유지하며,
// 잃은 체력 비율에 비례해 두 값 모두 증폭되고(1% 잃을 때마다 1%), 체력 50% 미만이면 회복만 추가로 2배.
//
// 회복/방어력 둘 다 "현재 체력"이라는 계속 바뀌는 값에 의존하는 동적 수치라, Infinite Duration GE를
// 한 번 적용하고 끝내는 방식으로는 표현할 수 없다 — Make Way/Stone Forged Soul과 같은 반복 타이머
// 패턴을 재사용하되, 이 어빌리티는 특정 지속시간이 있는 스킬이 아니라 캐릭터 생존 내내 도는
// 상시 패시브라 타이머가 끝나지 않고 계속 반복된다는 점만 다르다.
UCLASS()
class P1_API UP1GameplayAbility_StoicismVitality : public UP1GameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_StoicismVitality();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	void OnVitalityTick();

	// 재스캔/재적용 간격(초). "초당" 수치이므로 1초가 기본.
	UPROPERTY(EditDefaultsOnly, Category = "Stoicism|Vitality")
	float TickPeriod = 1.0f;

	// 초당 회복량 = MaxHealth * RegenPercentPerSecond * (1+잃은체력비율) * (50% 미만이면 2, 아니면 1).
	UPROPERTY(EditDefaultsOnly, Category = "Stoicism|Vitality", meta = (ClampMin = "0.0"))
	float RegenPercentPerSecond = 0.0015f;

	// 체력 비율이 이 값 미만이면 회복량이 추가로 2배.
	UPROPERTY(EditDefaultsOnly, Category = "Stoicism|Vitality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthThreshold = 0.5f;

	// 즉시 회복 GE — Health += SetByCaller(Data.Heal.Flat). Stone Forged Soul과 같은 에셋 재사용 가능.
	UPROPERTY(EditDefaultsOnly, Category = "Stoicism|Vitality")
	TSubclassOf<UGameplayEffect> HealEffectClass;

	// 방어력 보너스 = (BaseArmorBonus + ArmorBonusPerLevel*(레벨-1)) * (1+잃은체력비율).
	UPROPERTY(EditDefaultsOnly, Category = "Stoicism|Vitality")
	float BaseArmorBonus = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Stoicism|Vitality")
	float ArmorBonusPerLevel = 1.0f;

	// 방어력 버프 GE — PhysicalArmor += SetByCaller(Data.ArmorBonus.Flat). Duration 있는 상태로
	// 매 틱 최신 값으로 재적용되므로, GE 에셋에서 Stacking Type=AggregateByTarget/Stack Limit=1로
	// 설정해 누적이 아니라 항상 최신 값으로 덮어써야 한다 (Stone Forged Soul 슬로우 디버프와 같은 패턴).
	UPROPERTY(EditDefaultsOnly, Category = "Stoicism|Vitality")
	TSubclassOf<UGameplayEffect> ArmorBuffEffectClass;

	FTimerHandle VitalityTickTimerHandle;
};
