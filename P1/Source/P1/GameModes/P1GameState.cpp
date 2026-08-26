// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/P1GameState.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "P1.h"

void AP1GameState::SetMatchStartTime()
{
	if (HasAuthority())
	{
		MatchStartServerTime = GetServerWorldTimeSeconds();
		MARK_PROPERTY_DIRTY_FROM_NAME(AP1GameState, MatchStartServerTime, this);
	}
}

void AP1GameState::InitializeTeamScores(int32 NumTeams)
{
	if (!HasAuthority())
	{
		return;
	}

	TeamKillScores.Init(0, FMath::Max(0, NumTeams));
	MARK_PROPERTY_DIRTY_FROM_NAME(AP1GameState, TeamKillScores, this);
}

void AP1GameState::AddTeamKill(int32 TeamId)
{
	if (!HasAuthority() || !TeamKillScores.IsValidIndex(TeamId))
	{
		return;
	}

	++TeamKillScores[TeamId];
	MARK_PROPERTY_DIRTY_FROM_NAME(AP1GameState, TeamKillScores, this);
}

void AP1GameState::SetMatchEnded(int32 InWinningTeamId)
{
	if (!HasAuthority())
	{
		return;
	}

	MatchState = EP1MatchState::Ended;
	WinningTeamId = InWinningTeamId;
	MARK_PROPERTY_DIRTY_FROM_NAME(AP1GameState, MatchState, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AP1GameState, WinningTeamId, this);
}

void AP1GameState::SetMatchInProgress()
{
	if (!HasAuthority())
	{
		return;
	}

	MatchState = EP1MatchState::InProgress;
	MARK_PROPERTY_DIRTY_FROM_NAME(AP1GameState, MatchState, this);
}

void AP1GameState::OnRep_MatchState()
{
	UE_LOG(LogP1, Log, TEXT("[GameState] OnRep_MatchState — MatchState=%d WinningTeamId=%d"),
		static_cast<int32>(MatchState), WinningTeamId);
}

void AP1GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AP1GameState, MatchStartServerTime, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1GameState, TeamKillScores, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1GameState, MatchState, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1GameState, WinningTeamId, SharedParams);
}
