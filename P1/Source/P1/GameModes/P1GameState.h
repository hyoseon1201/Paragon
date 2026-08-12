// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "P1GameState.generated.h"

// 매치 진행 상태 — 종료 판정 자체(임계치 체크)는 AP1ArenaGameMode가 하고, 여기는 그 결과를
// 복제해서 들고 있는 순수 데이터일 뿐이다(MatchStartServerTime과 같은 원칙).
UENUM(BlueprintType)
enum class EP1MatchState : uint8
{
	InProgress,
	Ended
};

UCLASS()
class P1_API AP1GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	// 서버 전용 — GameMode(AP1ArenaGameMode::HandleMatchHasStarted)가 매치가 실제로 시작되는 시점에
	// 딱 한 번 호출해 "경과 0초" 기준점을 고정한다.
	void SetMatchStartTime();

	// 매치 경과 시간(초). MatchStartServerTime이 아직 복제/설정되지 않았으면(-1) 0을 반환 — HUD가
	// 델리게이트 없이 NativeTick에서 매 프레임 직접 읽는 폴링 방식이라 이 폴백이 중요하다.
	// 센티널이 반드시 음수여야 하는 이유: 매치가 서버 월드 시간 0초 근처(레벨 시작 직후)에 시작되는
	// 경우가 실제로 흔해서, 0을 "미설정"으로 쓰면 정상적으로 설정된 0.00이 영원히 미설정으로
	// 오판된다(직접 겪은 버그 — MatchStartServerTime>0.0f 체크였을 때 ElapsedSeconds가 0에서 고정됨).
	float GetElapsedMatchTime() const
	{
		return MatchStartServerTime >= 0.0f ? FMath::Max(0.0f, GetServerWorldTimeSeconds() - MatchStartServerTime) : 0.0f;
	}

	// --- 팀 킬스코어 / 매치 종료 상태 ---
	// 전부 서버 전용 뮤테이터 — 승리 임계치 판정/매치 종료 오케스트레이션은 AP1ArenaGameMode가 전담하고,
	// 여기는 그 결과를 담아 복제하는 순수 데이터 저장소 역할만 한다.

	// AP1ArenaGameMode::BeginPlay()가 NumTeams를 알고 있는 시점에 한 번 호출해 배열 크기를 맞춘다.
	void InitializeTeamScores(int32 NumTeams);

	// TeamId의 킬스코어를 1 증가시킨다. 범위를 벗어난 TeamId는 조용히 무시(방어적).
	void AddTeamKill(int32 TeamId);

	void SetMatchEnded(int32 InWinningTeamId);

	int32 GetTeamKillScore(int32 TeamId) const
	{
		return TeamKillScores.IsValidIndex(TeamId) ? TeamKillScores[TeamId] : 0;
	}

	EP1MatchState GetMatchState() const { return MatchState; }
	int32 GetWinningTeamId() const { return WinningTeamId; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 지금은 UI 트리거 용도가 아니라(결과창은 GameMode가 보내는 Client RPC로 뜬다) 로그만 남긴다 —
	// 다만 값 자체는 계속 복제해둬야 나중에 재접속/관전 클라이언트가 매치 상태를 스냅샷으로 읽을 수 있다.
	UFUNCTION()
	void OnRep_MatchState();

private:
	// 매치가 실제로 시작된 서버 시각(GetServerWorldTimeSeconds() 기준). 전원 복제, RepNotify는 불필요
	// (변경이 매치당 딱 한 번뿐이고, HUD는 이벤트가 아니라 매 프레임 GetElapsedMatchTime() 폴링으로 읽음).
	// 기본값 -1(미설정)은 실제 서버 시각(항상 0 이상)과 절대 겹치지 않는 센티널.
	UPROPERTY(Replicated)
	float MatchStartServerTime = -1.0f;

	// 인덱스=TeamId, 값=그 팀의 누적 킬스코어. InitializeTeamScores()로 NumTeams만큼 0으로 채워진다.
	UPROPERTY(Replicated)
	TArray<int32> TeamKillScores;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	EP1MatchState MatchState = EP1MatchState::InProgress;

	// -1 = 아직 미정.
	UPROPERTY(Replicated)
	int32 WinningTeamId = -1;
};
