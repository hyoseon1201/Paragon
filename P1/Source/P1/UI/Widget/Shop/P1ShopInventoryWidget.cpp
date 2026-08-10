// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Shop/P1ShopInventoryWidget.h"
#include "UI/Widget/Shop/P1ShopInventorySlotWidget.h"
#include "UI/WidgetController/P1ShopWidgetController.h"
#include "Components/PanelWidget.h"
#include "Player/P1PlayerState.h"
#include "Player/P1ShopTypes.h"
#include "Engine/DataTable.h"

void UP1ShopInventoryWidget::OnWidgetControllerSet()
{
	UP1ShopWidgetController* Controller = CastChecked<UP1ShopWidgetController>(WidgetController);
	Controller->OnInventoryChanged.AddDynamic(this, &UP1ShopInventoryWidget::HandleInventoryChanged);

	RefreshInventory();
}

UP1ShopWidgetController* UP1ShopInventoryWidget::GetShopController() const
{
	return Cast<UP1ShopWidgetController>(WidgetController);
}

void UP1ShopInventoryWidget::HandleInventoryChanged()
{
	RefreshInventory();
}

void UP1ShopInventoryWidget::RefreshInventory()
{
	UP1ShopWidgetController* Controller = GetShopController();
	AP1PlayerState* PS = Controller ? Controller->GetP1PlayerState() : nullptr;
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
		UP1ShopInventorySlotWidget* SlotWidget = CreateWidget<UP1ShopInventorySlotWidget>(GetOwningPlayer(), InventorySlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		const FP1ShopItemData* ItemData = nullptr;
		FName ItemRowName = NAME_None;
		if (Inventory.IsValidIndex(Index) && ShopItemTable)
		{
			ItemRowName = Inventory[Index];
			ItemData = ShopItemTable->FindRow<FP1ShopItemData>(ItemRowName, TEXT("UP1ShopInventoryWidget::RefreshInventory"));
		}

		SlotWidget->SetSlotData(PS, ItemRowName, ItemData);
		InventorySlotsContainer->AddChild(SlotWidget);
	}
}
