// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/P1GameState.h"
#include "Net/UnrealNetwork.h"
#include "P1.h"

void AP1GameState::SetMatchStartTime()
{
	if (HasAuthority())
	{
		MatchStartServerTime = GetServerWorldTimeSeconds();
	}
}

void AP1GameState::InitializeTeamScores(int32 NumTeams)
{
	if (!HasAuthority())
	{
		return;
	}

	TeamKillScores.Init(0, FMath::Max(0, NumTeams));
}

void AP1GameState::AddTeamKill(int32 TeamId)
{
	if (!HasAuthority() || !TeamKillScores.IsValidIndex(TeamId))
	{
		return;
	}

	++TeamKillScores[TeamId];
}

void AP1GameState::SetMatchEnded(int32 InWinningTeamId)
{
	if (!HasAuthority())
	{
		return;
	}

	MatchState = EP1MatchState::Ended;
	WinningTeamId = InWinningTeamId;
}

void AP1GameState::SetMatchInProgress()
{
	if (!HasAuthority())
	{
		return;
	}

	MatchState = EP1MatchState::InProgress;
}

void AP1GameState::OnRep_MatchState()
{
	UE_LOG(LogP1, Log, TEXT("[GameState] OnRep_MatchState — MatchState=%d WinningTeamId=%d"),
		static_cast<int32>(MatchState), WinningTeamId);
}

void AP1GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AP1GameState, MatchStartServerTime);
	DOREPLIFETIME(AP1GameState, TeamKillScores);
	DOREPLIFETIME(AP1GameState, MatchState);
	DOREPLIFETIME(AP1GameState, WinningTeamId);
}
