// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Scoreboard/P1ScoreboardWidget.h"
#include "UI/Widget/Scoreboard/P1ScoreboardTeamWidget.h"
#include "UI/WidgetController/P1ScoreboardWidgetController.h"
#include "Blueprint/WidgetTree.h"
#include "P1.h"

void UP1ScoreboardWidget::OnWidgetControllerSet()
{
	UP1ScoreboardWidgetController* Controller = CastChecked<UP1ScoreboardWidgetController>(WidgetController);

	if (!WidgetTree)
	{
		return;
	}

	// 디자이너에 미리 배치된 팀 위젯 전부에 같은 컨트롤러를 전파 — 각자 GameState에 스스로 접근하게 된다.
	WidgetTree->ForEachWidget([Controller](UWidget* Widget)
	{
		if (UP1ScoreboardTeamWidget* TeamWidget = Cast<UP1ScoreboardTeamWidget>(Widget))
		{
			TeamWidget->SetWidgetController(Controller);
		}
	});
}

void UP1ScoreboardWidget::RefreshScoreboard()
{
	// 디자이너에 미리 배치된 팀 위젯을 매 리프레시마다 새로 훑는다 — 위젯 트리가 작아 비용이 무시할
	// 수준이고(Tab을 누를 때만 호출), 캐시를 두지 않아 스테일 포인터 걱정도 없다.
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
		UE_LOG(LogP1, Warning, TEXT("[Scoreboard] 배치된 팀 위젯이 없습니다 — WBP_Scoreboard에 WBP_ScoreboardTeam 인스턴스를 팀마다 하나씩 배치하고 각각의 DesignatedTeamId를 설정하세요."));
	}
}
