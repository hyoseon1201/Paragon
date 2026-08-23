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
	// PreGame에서 선택한 히어로를 접속 URL의 "?HeroId=" 옵션으로 받아 HeroTable에서 조회 후
	// PC->SelectedCharacterClass에 채워넣는다(아래 InitNewPlayer 참고) — 그 값을 그대로 스폰 클래스로 쓴다.
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	// URL 옵션 "HeroId"를 파싱해 HeroTable에서 조회 → 성공 시 PC->SelectedCharacterClass에 대입.
	// HeroTable에 존재하는 HeroId만 유효한 값으로 인정되므로 이 테이블 자체가 화이트리스트 역할을
	// 겸한다(클라이언트가 URL을 조작해도 테이블에 없는 값이면 그냥 무시되고 기본 스폰 클래스로 폴백).
	// ChoosePlayerStart/RestartPlayer(및 그 안에서 호출되는 위 GetDefaultPawnClassForController)보다
	// 항상 먼저 실행되므로 타이밍 문제가 없다.
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
		const FString& Options, const FString& Portal = TEXT("")) override;

	// HeroId(FName) → 히어로 Pawn 클래스 매핑 테이블(FP1HeroDefinition, RowStruct). DT_ShopItems를
	// 서버/클라가 각자 독립 참조하는 것과 동일한 패턴으로, 이 픽 화면 위젯(UP1HeroPickerWidget)도
	// 같은 에셋을 표시용으로 별도 참조한다.
	UPROPERTY(EditDefaultsOnly, Category = "Heroes")
	TObjectPtr<UDataTable> HeroTable;

	// 팀 미배정(255) 플레이어에게 교대로 Team0~Team(NumTeams-1) 배정 후, 해당 태그의 PlayerStart 중
	// 하나를 랜덤으로 반환(같은 팀 여러 명이 전부 같은 자리 하나에 겹쳐 스폰되지 않도록 — 팀당 여러
	// PlayerStart를 배치해뒀다는 전제). ChoosePlayerStart가 PostLogin보다 먼저 호출되므로 팀 배정도
	// 여기서 수행.
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	// AP1GameMode가 AGameModeBase(정식 매치 상태머신 AGameMode가 아님)를 상속하므로 HandleMatchHasStarted()가
	// 없다 — GameState 초기화(팀 점수 배열 크기 등)만 여기서 하고, 매치 시작 시각 고정은 이제 BeginPlay가
	// 아니라 ExpectedPlayerCount명이 전부 접속한 시점(PostLogin, 아래 참고)으로 미뤄졌다(2026-08-22) —
	// 레벨 로드 즉시 시작하면 먼저 접속한 플레이어가 정글 캠프 레벨링 등에서 유리해지는 불공정이 있었음.
	virtual void BeginPlay() override;

	// 접속 인원이 ExpectedPlayerCount에 도달하면 StartMatch()를 호출해 매치 클럭을 0초부터 시작한다.
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 팀 수 — 레벨에 배치하는 PlayerStart의 PlayerStartTag("Team0".."Team{NumTeams-1}")도 이 값과
	// 일치해야 한다(팀당 인원이 여러 명이면 같은 태그의 PlayerStart를 그만큼 여러 개 배치할 것).
	UPROPERTY(EditDefaultsOnly, Category = "Teams")
	int32 NumTeams = 3;

public:
	// 이 인원이 전부 접속(PostLogin)해야 매치가 실제로 시작된다(GameState WaitingForPlayers→InProgress
	// 전환 + 매치 클럭 0초 고정). Backend/src/main/resources/application.yml의 match.required-players와
	// 반드시 같은 값으로 수동 동기화할 것 — 서버는 백엔드로부터 이 값을 직접 전달받지 않는다(둘 사이에
	// 매칭 성사 후의 직접 통신 채널이 없음, 각 클라이언트가 개별적으로 ClientTravel할 뿐).
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 ExpectedPlayerCount = 9;

	// 한 팀의 누적 킬스코어가 이 값에 도달하면 매치 종료.
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 KillScoreToWin = 30;

	// 결과창을 띄워두는 시간(초) — 이후 각 클라이언트가 자동으로 로비로 복귀한다.
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	float ResultScreenDurationSeconds = 6.0f;

	// 로비 복귀용 로컬 맵 경로 — 데디케이티드 서버가 없는 완전 로컬 레벨(AP1LobbyGameMode)이라
	// IP:Port 없이 순수 맵 경로만으로 ClientTravel한다. Scripts/Package_Client.bat이 실제로 쿡하는
	// 경로(/Game/Maps/PreGame+/Game/Maps/InGame — Arena는 더 이상 쓰지 않는 구 맵)와 반드시 일치시킬 것.
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	FString LobbyMapPath = TEXT("/Game/Maps/PreGame");

	// 킬 발생마다 UP1AttributeSet::HandleKillRewards()가 호출 — 해당 팀의 킬스코어를 1 올리고
	// 승리 임계치 도달 여부를 판정한다. 매치가 이미 종료됐으면 조용히 무시(막타 이후 잔여 데미지 등
	// 뒤늦게 들어오는 킬 이벤트 방어).
	void OnTeamKillScored(int32 TeamId);

private:
	// 다음 플레이어에게 배정할 팀 인덱스 (0→1→...→NumTeams-1→0 순환).
	int32 NextTeamIndex = 0;

	// GameState 상태를 "종료"로 갱신하고, 전 플레이어에게 결과창 표시 Client RPC를 보낸다.
	void EndMatch(int32 WinningTeamId);

	// GameState->PlayerArray.Num() >= ExpectedPlayerCount가 됐을 때 PostLogin이 호출 — 매치 클럭을
	// 0초로 고정하고 WaitingForPlayers→InProgress로 전환한다.
	void StartMatch();
};
