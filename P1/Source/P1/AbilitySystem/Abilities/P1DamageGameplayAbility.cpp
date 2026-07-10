// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Characters/P1CharacterBase.h"
#include "Engine/OverlapResult.h"

void UP1DamageGameplayAbility::ApplyDamageToTarget(AActor* TargetActor, float DamageMultiplier,
	float BonusFlat, float BonusPhysicalPowerCoeff, float BonusMagicalPowerCoeff,
	float BonusTargetMaxHealthPctCoeff, float BonusSourceMaxHealthPctCoeff)
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
		// 이 데미지를 발생시킨 어빌리티의 Asset Tags(예: Ability.BasicAttack)를 스펙에 실어 보낸다.
		// 이걸 안 하면(raw ASC MakeOutgoingSpec은 자동으로 안 실어줌) "이 데미지가 기본공격에서
		// 왔는지" 같은 판별이 타겟 쪽(AttributeSet 등)에서 불가능해진다 — Stoicism 디플렉트 등에 필요.
		SpecHandle.Data->CapturedSourceTags.GetSpecTags().AppendTags(GetAssetTags());

		// ExecCalc_Damage가 이 채널들을 읽어 최종 데미지를 산출한다. GE가 SetByCaller를 참조하지 않으면 무시됨.
		// Bonus 파라미터는 클래스 기본 계수 위에 이번 호출 한정으로 얹는다 (기본 0 = 평소와 동일).
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage_Flat, FlatDamage.GetValueAtLevel(GetAbilityLevel()) + BonusFlat);
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage_PhysicalPower, PhysicalPowerCoefficient + BonusPhysicalPowerCoeff);
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage_MagicalPower, MagicalPowerCoefficient + BonusMagicalPowerCoeff);
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage_TargetMaxHealthPct, TargetMaxHealthPctCoefficient + BonusTargetMaxHealthPctCoeff);
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage_SourceMaxHealthPct, SourceMaxHealthPctCoefficient + BonusSourceMaxHealthPctCoeff);
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

TArray<AActor*> UP1DamageGameplayAbility::GetEnemiesInRadius(const FVector& Center, float Radius, float HalfHeight) const
{
	TArray<AActor*> EnemyTargets;

	AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
	if (!IsValid(SourceCharacter))
	{
		return EnemyTargets;
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(SourceCharacter);

	TArray<FOverlapResult> OverlapResults;
	GetWorld()->OverlapMultiByChannel(OverlapResults, Center, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params);

	for (const FOverlapResult& Result : OverlapResults)
	{
		// P1CharacterBase 계열만 대상으로 인정 — 지형/소품은 캐스트 실패로 자동 제외.
		AP1CharacterBase* HitCharacter = Cast<AP1CharacterBase>(Result.GetActor());
		if (!IsValid(HitCharacter))
		{
			continue;
		}

		if (AP1CharacterBase::IsSameTeam(SourceCharacter, HitCharacter))
		{
			continue;
		}

		if (FMath::Abs(HitCharacter->GetActorLocation().Z - Center.Z) > HalfHeight)
		{
			continue;
		}

		EnemyTargets.Add(HitCharacter);
	}

	return EnemyTargets;
}
