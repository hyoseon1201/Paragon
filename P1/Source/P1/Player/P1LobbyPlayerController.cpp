// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/P1LobbyPlayerController.h"
#include "UI/Widget/P1PreGameHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "P1.h"

void AP1LobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// PIE에서 클라이언트 2개 이상으로 테스트하면 서버 프로세스 안에 각 클라이언트의 PlayerController
	// 인스턴스가 전부 존재하고 BeginPlay()도 그만큼 실행된다 — 위젯은 로컬로 조작 중인 컨트롤러에만
	// 붙일 수 있으므로(AP1HeroCharacter::InitAbilityActorInfo의 IsLocallyControlled() 체크와 동일 패턴),
	// 로컬이 아닌 인스턴스에서는 위젯 생성을 건너뛴다.
	if (!IsLocalController())
	{
		return;
	}

	if (!PreGameHUDWidgetClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[Lobby] PreGameHUDWidgetClass가 설정되지 않았습니다 — BP_P1LobbyPlayerController의 Details를 확인하세요."));
		return;
	}

	PreGameHUDWidget = CreateWidget<UP1PreGameHUDWidget>(this, PreGameHUDWidgetClass);
	if (PreGameHUDWidget)
	{
		PreGameHUDWidget->AddToViewport();
	}

	FInputModeUIOnly InputMode;
	if (PreGameHUDWidget)
	{
		InputMode.SetWidgetToFocus(PreGameHUDWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}
