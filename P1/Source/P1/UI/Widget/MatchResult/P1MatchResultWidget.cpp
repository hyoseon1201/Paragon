// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/MatchResult/P1MatchResultWidget.h"
#include "UI/Widget/Scoreboard/P1ScoreboardTeamWidget.h"
#include "UI/WidgetController/P1ScoreboardWidgetController.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "P1.h"

void UP1MatchResultWidget::SetWinningTeamId(int32 TeamId)
{
	if (WinningTeamText)
	{
		WinningTeamText->SetText(FText::FromString(FString::Printf(TEXT("Team %d 승리!"), TeamId)));
	}
}

void UP1MatchResultWidget::OnWidgetControllerSet()
{
	UP1ScoreboardWidgetController* Controller = CastChecked<UP1ScoreboardWidgetController>(WidgetController);

	if (!WidgetTree)
	{
		return;
	}

	// 스코어보드와 동일한 팀 위젯을 재사용 — 디자이너에 미리 배치된 인스턴스 전부에 같은 컨트롤러를 전파.
	WidgetTree->ForEachWidget([Controller](UWidget* Widget)
	{
		if (UP1ScoreboardTeamWidget* TeamWidget = Cast<UP1ScoreboardTeamWidget>(Widget))
		{
			TeamWidget->SetWidgetController(Controller);
		}
	});
}

void UP1MatchResultWidget::RefreshResult()
{
	if (!WidgetTree)
	{
		return;
	}

	int32 FoundCount = 0;
	WidgetTree->ForEachWidget([&FoundCount](UWidget* Widget)
	{
		if (UP1ScoreboardTeamWidget* TeamWidget = Cast<UP1ScoreboardTeamWidget>(Widget))
		{
			TeamWidget->RefreshTeam();
			++FoundCount;
		}
	});

	if (FoundCount == 0)
	{
		UE_LOG(LogP1, Warning, TEXT("[MatchResult] 배치된 팀 위젯이 없습니다 — WBP_MatchResult에 WBP_ScoreboardTeam 인스턴스를 팀마다 하나씩 배치하고 각각의 DesignatedTeamId를 설정하세요."));
	}
}
