// Copyright Epic Games, Inc. All Rights Reserved.

#include "P1GameInstance.h"
#include "AbilitySystemGlobals.h"

void UP1GameInstance::Init()
{
	Super::Init();

	// GAS 전역 초기화 — GameplayCue, TargetData, EffectContext 등이 올바르게 동작하려면 필수.
	UAbilitySystemGlobals::Get().InitGlobalData();
}
