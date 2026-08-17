// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1MMC_StackCountMagnitude.h"
#include "GameplayEffect.h"

float UP1MMC_StackCountMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return MagnitudePerStack * static_cast<float>(Spec.GetStackCount());
}
