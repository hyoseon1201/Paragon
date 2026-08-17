// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Items/P1GameplayAbility_Item_TrueStrike.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/P1CharacterBase.h"
#include "GameplayEffect.h"
#include "P1.h"

UP1GameplayAbility_Item_TrueStrike::UP1GameplayAbility_Item_TrueStrike()
{
	// "소모" 부분의 추가 데미지는 방어력을 무시한다(고정 피해) — 순수 스탯 계수, Flat/Magical 등은 기본값(0) 그대로.
	bIsTrueDamage = true;
	PhysicalPowerCoefficient = 0.90f;
}

void UP1GameplayAbility_Item_TrueStrike::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// 서버 쪽 ASC 인스턴스에만 구독 — RocketBoots::OnGiveAbility와 동일한 이유(쿨다운/버프 적용은
	// 서버 권위 로직이므로 클라 예측 인스턴스에서 중복 구독할 필요가 없음).
	if (ActorInfo && ActorInfo->IsNetAuthority() && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->AbilityActivatedCallbacks.AddUObject(
			this, &UP1GameplayAbility_Item_TrueStrike::OnAnyAbilityActivated);
	}
}

void UP1GameplayAbility_Item_TrueStrike::OnAnyAbilityActivated(UGameplayAbility* ActivatedAbility)
{
	if (ActivatedAbility == this)
	{
		return;
	}

	// Q/E/RMB/R("스킬")만 트리거로 인정 — 기본공격/패시브/InputTag가 없는 아이템 반응형 어빌리티는
	// InputTag가 이 넷 중 어느 것과도 안 맞아 자연히 걸러진다(별도 제외 목록이 필요 없음).
	const UP1GameplayAbility* P1Ability = Cast<UP1GameplayAbility>(ActivatedAbility);
	if (!P1Ability)
	{
		return;
	}

	const FGameplayTag& ActivatedInputTag = P1Ability->InputTag;
	const bool bIsQualifyingSpell = ActivatedInputTag.MatchesTagExact(TAG_InputTag_Ability_Q)
		|| ActivatedInputTag.MatchesTagExact(TAG_InputTag_Ability_E)
		|| ActivatedInputTag.MatchesTagExact(TAG_InputTag_Ability_RMB)
		|| ActivatedInputTag.MatchesTagExact(TAG_InputTag_Ability_R);
	if (!bIsQualifyingSpell)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// 자기 자신(진실의 일격)이 지금 쿨다운 중이면 무시 — TryActivateAbility를 거치지 않는 순수
	// 사이드이펙트 경로라 엔진의 CanActivateAbility() 자동 게이팅을 못 받으므로 여기서 수동 확인한다.
	if (const FGameplayTagContainer* MyCooldownTags = GetCooldownTags())
	{
		if (!MyCooldownTags->IsEmpty() && ASC->HasAnyMatchingGameplayTags(*MyCooldownTags))
		{
			return;
		}
	}

	if (EmpowerBuffEffectClass)
	{
		ApplyEffectToSelf(EmpowerBuffEffectClass);
	}

	if (AP1CharacterBase* Character = GetP1CharacterFromActorInfo())
	{
		Character->MulticastPlayNiagaraEffect(EmpowerEffect, EmpowerEffectSocketName);
	}

	if (const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect())
	{
		ApplyEffectToSelf(CooldownGE->GetClass());
	}

	UE_LOG(LogP1, Log, TEXT("[TrueStrike] %s 사용 감지 — Empowered 버프 부여 (%s)"),
		*ActivatedInputTag.ToString(), GetAvatarActorFromActorInfo() ? *GetAvatarActorFromActorInfo()->GetName() : TEXT("null"));
}

void UP1GameplayAbility_Item_TrueStrike::OnBasicAttackHitDealt(AActor* Target, bool bWasCritical)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->HasMatchingGameplayTag(TAG_Buff_TrueStrike_Empowered))
	{
		return; // 강화 대기 상태가 아니면 조용히 무시 — 매 기본공격마다 호출되는 게 정상.
	}

	ApplyDamageToTarget(Target, 1.0f);

	if (AP1CharacterBase* Character = GetP1CharacterFromActorInfo())
	{
		Character->MulticastPlayNiagaraEffect(ConsumeEffect, ConsumeEffectSocketName);
	}

	// 소모 — 다음 기본공격에 또 발동하지 않도록 즉시 제거.
	ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TAG_Buff_TrueStrike_Empowered));

	UE_LOG(LogP1, Log, TEXT("[TrueStrike] 강화된 기본공격 적중 — %s (크리티컬=%d)"),
		Target ? *Target->GetName() : TEXT("null"), bWasCritical);
}
