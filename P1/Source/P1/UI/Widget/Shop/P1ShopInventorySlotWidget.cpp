// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Shop/P1ShopInventorySlotWidget.h"
#include "Components/Image.h"
#include "Player/P1PlayerState.h"
#include "Player/P1ShopTypes.h"
#include "Engine/Texture2D.h"

void UP1ShopInventorySlotWidget::SetSlotData(AP1PlayerState* InPlayerState, FName InItemRowName, const FP1ShopItemData* ItemData)
{
	OwningPlayerState = InPlayerState;
	ItemRowName = ItemData ? InItemRowName : NAME_None;

	if (IconImage)
	{
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
}

FReply UP1ShopInventorySlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (AP1PlayerState* PS = OwningPlayerState.Get())
		{
			if (!ItemRowName.IsNone())
			{
				PS->ServerSellItem(ItemRowName);
			}
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
