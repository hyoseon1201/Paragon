// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/P1GameState.h"
#include "Net/UnrealNetwork.h"

void AP1GameState::SetMatchStartTime()
{
	if (HasAuthority())
	{
		MatchStartServerTime = GetServerWorldTimeSeconds();
	}
}

void AP1GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AP1GameState, MatchStartServerTime);
}
