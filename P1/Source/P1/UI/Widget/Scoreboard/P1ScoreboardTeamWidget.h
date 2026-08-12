// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1ScoreboardTeamWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class UP1ScoreboardPlayerRowWidget;

// 점수판의 팀 하나 — 그 팀 소속 플레이어들의 행(UP1ScoreboardPlayerRowWidget)을 담는 컨테이너 +
// 팀 누적 킬스코어(TeamScoreText, 2026-08-12 추가 — 매치 종료 조건 시스템과 함께 도입).
//
// 이 위젯은 WBP_Scoreboard의 디자이너에 팀마다 하나씩 "미리 배치"하는 방식이다(런타임 생성 아님) —
// 팀 수가 고정(AP1ArenaGameMode::NumTeams)이고 배치 위치를 디자이너에서 정하고 싶기 때문. 반대로
// 팀 내 플레이어 행은 접속자 수에 따라 달라지므로 아래 PlayerRowWidgetClass로 런타임 생성한다.
//
// UP1ScoreboardWidget이 SetWidgetController()로 전파해준 UP1ScoreboardWidgetController를 통해
// GameState->PlayerArray에 스스로 접근한다(부모가 미리 필터링한 배열을 넘겨주지 않음).
UCLASS()
class P1_API UP1ScoreboardTeamWidget : public UP1UserWidget
{
	GENERATED_BODY()

public:
	// 이 팀 소속 플레이어 전원의 최신 스냅샷으로 컨테이너를 새로 채운다 — 매 리프레시마다 기존 행을
	// 지우고 다시 만든다(Tab을 누르고 있는 동안만 잠깐 보이는 UI라 재사용 풀링 없이도 비용이 무시할
	// 수준). 아무도 없는 팀이면 행만 비우고 팀 라벨은 그대로 유지된다.
	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	void RefreshTeam();

	// 이 슬롯이 담당할 팀 번호 — WBP_Scoreboard에 배치한 각 인스턴스의 Details 패널에서 지정한다
	// (AP1ArenaGameMode가 배정하는 TeamId 0..NumTeams-1과 일치시킬 것).
	uint8 GetDesignatedTeamId() const { return DesignatedTeamId; }

protected:
	UPROPERTY(EditAnywhere, Category = "Scoreboard")
	uint8 DesignatedTeamId = 0;

	// 생성할 플레이어 행 위젯 클래스 — WBP_ScoreboardPlayerRow(parent=UP1ScoreboardPlayerRowWidget) 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Scoreboard")
	TSubclassOf<UP1ScoreboardPlayerRowWidget> PlayerRowWidgetClass;

	// VerticalBox 등 임의 컨테이너 — 팀원 수만큼 PlayerRowWidgetClass 인스턴스를 자식으로 추가한다.
	// 런타임에 AddChild/ClearChildren으로 갱신되므로 반드시 Box 계열(자동 배치)이어야 한다 —
	// CanvasPanel/Overlay는 자식이 전부 같은 자리에 겹쳐서 안 된다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PlayerListContainer;

	// 팀 구분용 라벨(선택, 예: "Team 0") — 없어도 크래시는 안 나고 라벨만 안 뜬다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TeamNameText;

	// 이 팀의 누적 킬스코어(AP1GameState::GetTeamKillScore) — 매치 진행 중엔 Tab 스코어보드에서
	// 실시간 순위 확인용으로, 매치 종료 후엔 결과창에서 최종 스코어 확인용으로 같은 위젯이 쓰인다
	// (스코어보드와 결과창이 이 클래스를 공유하므로 둘 다 공짜로 적용됨). 선택 — 없어도 크래시 안 남.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TeamScoreText;
};
