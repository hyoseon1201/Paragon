// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/HUD/P1HUDInventoryWidget.h"
#include "UI/Widget/HUD/P1HUDInventorySlotWidget.h"
#include "UI/WidgetController/P1OverlayWidgetController.h"
#include "Components/PanelWidget.h"
#include "Player/P1PlayerState.h"
#include "Player/P1ShopTypes.h"
#include "Engine/DataTable.h"

void UP1HUDInventoryWidget::OnWidgetControllerSet()
{
	// UP1OverlayWidgetController여야 한다는 타입 계약만 확인한다 — 실제 데이터 접근은 베이스
	// UP1WidgetController::GetP1PlayerState()로 충분해서 캐스팅된 포인터를 따로 들고 있지 않는다.
	CastChecked<UP1OverlayWidgetController>(WidgetController);

	if (AP1PlayerState* PS = WidgetController->GetP1PlayerState())
	{
		PS->OnInventoryChangedNative.AddUObject(this, &UP1HUDInventoryWidget::HandleInventoryChanged);
	}

	RefreshInventory();
}

void UP1HUDInventoryWidget::HandleInventoryChanged()
{
	RefreshInventory();
}

void UP1HUDInventoryWidget::RefreshInventory()
{
	AP1PlayerState* PS = WidgetController ? WidgetController->GetP1PlayerState() : nullptr;
	if (!InventorySlotsContainer || !InventorySlotWidgetClass || !PS)
	{
		return;
	}

	UDataTable* ShopItemTable = PS->GetShopItemTable();

	InventorySlotsContainer->ClearChildren();

	const TArray<FName>& Inventory = PS->GetInventory();
	const int32 SlotCount = PS->GetMaxInventorySlots();

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UP1HUDInventorySlotWidget* SlotWidget = CreateWidget<UP1HUDInventorySlotWidget>(GetOwningPlayer(), InventorySlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		const FP1ShopItemData* ItemData = nullptr;
		if (Inventory.IsValidIndex(Index) && ShopItemTable)
		{
			ItemData = ShopItemTable->FindRow<FP1ShopItemData>(Inventory[Index], TEXT("UP1HUDInventoryWidget::RefreshInventory"));
		}

		SlotWidget->SetSlotData(ItemData);
		InventorySlotsContainer->AddChild(SlotWidget);
	}
}
