// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"
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
	InitMagicalPower(0.0f);
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
	DOREPLIFETIME_CONDITION_NOTIFY(UP1AttributeSet, MagicalPower, COND_None, REPNOTIFY_Always);
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

	if (Attr == GetDamageAttribute())
	{
		// 메타 Damage → Health 변환. ExecCalc가 방어/관통까지 반영한 최종값을 누적해둔다.
		const float LocalDamage = GetDamage();
		SetDamage(0.0f);

		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();

		// 무적 상태(State.Invulnerable)면 데미지를 완전히 무시 — 스테이시스류 스킬이 공용으로 사용.
		const bool bInvulnerable = ASC && ASC->HasMatchingGameplayTag(TAG_State_Invulnerable);

		// Stoicism 디플렉트: 이 데미지가 기본공격에서 왔고(발생시킨 어빌리티의 Asset Tag가 스펙에 실려있음,
		// UP1DamageGameplayAbility::ApplyDamageToTarget 참고), 이 캐릭터가 디플렉트를 갖고 있으며
		// (루즈 태그로 존재 여부 판별) 쿨다운 중이 아니면(네이티브 쿨다운 태그 부재=사용 가능) 무효화.
		// 실제 쿨다운 커밋은 어빌리티에게 이벤트로 위임 — AttributeSet은 어빌리티 클래스를 몰라도 된다.
		bool bDeflected = false;
		if (ASC && !bInvulnerable)
		{
			const FGameplayTagContainer* SourceTags = Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
			const bool bFromBasicAttack = SourceTags && SourceTags->HasTag(TAG_Ability_BasicAttack);
			const bool bHasStoicismDeflect = ASC->HasMatchingGameplayTag(TAG_Ability_StoicismDeflect);
			const bool bDeflectOnCooldown = ASC->HasMatchingGameplayTag(TAG_Cooldown_Ability_StoicismDeflect);
			if (bFromBasicAttack && bHasStoicismDeflect && !bDeflectOnCooldown)
			{
				bDeflected = true;
				UE_LOG(LogP1, Log, TEXT("[AttributeSet] Stoicism 디플렉트 발동 — 기본공격 %.2f 무효화 (Owner=%s)"),
					LocalDamage, GetOwningActor() ? *GetOwningActor()->GetName() : TEXT("null"));

				FGameplayEventData EventData;
				EventData.EventTag = TAG_Event_StoicismDeflect_Consumed;
				ASC->HandleGameplayEvent(TAG_Event_StoicismDeflect_Consumed, &EventData);
			}
		}

		if (LocalDamage > 0.0f && !bInvulnerable && !bDeflected)
		{
			const float NewHealth = FMath::Clamp(GetHealth() - LocalDamage, 0.0f, GetMaxHealth());
			UE_LOG(LogP1, Log, TEXT("[AttributeSet] Damage %.2f applied: Health %.2f → %.2f"),
				LocalDamage, GetHealth(), NewHealth);
			SetHealth(NewHealth);
			// TODO: 사망 처리 / 히트리액트 이벤트 트리거
		}
	}
	else if (Attr == GetHealthAttribute())
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

void UP1AttributeSet::OnRep_MagicalPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MagicalPower, OldValue);
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
