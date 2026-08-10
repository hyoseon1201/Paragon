// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/HUD/P1HUDInventorySlotWidget.h"
#include "Components/Image.h"
#include "Player/P1ShopTypes.h"
#include "Engine/Texture2D.h"

void UP1HUDInventorySlotWidget::SetSlotData(const FP1ShopItemData* ItemData)
{
	if (!IconImage)
	{
		return;
	}

	if (ItemData && ItemData->Icon)
	{
		IconImage->SetBrushFromTexture(ItemData->Icon);
		IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		IconImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}
