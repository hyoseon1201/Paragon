// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "P1LobbyPlayerController.generated.h"

class UP1PreGameHUDWidget;

// PreGame(로비) 레벨 전용 — 데디케이티드 서버 없는 로컬 세션이라 Enhanced Input/GAS 관련 로직이 전혀 없다.
// BeginPlay에서 로그인/회원가입/매칭 UI(UP1PreGameHUDWidget)를 띄우고 UI 입력 모드로 전환하는 게 전부.
UCLASS()
class P1_API AP1LobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	TSubclassOf<UP1PreGameHUDWidget> PreGameHUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UP1PreGameHUDWidget> PreGameHUDWidget;
};
