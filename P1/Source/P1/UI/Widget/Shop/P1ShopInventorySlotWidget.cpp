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
		// 빈 칸이어도 Collapsed로 감추지 않는다 — Collapsed는 레이아웃 공간 자체를 0으로 만들어서,
		// 슬롯 크기가 이 이미지(+감싸고 있는 Border 등)에 의해 결정되는 구조라면 빈 칸 전체가 사라져
		// 보인다. 텍스처만 비우고 계속 Visible로 둬서 배경/테두리는 그대로 유지되게 한다.
		IconImage->SetBrushFromTexture((ItemData && ItemData->Icon) ? ItemData->Icon : nullptr);
		IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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
