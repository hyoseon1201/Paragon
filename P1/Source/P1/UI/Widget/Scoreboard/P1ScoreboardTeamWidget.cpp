// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Scoreboard/P1ScoreboardTeamWidget.h"
#include "UI/Widget/Scoreboard/P1ScoreboardPlayerRowWidget.h"
#include "UI/WidgetController/P1ScoreboardWidgetController.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "GameModes/P1GameState.h"
#include "Player/P1PlayerState.h"
#include "P1.h"

void UP1ScoreboardTeamWidget::RefreshTeam()
{
	if (TeamNameText)
	{
		TeamNameText->SetText(FText::FromString(FString::Printf(TEXT("Team %d"), DesignatedTeamId)));
	}

	// 팀 킬스코어 — PlayerListContainer/PlayerRowWidgetClass 바인딩 여부와 무관하게 먼저 갱신해둔다
	// (아래에서 그 둘이 없으면 조기 리턴하는데, 스코어 표시는 그거랑 상관없이 항상 최신이어야 하므로).
	if (TeamScoreText)
	{
		if (UP1ScoreboardWidgetController* ScoreController = Cast<UP1ScoreboardWidgetController>(WidgetController))
		{
			if (AP1GameState* ScoreGameState = ScoreController->GetP1GameState())
			{
				TeamScoreText->SetText(FText::AsNumber(ScoreGameState->GetTeamKillScore(DesignatedTeamId)));
			}
		}
	}

	if (!PlayerListContainer || !PlayerRowWidgetClass)
	{
		// 진단용 — 어느 인스턴스가, 어느 프로퍼티가 비어있는지 정확히 찍는다(WBP_Scoreboard에 배치한
		// 여러 인스턴스 중 특정 하나만 문제인 경우를 구분하기 위해 DesignatedTeamId/오브젝트 이름도 같이 찍음).
		UE_LOG(LogP1, Warning, TEXT("[Scoreboard] RefreshTeam 실패 — Widget=%s DesignatedTeamId=%d PlayerListContainer바인딩=%d PlayerRowWidgetClass=%s"),
			*GetName(), DesignatedTeamId, PlayerListContainer != nullptr,
			PlayerRowWidgetClass ? *PlayerRowWidgetClass->GetName() : TEXT("None"));
		return;
	}

	PlayerListContainer->ClearChildren();

	UP1ScoreboardWidgetController* Controller = Cast<UP1ScoreboardWidgetController>(WidgetController);
	AP1GameState* P1GS = Controller ? Controller->GetP1GameState() : nullptr;
	if (!P1GS)
	{
		return;
	}

	int32 PlayerCount = 0;
	for (APlayerState* PS : P1GS->PlayerArray)
	{
		AP1PlayerState* P1PS = Cast<AP1PlayerState>(PS);
		if (!P1PS || P1PS->GetGenericTeamId().GetId() != DesignatedTeamId)
		{
			continue;
		}

		if (UP1ScoreboardPlayerRowWidget* RowWidget = CreateWidget<UP1ScoreboardPlayerRowWidget>(GetOwningPlayer(), PlayerRowWidgetClass))
		{
			RowWidget->SetPlayerData(P1PS);
			PlayerListContainer->AddChild(RowWidget);
			++PlayerCount;
		}
	}

	UE_LOG(LogP1, Log, TEXT("[Scoreboard] RefreshTeam 성공 — Widget=%s DesignatedTeamId=%d 플레이어수=%d"),
		*GetName(), DesignatedTeamId, PlayerCount);
}
