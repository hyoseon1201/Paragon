// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

void UP1DamageGameplayAbility::ApplyDamageToTarget(AActor* TargetActor, float DamageMultiplier)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	if (!DamageEffectClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[DamageAbility] DamageEffectClass가 설정되지 않았습니다. GA BP에서 Damage > Damage Effect Class를 지정해주세요."));
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		UE_LOG(LogP1, Warning, TEXT("[DamageAbility] SourceASC is null"));
		return;
	}

	IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetASC = TargetASI ? TargetASI->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC)
	{
		UE_LOG(LogP1, Warning, TEXT("[DamageAbility] TargetASC is null on %s — PlayerState 없이 배치된 캐릭터이거나 ASC가 초기화되지 않았습니다."), *TargetActor->GetName());
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), ContextHandle);
	if (SpecHandle.IsValid())
	{
		// ExecCalc_Damage가 이 값을 읽어 최종 데미지에 곱한다. GE가 SetByCaller를 참조하지 않으면 무시됨.
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_DamageMultiplier, DamageMultiplier);
		const FActiveGameplayEffectHandle ActiveHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		UE_LOG(LogP1, Log, TEXT("[DamageAbility] Applied %s to %s (Multiplier=%.2f) → ActiveHandle valid=%d"),
			*DamageEffectClass->GetName(), *TargetActor->GetName(), DamageMultiplier, ActiveHandle.IsValid() ? 1 : 0);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[DamageAbility] MakeOutgoingSpec 실패 — DamageEffectClass=%s"), *DamageEffectClass->GetName());
	}
}
