// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/P1LobbyPlayerController.h"
#include "UI/Widget/P1LobbyWidget.h"
#include "Blueprint/UserWidget.h"
#include "P1.h"

void AP1LobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!LobbyWidgetClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[Lobby] LobbyWidgetClass가 설정되지 않았습니다 — BP_P1LobbyPlayerController의 Details를 확인하세요."));
		return;
	}

	LobbyWidget = CreateWidget<UP1LobbyWidget>(this, LobbyWidgetClass);
	if (LobbyWidget)
	{
		LobbyWidget->AddToViewport();
	}

	FInputModeUIOnly InputMode;
	if (LobbyWidget)
	{
		InputMode.SetWidgetToFocus(LobbyWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}
