// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/WidgetController/P1WidgetController.h"
#include "Player/P1PlayerState.h"

void UP1WidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WCParams)
{
	PlayerController = WCParams.PlayerController;
	PlayerState = WCParams.PlayerState;
	AbilitySystemComponent = WCParams.AbilitySystemComponent;
	AttributeSet = WCParams.AttributeSet;
}

AP1PlayerState* UP1WidgetController::GetP1PlayerState() const
{
	return Cast<AP1PlayerState>(PlayerState);
}
