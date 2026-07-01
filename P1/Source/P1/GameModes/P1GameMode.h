// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "P1GameMode.generated.h"

// PreGame(로그인)을 제외한 모든 단계(Lobby, Arena 등)에서 공유되는 베이스.
// 단계별로 달라지는 설정(DefaultPawnClass, GameStateClass 등)은 서브클래스에서 지정한다.
UCLASS()
class P1_API AP1GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AP1GameMode();
};
