// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1MMC_GoldKillBounty.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Engine/CurveTable.h"
#include "GameplayEffect.h"

float UP1MMC_GoldKillBounty::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const float KillStreak = Spec.GetSetByCallerMagnitude(TAG_Data_KillStreak, false, 0.0f);
	const float TimeSinceLastDeath = Spec.GetSetByCallerMagnitude(TAG_Data_TimeSinceLastDeath, false, 0.0f);

	float StreakBonus = 0.0f;
	if (KillStreakBonusTable)
	{
		static const FString ContextString(TEXT("GoldKillBounty_KillStreak"));
		if (const FRealCurve* Curve = KillStreakBonusTable->FindCurve(FName(TEXT("KillStreakBonusGold")), ContextString, false))
		{
			StreakBonus = Curve->Eval(KillStreak);
		}
	}

	const float TimeAliveBonus = FMath::Min(TimeSinceLastDeath * GoldPerSecondAlive, MaxTimeAliveBonus);

	return BaseKillGold + StreakBonus + TimeAliveBonus;
}
