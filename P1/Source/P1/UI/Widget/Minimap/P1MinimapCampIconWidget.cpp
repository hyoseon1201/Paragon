// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Minimap/P1MinimapCampIconWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "P1.h"

void UP1MinimapCampIconWidget::SetIconColor(FLinearColor Color)
{
	if (IconImage)
	{
		IconImage->SetColorAndOpacity(Color);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[MinimapCampIcon] SetIconColor 실패 — IconImage바인딩=0 (WBP_MinimapCampIcon에서 이름/타입 확인 필요)"));
	}
}

void UP1MinimapCampIconWidget::SetCountdownSeconds(float RemainingSeconds)
{
	if (!CountdownText)
	{
		return;
	}

	CountdownText->SetText(FText::AsNumber(FMath::Max(0, FMath::RoundToInt(RemainingSeconds))));
	CountdownText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UP1MinimapCampIconWidget::ClearCountdown()
{
	if (!CountdownText)
	{
		return;
	}

	CountdownText->SetText(FText::GetEmpty());
	CountdownText->SetVisibility(ESlateVisibility::Collapsed);
}
