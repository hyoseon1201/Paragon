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

	// 빈 칸이어도 Collapsed로 감추지 않는다 — Collapsed는 레이아웃 공간 자체를 0으로 만들어서,
	// 슬롯 크기가 이 이미지(+감싸고 있는 Border 등)에 의해 결정되는 구조라면 빈 칸 전체가 사라져
	// 보인다. 텍스처만 비우고 계속 Visible로 둬서 배경/테두리는 그대로 유지되게 한다.
	IconImage->SetBrushFromTexture((ItemData && ItemData->Icon) ? ItemData->Icon : nullptr);
	IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}
