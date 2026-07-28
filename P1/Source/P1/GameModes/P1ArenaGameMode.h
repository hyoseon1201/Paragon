// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/P1GameMode.h"
#include "P1ArenaGameMode.generated.h"

// 실제 콜로세움 아레나 매치(InGame) 전용 GameMode.
UCLASS()
class P1_API AP1ArenaGameMode : public AP1GameMode
{
	GENERATED_BODY()

public:
	AP1ArenaGameMode();

protected:
	// TODO: Login() 오버라이드에서 URL Options 파싱 → PC->SelectedCharacterClass 설정.
	//       클라이언트가 로비(웹서버)에서 선택한 캐릭터 데이터를 접속 시 URL로 전달받는 시점.
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	// 팀 미배정(255) 플레이어에게 교대로 Team0~Team(NumTeams-1) 배정 후, 해당 태그의 PlayerStart 중
	// 하나를 랜덤으로 반환(같은 팀 여러 명이 전부 같은 자리 하나에 겹쳐 스폰되지 않도록 — 팀당 여러
	// PlayerStart를 배치해뒀다는 전제). ChoosePlayerStart가 PostLogin보다 먼저 호출되므로 팀 배정도
	// 여기서 수행.
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	// 팀 수 — 레벨에 배치하는 PlayerStart의 PlayerStartTag("Team0".."Team{NumTeams-1}")도 이 값과
	// 일치해야 한다(팀당 인원이 여러 명이면 같은 태그의 PlayerStart를 그만큼 여러 개 배치할 것).
	UPROPERTY(EditDefaultsOnly, Category = "Teams")
	int32 NumTeams = 3;

private:
	// 다음 플레이어에게 배정할 팀 인덱스 (0→1→...→NumTeams-1→0 순환).
	int32 NextTeamIndex = 0;
};
