// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "P1GameState.generated.h"

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

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// 매치가 실제로 시작된 서버 시각(GetServerWorldTimeSeconds() 기준). 전원 복제, RepNotify는 불필요
	// (변경이 매치당 딱 한 번뿐이고, HUD는 이벤트가 아니라 매 프레임 GetElapsedMatchTime() 폴링으로 읽음).
	// 기본값 -1(미설정)은 실제 서버 시각(항상 0 이상)과 절대 겹치지 않는 센티널.
	UPROPERTY(Replicated)
	float MatchStartServerTime = -1.0f;
};
