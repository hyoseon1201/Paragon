// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/P1GameMode.h"
#include "P1LobbyGameMode.generated.h"

// PreGame(로비) 레벨 전용 GameMode — 데디케이티드 서버가 없는 완전 로컬/싱글플레이 레벨이다.
// 로그인/회원가입/매칭 대기열은 전부 UP1BackendSubsystem이 웹 백엔드와 직접 통신해서 처리하고,
// 매칭이 성사되면 ClientTravel로 이미 떠 있는 Arena 데디케이티드 서버에 접속한다 — 그러므로 이 GameMode는
// 스폰할 Pawn도, GAS/ASC를 가진 PlayerState도 필요 없다(베이스 AP1GameMode가 기본으로 물려주는
// AP1PlayerState는 로비에서 쓸모없으므로 여기서 plain APlayerState로 되돌린다).
UCLASS()
class P1_API AP1LobbyGameMode : public AP1GameMode
{
	GENERATED_BODY()

public:
	AP1LobbyGameMode();
};
