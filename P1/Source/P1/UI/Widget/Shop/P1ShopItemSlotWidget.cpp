// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Shop/P1ShopItemSlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Player/P1PlayerState.h"
#include "Player/P1ShopTypes.h"
#include "Engine/Texture2D.h"

void UP1ShopItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddDynamic(this, &UP1ShopItemSlotWidget::HandleSelectClicked);
	}
}

void UP1ShopItemSlotWidget::SetSlotData(AP1PlayerState* InPlayerState, FName InItemRowName, const FP1ShopItemData& ItemData, bool bInOwned)
{
	OwningPlayerState = InPlayerState;
	ItemRowName = InItemRowName;

	if (NameText)
	{
		NameText->SetText(ItemData.DisplayName);
	}

	if (IconImage && ItemData.Icon)
	{
		IconImage->SetBrushFromTexture(ItemData.Icon);
	}

	if (OwnedBadge)
	{
		OwnedBadge->SetVisibility(bInOwned ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UP1ShopItemSlotWidget::HandleSelectClicked()
{
	if (!ItemRowName.IsNone())
	{
		OnSelected.Broadcast(ItemRowName);
	}
}

FReply UP1ShopItemSlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (AP1PlayerState* PS = OwningPlayerState.Get())
	{
		// 서버도 어차피 중복 보유를 거부하지만(ServerBuyItem), 이미 보유 중인 아이템은 애초에 RPC를
		// 보내지 않고 클라이언트에서 조용히 막는다.
		if (!ItemRowName.IsNone() && !PS->GetInventory().Contains(ItemRowName))
		{
			PS->ServerBuyItem(ItemRowName);
		}
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}
