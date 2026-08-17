// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1MMC_StackThresholdMagnitude.h"
#include "GameplayEffect.h"

float UP1MMC_StackThresholdMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return Spec.GetStackCount() >= RequiredStackCount ? BonusMagnitude : BelowThresholdMagnitude;
}
