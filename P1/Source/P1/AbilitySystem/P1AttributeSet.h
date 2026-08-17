// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "P1AttributeSet.generated.h"

class AP1PlayerState;
class UGameplayEffect;

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class P1_API UP1AttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UP1AttributeSet();

	// --- Health / Mana (Current / Max 쌍) ---

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes|Vital")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes|Vital")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Attributes|Vital")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "Attributes|Vital")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, MaxMana)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HealthRegen, Category = "Attributes|Vital")
	FGameplayAttributeData HealthRegen;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, HealthRegen)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ManaRegen, Category = "Attributes|Vital")
	FGameplayAttributeData ManaRegen;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, ManaRegen)

	// --- Combat (Greystone Statistics 시트 기준) ---

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalPower, Category = "Attributes|Combat")
	FGameplayAttributeData PhysicalPower;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, PhysicalPower)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicalPower, Category = "Attributes|Combat")
	FGameplayAttributeData MagicalPower;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, MagicalPower)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "Attributes|Combat")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, AttackSpeed)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BasicAttackTime, Category = "Attributes|Combat")
	FGameplayAttributeData BasicAttackTime;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, BasicAttackTime)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackRange, Category = "Attributes|Combat")
	FGameplayAttributeData AttackRange;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, AttackRange)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Cleave, Category = "Attributes|Combat")
	FGameplayAttributeData Cleave;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, Cleave)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalArmor, Category = "Attributes|Combat")
	FGameplayAttributeData PhysicalArmor;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, PhysicalArmor)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicalArmor, Category = "Attributes|Combat")
	FGameplayAttributeData MagicalArmor;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, MagicalArmor)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalPenetration, Category = "Attributes|Combat")
	FGameplayAttributeData PhysicalPenetration;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, PhysicalPenetration)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicalPenetration, Category = "Attributes|Combat")
	FGameplayAttributeData MagicalPenetration;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, MagicalPenetration)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_LifeSteal, Category = "Attributes|Combat")
	FGameplayAttributeData LifeSteal;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, LifeSteal)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Tenacity, Category = "Attributes|Combat")
	FGameplayAttributeData Tenacity;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, Tenacity)

	// 0.0~1.0 비율 — 방어력(Armor/(Armor+100) 감산)과는 별개의 최종 승산 레이어. PhysicalArmor/
	// MagicalArmor 감산까지 끝난 데미지에 (1-DamageReduction)을 곱한다(P1ExecCalc_Damage 참고).
	// 방어력처럼 관통(Penetration)으로 무효화되지 않는 순수 최종 감쇄 — 짧은 지속시간 버프(예: 사면(아이템)
	// "용기")로 주로 쓰일 걸 염두에 두고 설계, 상시 스탯으로도 문제없이 동작한다.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageReduction, Category = "Attributes|Combat")
	FGameplayAttributeData DamageReduction;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, DamageReduction)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AbilityHaste, Category = "Attributes|Combat")
	FGameplayAttributeData AbilityHaste;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, AbilityHaste)

	// AbilityHaste와 별개로 "궁극기(R)에만" 추가로 붙는 가속치 — 최종 R 쿨다운 감소율은
	// UP1GameplayAbility::ApplyCooldown()이 (AbilityHaste + UltimateHaste)를 합산해 계산한다
	// (LoL식 공식: 감소율 = Haste/(Haste+100)). Q/E/RMB 등 나머지 어빌리티는 AbilityHaste만 적용.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_UltimateHaste, Category = "Attributes|Combat")
	FGameplayAttributeData UltimateHaste;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, UltimateHaste)

	// 궁극기(R) 데미지 배율 보너스 — 0.15면 R 데미지 +15%. R 어빌리티(StoneForgedSoul/IonStrike)가
	// 캐스트 시점에 직접 읽어 ApplyDamageToTarget의 DamageMultiplier 인자로 (1.0+이 값)을 넘긴다.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_UltimateDamagePercent, Category = "Attributes|Combat")
	FGameplayAttributeData UltimateDamagePercent;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, UltimateDamagePercent)

	// 0.0~1.0 비율(0%~100%) — AttackSpeed 등과 달리 1.0을 넘을 일이 없어 [0,1]로 클램프한다.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalChance, Category = "Attributes|Combat")
	FGameplayAttributeData CriticalChance;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, CriticalChance)

	// 치명타 적중 시 데미지 배율(1.5 = 150%). 일반 공격보다 약해질 수 없으므로 1.0 미만으로 안 내려간다.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalDamage, Category = "Attributes|Combat")
	FGameplayAttributeData CriticalDamage;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, CriticalDamage)

	// --- Movement ---

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MovementSpeed, Category = "Attributes|Movement")
	FGameplayAttributeData MovementSpeed;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, MovementSpeed)

	// --- Progression (레벨업/킬·어시스트 보상) ---

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Gold, Category = "Attributes|Progression")
	FGameplayAttributeData Gold;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, Gold)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Experience, Category = "Attributes|Progression")
	FGameplayAttributeData Experience;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, Experience)

	// --- Meta (transient, 비복제) ---
	// ExecCalc_Damage가 최종 데미지를 여기에 누적하고, PostGameplayEffectExecute에서 Health로 변환한다.
	// 서버에서만 계산되므로 복제하지 않는다.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, Damage)

	// UP1ExecCalc_Damage가 크리티컬 판정 결과(0.0/1.0)를 여기 출력(Override)한다 — 크리티컬 여부는
	// ExecCalc 내부에서 RNG로 결정되므로 어빌리티가 스펙을 만드는 시점엔 알 수 없고, Damage 메타
	// 어트리뷰트와 동일하게 실행 결과를 나중에 PostGameplayEffectExecute에서 읽고 즉시 리셋하는
	// "메타 출력값" 패턴으로 전달한다(온-히트 크리티컬 반응 아이템 등이 이 값을 필요로 함, 예:
	// 애쉬브링어의 크로노 스트라이크와 달리 크리티컬 시 스택을 더 얻는 더스트 데빌의 "매너스").
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData CriticalHitFlag;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, CriticalHitFlag)

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_Mana(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_MaxMana(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_HealthRegen(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_ManaRegen(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_PhysicalPower(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_MagicalPower(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_BasicAttackTime(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_AttackRange(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_Cleave(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_PhysicalArmor(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_MagicalArmor(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_PhysicalPenetration(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_MagicalPenetration(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_LifeSteal(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_Tenacity(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_DamageReduction(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_AbilityHaste(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_UltimateHaste(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_UltimateDamagePercent(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_CriticalChance(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_CriticalDamage(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_MovementSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_Gold(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_Experience(const FGameplayAttributeData& OldValue);

private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	// 최근 이 캐릭터에게 데미지를 입힌 적 플레이어스테이트 → 마지막 피격 시각(초, GetTimeSeconds 기준).
	// 어시스트 판정용 서버 전용 북키핑 — TWeakObjectPtr라 GC 안전하고 UPROPERTY/복제가 필요 없다.
	TMap<TWeakObjectPtr<AP1PlayerState>, float> RecentDamageContributors;

	// 데미지가 실제로 적용될 때(무적/디플렉트로 무효화되지 않은 경우) 소스를 기록해 어시스트 윈도우를 갱신한다.
	void RecordDamageContribution(const FGameplayEffectModCallbackData& Data);

	// 가해자(Instigator)의 PlayerController에만 데미지 팝업 텍스트를 띄우도록 Client RPC를 보낸다
	// ("내가 입힌 데미지만 나에게 보인다" — LoL/Dota 컨벤션). 데미지가 실제로 적용된 경우에만 호출.
	void NotifyDamageDealt(const FGameplayEffectModCallbackData& Data, float DamageAmount);

	// 온-히트 아이템 어빌리티 범용 디스패치(2026-08-17 리팩터링) — 이 데미지가 기본 공격에서 왔으면
	// 가해자의 ASC에 Event.Character.BasicAttackHitDealt를 보낸다(Target=피격자, EventMagnitude=
	// 크리티컬 여부). 어떤 아이템이 이 이벤트에 반응하는지는 전혀 모른다 — 아이템 구매 시 부여되는
	// 각 아이템 전용 UP1GameplayAbility_OnHitItemAbility 파생 어빌리티가 이 이벤트를 구독해서 각자
	// 알아서 반응한다(크로노 스트라이크/매너스가 첫 사용처). 원래 이 자리에 아이템별 HandleXxxProc
	// 메서드를 직접 두었었는데, 아이템이 늘어날수록 AttributeSet이 계속 커지는 문제가 있어 이 훅
	// 하나로 일반화함 — 새 온-히트 아이템이 추가돼도 이 함수는 전혀 안 바뀐다.
	void DispatchBasicAttackHitDealt(const FGameplayEffectModCallbackData& Data, bool bWasCritical);

	// 사망 확정 시 호출 — 킬러 판별, 최근 10초 내 딜 넣은 플레이어 전원에게 어시스트 지급,
	// Kills/Deaths/Assists/KillStreak 갱신 후 골드+경험치 보상 GE를 킬러/어시스터 각자에게 적용한다.
	// 피해자가 AP1PlayerState를 못 가진 경우(=정글 몬스터)는 HandleMonsterKillRewards로 위임한다.
	void HandleKillRewards(const FGameplayEffectModCallbackData& Data);

	// 정글 몬스터 처치 보상 — Kills/Deaths/KillStreak/어시스트 등 플레이어간 KDA 집계는 전혀 관여하지
	// 않고, 킬러에게 몬스터가 계산한 골드/경험치만 그대로 지급한다(AP1JungleMonsterCharacter::GetKillReward 참고).
	void HandleMonsterKillRewards(const FGameplayEffectModCallbackData& Data, AActor* VictimOwner);
};

#undef ATTRIBUTE_ACCESSORS
