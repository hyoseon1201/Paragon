// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "P1AttributeSet.generated.h"

class AP1PlayerState;

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

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AbilityHaste, Category = "Attributes|Combat")
	FGameplayAttributeData AbilityHaste;
	ATTRIBUTE_ACCESSORS(UP1AttributeSet, AbilityHaste)

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
	virtual void OnRep_AbilityHaste(const FGameplayAttributeData& OldValue);
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

	// 사망 확정 시 호출 — 킬러 판별, 최근 10초 내 딜 넣은 플레이어 전원에게 어시스트 지급,
	// Kills/Deaths/Assists/KillStreak 갱신 후 골드+경험치 보상 GE를 킬러/어시스터 각자에게 적용한다.
	void HandleKillRewards(const FGameplayEffectModCallbackData& Data);
};

#undef ATTRIBUTE_ACCESSORS
