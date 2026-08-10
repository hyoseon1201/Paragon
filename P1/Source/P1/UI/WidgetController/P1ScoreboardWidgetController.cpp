// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/WidgetController/P1ScoreboardWidgetController.h"
#include "GameModes/P1GameState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AP1GameState* UP1ScoreboardWidgetController::GetP1GameState() const
{
	return (PlayerController && PlayerController->GetWorld())
		? PlayerController->GetWorld()->GetGameState<AP1GameState>()
		: nullptr;
}
