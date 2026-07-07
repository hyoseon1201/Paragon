// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/P1BuffCosmeticEffectComponent.h"
#include "Characters/P1CharacterBase.h"
#include "AbilitySystemComponent.h"
#include "P1.h"

void UP1BuffCosmeticEffectComponent::BindToAbilitySystemComponent(UAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC))
	{
		return;
	}

	// InitAbilityActorInfo가 PossessedBy/OnRep_PlayerState 양쪽에서 호출될 수 있으므로,
	// 기존 구독이 있다면 먼저 정리하고 다시 등록한다.
	UnbindFromAbilitySystemComponent();

	CachedASC = ASC;

	for (const FP1BuffCosmeticEffectEntry& Entry : Effects)
	{
		if (!Entry.BuffTag.IsValid())
		{
			continue;
		}

		const FDelegateHandle Handle = ASC->RegisterGameplayTagEvent(Entry.BuffTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UP1BuffCosmeticEffectComponent::OnBuffTagChanged, Entry);
		TagDelegateHandles.Add(TPair<FGameplayTag, FDelegateHandle>(Entry.BuffTag, Handle));
	}
}

void UP1BuffCosmeticEffectComponent::UnbindFromAbilitySystemComponent()
{
	if (IsValid(CachedASC))
	{
		for (const TPair<FGameplayTag, FDelegateHandle>& Pair : TagDelegateHandles)
		{
			CachedASC->RegisterGameplayTagEvent(Pair.Key).Remove(Pair.Value);
		}
	}

	TagDelegateHandles.Reset();
	CachedASC = nullptr;
}

void UP1BuffCosmeticEffectComponent::OnBuffTagChanged(const FGameplayTag Tag, int32 NewCount, FP1BuffCosmeticEffectEntry Entry)
{
	AP1CharacterBase* Character = GetPawn<AP1CharacterBase>();
	if (!Character || !Character->HasAuthority())
	{
		// Multicast RPC는 권위 인스턴스에서 호출해야 복제되므로 서버에서만 반응한다.
		return;
	}

	const bool bActive = NewCount > 0;
	UE_LOG(LogP1, Log, TEXT("[BuffCosmeticEffect] %s | Tag=%s NewCount=%d bActive=%d"),
		*Character->GetName(), *Tag.ToString(), NewCount, bActive ? 1 : 0);

	if (!Entry.MaterialSlotName.IsNone())
	{
		Character->MulticastSetMaterialOverride(Entry.MaterialSlotName, bActive ? Entry.OverrideMaterial : nullptr);
	}

	if (Entry.AttachedParticleTemplate)
	{
		if (bActive)
		{
			Character->MulticastSetAttachedParticleEffect(Entry.AttachedParticleTemplate, Entry.ParticleSocketName);
		}
		else
		{
			Character->MulticastStopAttachedParticleEffect();
		}
	}
}
