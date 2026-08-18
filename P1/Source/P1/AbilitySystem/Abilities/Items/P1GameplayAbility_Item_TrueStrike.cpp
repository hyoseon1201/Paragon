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

		// 손 발광 이펙트의 시작/중지는 어빌리티 활성화 시점이 아니라 태그 그 자체를 구독해서 처리한다 —
		// "부여"와 "소모"뿐 아니라 4초 자연만료도 전부 이 태그의 0<->양수 전이로 귀결되므로, 여기 한
		// 곳에서만 반응하면 원인별로 따로 처리할 필요가 없다(Sacred Oath 검 발광과 동일한 이유).
		ActorInfo->AbilitySystemComponent->RegisterGameplayTagEvent(
			TAG_Buff_TrueStrike_Empowered, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UP1GameplayAbility_Item_TrueStrike::OnEmpoweredTagChanged);
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
		// 태그 부여는 이 호출 안에서 동기적으로 처리되므로, ApplyEffectToSelf가 반환할 때쯤엔
		// OnEmpoweredTagChanged(NewCount=1)이 이미 호출되어 손 발광 이펙트도 함께 시작돼 있다 —
		// 여기서 별도로 이펙트 재생을 호출할 필요가 없다.
		ApplyEffectToSelf(EmpowerBuffEffectClass);
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

void UP1GameplayAbility_Item_TrueStrike::OnEmpoweredTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	AP1CharacterBase* Character = GetP1CharacterFromActorInfo();
	if (!Character)
	{
		return;
	}

	// 양손 소켓 각각 독립적으로 시작/중지 — 소켓 이름이 실제로 지정된 쪽만 호출한다(둘 다 NAME_None이면
	// 둘 다 스킵). 한쪽만 채워도 안전: 여기서 걸러내지 않으면 빈 소켓 이름이 MulticastSetAttachedNiagaraEffect
	// 내부에서 "소켓 없음"으로 처리돼 재생 자체가 생략되므로 문제는 없지만, 애초에 빈 호출을 보내지 않는다.
	if (NewCount > 0)
	{
		if (!EmpowerEffectSocketNameLeftHand.IsNone())
		{
			Character->MulticastSetAttachedNiagaraEffect(EmpowerEffect, EmpowerEffectSocketNameLeftHand);
		}
		if (!EmpowerEffectSocketNameRightHand.IsNone())
		{
			Character->MulticastSetAttachedNiagaraEffect(EmpowerEffect, EmpowerEffectSocketNameRightHand);
		}
	}
	else
	{
		if (!EmpowerEffectSocketNameLeftHand.IsNone())
		{
			Character->MulticastStopAttachedNiagaraEffect(EmpowerEffectSocketNameLeftHand);
		}
		if (!EmpowerEffectSocketNameRightHand.IsNone())
		{
			Character->MulticastStopAttachedNiagaraEffect(EmpowerEffectSocketNameRightHand);
		}
	}
}
