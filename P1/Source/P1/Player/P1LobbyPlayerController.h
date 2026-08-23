// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "P1LobbyPlayerController.generated.h"

class UP1PreGameHUDWidget;
class UP1BotLobbyComponent;

// PreGame(로비) 레벨 전용 — 데디케이티드 서버 없는 로컬 세션이라 Enhanced Input/GAS 관련 로직이 전혀 없다.
// BeginPlay에서 로그인/회원가입/매칭 UI(UP1PreGameHUDWidget)를 띄우고 UI 입력 모드로 전환하는 게 전부.
UCLASS()
class P1_API AP1LobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AP1LobbyPlayerController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	TSubclassOf<UP1PreGameHUDWidget> PreGameHUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UP1PreGameHUDWidget> PreGameHUDWidget;

	// 부하테스트용 헤드리스 봇 전용 — 실제 로그인/픽/매칭 로직은 전부 이 컴포넌트 안에 있다(이 클래스는
	// 생성자에서 붙이기만 함). 커맨드라인에 "-BotId=N"이 없으면 컴포넌트가 스스로 아무 일도 안 하므로
	// 일반 플레이어에게는 무관 — 자세한 내용은 P1BotLobbyComponent.h 참고.
	UPROPERTY(VisibleAnywhere, Category = "Bot")
	TObjectPtr<UP1BotLobbyComponent> BotLobbyComponent;
};
