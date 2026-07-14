// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/P1LobbyGameMode.h"
#include "Player/P1LobbyPlayerController.h"
#include "GameFramework/PlayerState.h"

AP1LobbyGameMode::AP1LobbyGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerStateClass = APlayerState::StaticClass();
	PlayerControllerClass = AP1LobbyPlayerController::StaticClass();
}
