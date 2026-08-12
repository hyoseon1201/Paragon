// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Minimap/P1MinimapIconWidget.h"
#include "Components/Image.h"

void UP1MinimapIconWidget::SetIconColor(FLinearColor Color)
{
	if (IconImage)
	{
		IconImage->SetColorAndOpacity(Color);
	}
}
