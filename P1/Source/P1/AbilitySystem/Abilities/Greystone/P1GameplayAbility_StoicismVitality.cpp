// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Greystone/P1GameplayAbility_StoicismVitality.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"

UP1GameplayAbility_StoicismVitality::UP1GameplayAbility_StoicismVitality()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_StoicismVitality);
	SetAssetTags(Tags);

	// 상시 백그라운드 패시브 — 다른 어빌리티 사용 여부와 무관하게 항상 돌아야 한다.
	ActivationBlockedTags.RemoveTag(TAG_State_Attacking);
	ActivationOwnedTags.RemoveTag(TAG_State_Attacking);

	// 순수 자기 자신 스탯 계산 — 서버 권위면 충분, 클라 예측 불필요.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 입력도 트리거 이벤트도 없는 상시 패시브라, 부여되는 즉시 스스로 활성화되어야 한다
	// (AP1HeroCharacter::AddDefaultAbilities가 이 플래그를 보고 GiveAbilityAndActivateOnce로 부여).
	bActivateOnGranted = true;
}

void UP1GameplayAbility_StoicismVitality::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogP1, Log, TEXT("[StoicismVitality] 활성화됨 — %.1f초 간격 반복 시작 (Auth=%d)"),
		TickPeriod, ActorInfo->IsNetAuthority() ? 1 : 0);

	// 코스트/쿨다운 없는 상시 패시브라 CommitAbility가 필요 없다 — 캐릭터 생존 내내 반복.
	GetWorld()->GetTimerManager().SetTimer(VitalityTickTimerHandle, this,
		&UP1GameplayAbility_StoicismVitality::OnVitalityTick, TickPeriod, true);

	// 스폰 직후부터 바로 적용되도록 첫 틱을 기다리지 않고 즉시 1회 실행.
	OnVitalityTick();
}

void UP1GameplayAbility_StoicismVitality::OnVitalityTick()
{
	if (!CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UP1AttributeSet* AttrSet = ASC ? ASC->GetSet<UP1AttributeSet>() : nullptr;
	if (!AttrSet)
	{
		return;
	}

	const float MaxHealth = AttrSet->GetMaxHealth();
	const float CurrentHealth = AttrSet->GetHealth();
	if (MaxHealth <= 0.0f)
	{
		return;
	}

	// "값들이 잃은 체력 1%당 1%씩 증가" = 만피면 1배, 완전 빈사면 2배.
	const float MissingRatio = FMath::Clamp((MaxHealth - CurrentHealth) / MaxHealth, 0.0f, 1.0f);
	const float ValueMultiplier = 1.0f + MissingRatio;
	const bool bLowHealth = (CurrentHealth / MaxHealth) < LowHealthThreshold;
	const float RegenMultiplier = bLowHealth ? 2.0f : 1.0f;

	float HealThisTick = 0.0f;
	if (HealEffectClass)
	{
		const float RegenPerSecond = MaxHealth * RegenPercentPerSecond * ValueMultiplier * RegenMultiplier;
		HealThisTick = RegenPerSecond * TickPeriod;
		if (HealThisTick > 0.0f)
		{
			ApplyEffectToSelf(HealEffectClass, TAG_Data_Heal_Flat, HealThisTick);
		}
	}

	UE_LOG(LogP1, Log, TEXT("[StoicismVitality] Tick — Health=%.1f/%.1f MissingRatio=%.2f ValueMult=%.2f LowHealth=%d HealThisTick=%.2f"),
		CurrentHealth, MaxHealth, MissingRatio, ValueMultiplier, bLowHealth ? 1 : 0, HealThisTick);

	if (ArmorBuffEffectClass)
	{
		const float ArmorBonus = (BaseArmorBonus + ArmorBonusPerLevel * (GetAbilityLevel() - 1)) * ValueMultiplier;
		ApplyEffectToSelf(ArmorBuffEffectClass, TAG_Data_ArmorBonus_Flat, ArmorBonus);
	}
}
