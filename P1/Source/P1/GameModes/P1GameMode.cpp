// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/P1GameMode.h"
#include "Player/P1PlayerState.h"
#include "Player/P1PlayerController.h"

AP1GameMode::AP1GameMode()
{
	PlayerStateClass = AP1PlayerState::StaticClass();
	PlayerControllerClass = AP1PlayerController::StaticClass();
}
