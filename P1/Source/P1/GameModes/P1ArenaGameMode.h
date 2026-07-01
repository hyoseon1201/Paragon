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

	// 팀 미배정(255) 플레이어에게 교대로 Team0/Team1 배정 후, 해당 태그의 PlayerStart 반환.
	// ChoosePlayerStart가 PostLogin보다 먼저 호출되므로 팀 배정도 여기서 수행.
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	// 다음 플레이어에게 배정할 팀 인덱스 (0→1→0→1 순환).
	int32 NextTeamIndex = 0;
};
