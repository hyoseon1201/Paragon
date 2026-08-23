// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/P1LobbyPlayerController.h"
#include "Player/P1BotLobbyComponent.h"
#include "UI/Widget/Auth/P1PreGameHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "Misc/CommandLine.h"
#include "P1.h"

AP1LobbyPlayerController::AP1LobbyPlayerController()
{
	BotLobbyComponent = CreateDefaultSubobject<UP1BotLobbyComponent>(TEXT("BotLobbyComponent"));
}

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

	// 부하테스트용 헤드리스 봇은 화면 자체가 없어(-nullrhi) UI가 필요 없다 — 실제 로그인/픽/매칭 로직은
	// BotLobbyComponent가 전담(위 생성자에서 이미 붙여둠). 이 한 줄이 이 클래스에 남는 유일한 봇 관련
	// 코드 — 이미 있는 커맨드라인 값을 한 번 더 확인해서 위젯 생성만 건너뛰는 가드일 뿐이다.
	// FParse::Param이 아니라 FParse::Value를 쓰는 이유: Param은 매치 뒤에 공백/문자열 끝이 와야
	// true라 "-BotId=5"처럼 "="가 바로 붙는 값 옵션은 인식 못 한다(컴포넌트 쪽 감지 방식과 동일하게 맞춤).
	int32 BotId = -1;
	if (FParse::Value(FCommandLine::Get(), TEXT("BotId="), BotId))
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
