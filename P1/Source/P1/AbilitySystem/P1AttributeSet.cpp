// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1AttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "P1.h"

UP1AttributeSet::UP1AttributeSet()
{
	InitHealth(680.0f);
	InitMaxHealth(680.0f);
	InitMana(280.0f);
	InitMaxMana(280.0f);
	InitHealthRegen(1.7f);
	InitManaRegen(1.2f);

	InitPhysicalPower(68.0f);
	InitAttackSpeed(1.0f);
	InitBasicAttackTime(1.1f);
	InitAttackRange(275.0f);
	InitCleave(0.2f);
	InitPhysicalArmor(36.0f);
	InitMagicalArmor(30.0f);
	InitPhysicalPenetration(0.0f);
	InitMagicalPenetration(0.0f);
	InitLifeSteal(0.0f);
	InitTenacity(0.0f);
	InitAbilityHaste(0.0f);

	InitMovementSpeed(720.0f);
}

void UP1AttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, HealthRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, ManaRegen, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, PhysicalPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, BasicAttackTime, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, AttackRange, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, Cleave, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, PhysicalArmor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, MagicalArmor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, PhysicalPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, MagicalPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, LifeSteal, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, Tenacity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, AbilityHaste, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, MovementSpeed, COND_None, REPNOTIFY_Always);
}

void UP1AttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetMovementSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UP1AttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	UE_LOG(LogP1, Log, TEXT("[AttributeSet] PreAttributeChange: %s → %.2f (Owner=%s)"),
		*Attribute.GetName(), NewValue,
		GetOwningActor() ? *GetOwningActor()->GetName() : TEXT("null"));

	ClampAttribute(Attribute, NewValue);
}

void UP1AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& Attr = Data.EvaluatedData.Attribute;
	const float Magnitude = Data.EvaluatedData.Magnitude;
	UE_LOG(LogP1, Log, TEXT("[AttributeSet] PostGameplayEffectExecute: %s | Magnitude=%.2f | Owner=%s"),
		*Attr.GetName(), Magnitude,
		GetOwningActor() ? *GetOwningActor()->GetName() : TEXT("null"));

	if (Attr == GetHealthAttribute())
	{
		const float OldHealth = GetHealth();
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		UE_LOG(LogP1, Log, TEXT("[AttributeSet] Health clamped: %.2f → %.2f (Max=%.2f)"),
			OldHealth, GetHealth(), GetMaxHealth());
	}
	else if (Attr == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
	}
}

void UP1AttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, Health, OldValue);
}

void UP1AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MaxHealth, OldValue);
}

void UP1AttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, Mana, OldValue);
}

void UP1AttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MaxMana, OldValue);
}

void UP1AttributeSet::OnRep_HealthRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, HealthRegen, OldValue);
}

void UP1AttributeSet::OnRep_ManaRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, ManaRegen, OldValue);
}

void UP1AttributeSet::OnRep_PhysicalPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, PhysicalPower, OldValue);
}

void UP1AttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, AttackSpeed, OldValue);
}

void UP1AttributeSet::OnRep_BasicAttackTime(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, BasicAttackTime, OldValue);
}

void UP1AttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, AttackRange, OldValue);
}

void UP1AttributeSet::OnRep_Cleave(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, Cleave, OldValue);
}

void UP1AttributeSet::OnRep_PhysicalArmor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, PhysicalArmor, OldValue);
}

void UP1AttributeSet::OnRep_MagicalArmor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MagicalArmor, OldValue);
}

void UP1AttributeSet::OnRep_PhysicalPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, PhysicalPenetration, OldValue);
}

void UP1AttributeSet::OnRep_MagicalPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MagicalPenetration, OldValue);
}

void UP1AttributeSet::OnRep_LifeSteal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, LifeSteal, OldValue);
}

void UP1AttributeSet::OnRep_Tenacity(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, Tenacity, OldValue);
}

void UP1AttributeSet::OnRep_AbilityHaste(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, AbilityHaste, OldValue);
}

void UP1AttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MovementSpeed, OldValue);
}
