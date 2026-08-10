// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1ScoreboardWidget.generated.h"

// 최상위 점수판 위젯 — Tab 홀드 동안만 보이는 스냅샷 UI(AP1PlayerController가 표시/숨김을 담당).
// UP1ScoreboardWidgetController를 전파받아 미리 배치된 팀 위젯들에 그대로 넘긴다(상점의
// SetWidgetController 전파 패턴과 통일) — 컨트롤러 자체엔 델리게이트가 없고 GameState 접근만 제공한다.
//
// 팀 위젯(UP1ScoreboardTeamWidget)은 **WBP_Scoreboard 디자이너에 미리 배치**한다(런타임 생성 아님) —
// 팀 수가 고정이고 각 팀 패널을 원하는 위치에 자유롭게 놓고 싶기 때문. RefreshScoreboard()가 위젯
// 트리를 훑어 배치된 팀 위젯을 전부 찾고 각자 RefreshTeam()을 부르면, 그 안에서 자기 DesignatedTeamId로
// GameState->PlayerArray를 걸러 스스로 채운다 — 데이터를 계산해서 넘겨주는 쪽은 더 이상 이 클래스가 아니다.
UCLASS()
class P1_API UP1ScoreboardWidget : public UP1UserWidget
{
	GENERATED_BODY()

public:
	// AP1PlayerController가 스코어보드를 보여줄 때마다 호출 — 배치된 각 팀 위젯에 새로 그리라고 지시한다.
	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	void RefreshScoreboard();

protected:
	virtual void OnWidgetControllerSet() override;
};
