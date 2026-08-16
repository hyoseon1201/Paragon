// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Minimap/P1MinimapIconWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "P1.h"

void UP1MinimapIconWidget::SetIconColor(FLinearColor Color)
{
	if (BorderImage)
	{
		BorderImage->SetColorAndOpacity(Color);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[MinimapIcon] SetIconColor 실패 — BorderImage바인딩=0 (WBP_MinimapIcon에서 이름/타입 확인 필요)"));
	}
}

void UP1MinimapIconWidget::SetPortraitTexture(UTexture2D* Texture)
{
	if (!PortraitImage)
	{
		return;
	}

	if (Texture)
	{
		PortraitImage->SetBrushFromTexture(Texture);
		PortraitImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		PortraitImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}
