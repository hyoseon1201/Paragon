// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1MatchResultWidget.generated.h"

class UTextBlock;

// 매치 종료 결과창 — AP1PlayerController::ClientShowMatchResult()가 매치 종료 시점에 한 번 생성해
// 띄운다. 팀별 최종 KDA 목록은 스코어보드(UP1ScoreboardTeamWidget/UP1ScoreboardPlayerRowWidget)를
// 그대로 재사용한다 — 둘 다 "GameState->PlayerArray를 팀별로 훑어 보여준다"는 같은 요구라 새 위젯
// 클래스를 또 만들 필요가 없다. 이 클래스가 새로 갖는 건 승리 팀 발표 텍스트뿐이고, 컨트롤러 전파와
// 팀 위젯 리프레시 로직은 UP1ScoreboardWidget과 완전히 동일한 패턴이다(WBP에 팀 위젯을 미리 배치).
UCLASS()
class P1_API UP1MatchResultWidget : public UP1UserWidget
{
	GENERATED_BODY()

public:
	// AP1PlayerController::ClientShowMatchResult()가 생성 직후 한 번 호출.
	void SetWinningTeamId(int32 TeamId);

	// 배치된 각 팀 위젯에 새로 그리라고 지시 — UP1ScoreboardWidget::RefreshScoreboard()와 동일.
	UFUNCTION(BlueprintCallable, Category = "MatchResult")
	void RefreshResult();

protected:
	virtual void OnWidgetControllerSet() override;

	// "Team 0 승리!" 같은 승리 팀 발표 텍스트 — 선택(없어도 크래시 안 남).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WinningTeamText;
};
