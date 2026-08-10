// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Shop/P1ShopItemListWidget.h"
#include "UI/Widget/Shop/P1ShopItemSlotWidget.h"
#include "UI/WidgetController/P1ShopWidgetController.h"
#include "Components/PanelWidget.h"
#include "Player/P1PlayerState.h"
#include "Engine/DataTable.h"
#include "P1.h"

void UP1ShopItemListWidget::OnWidgetControllerSet()
{
	UP1ShopWidgetController* Controller = CastChecked<UP1ShopWidgetController>(WidgetController);
	Controller->OnInventoryChanged.AddDynamic(this, &UP1ShopItemListWidget::HandleInventoryChanged);

	ShowCategory(ActiveCategory);
}

UP1ShopWidgetController* UP1ShopItemListWidget::GetShopController() const
{
	return Cast<UP1ShopWidgetController>(WidgetController);
}

void UP1ShopItemListWidget::ShowCategory(EP1ItemCategory Category)
{
	ActiveCategory = Category;

	UP1ShopWidgetController* Controller = GetShopController();
	AP1PlayerState* PS = Controller ? Controller->GetP1PlayerState() : nullptr;
	UDataTable* ShopItemTable = PS ? PS->GetShopItemTable() : nullptr;

	const FString ActiveCategoryName = UEnum::GetValueAsString(ActiveCategory);

	if (!CatalogContainer || !ItemSlotWidgetClass || !ShopItemTable)
	{
		UE_LOG(LogP1, Warning, TEXT("[Shop] ShowCategory(%s) 실패 — CatalogContainer바인딩=%d ItemSlotWidgetClass=%s ShopItemTable=%s"),
			*ActiveCategoryName, CatalogContainer != nullptr,
			ItemSlotWidgetClass ? *ItemSlotWidgetClass->GetName() : TEXT("None"),
			ShopItemTable ? *ShopItemTable->GetName() : TEXT("None(BP_P1PlayerState 확인 필요)"));
		return;
	}

	CatalogContainer->ClearChildren();

	TArray<FP1ShopItemData*> AllRows;
	ShopItemTable->GetAllRows<FP1ShopItemData>(TEXT("UP1ShopItemListWidget::ShowCategory"), AllRows);
	const TArray<FName> RowNames = ShopItemTable->GetRowNames();

	UE_LOG(LogP1, Log, TEXT("[Shop] ShowCategory(%s) — ShopItemTable=%s 전체 행 수=%d"),
		*ActiveCategoryName, *ShopItemTable->GetName(), AllRows.Num());

	int32 AddedCount = 0;
	for (int32 Index = 0; Index < AllRows.Num(); ++Index)
	{
		const FName RowName = RowNames.IsValidIndex(Index) ? RowNames[Index] : NAME_None;

		if (!AllRows[Index])
		{
			UE_LOG(LogP1, Warning, TEXT("[Shop]   행 %d(%s) 스킵 — 행 데이터가 null"), Index, *RowName.ToString());
			continue;
		}
		if (!RowNames.IsValidIndex(Index))
		{
			UE_LOG(LogP1, Warning, TEXT("[Shop]   행 %d 스킵 — RowNames 인덱스 불일치"), Index);
			continue;
		}
		if (!AllRows[Index]->Categories.Contains(ActiveCategory))
		{
			UE_LOG(LogP1, Log, TEXT("[Shop]   행 %s 스킵 — Categories에 %s 없음(보유 Categories 개수=%d)"),
				*RowName.ToString(), *ActiveCategoryName, AllRows[Index]->Categories.Num());
			continue;
		}

		if (UP1ShopItemSlotWidget* SlotWidget = CreateWidget<UP1ShopItemSlotWidget>(GetOwningPlayer(), ItemSlotWidgetClass))
		{
			const bool bOwned = PS->GetInventory().Contains(RowName);
			SlotWidget->SetSlotData(PS, RowName, *AllRows[Index], bOwned);
			SlotWidget->OnSelected.AddDynamic(this, &UP1ShopItemListWidget::HandleSlotSelected);
			CatalogContainer->AddChild(SlotWidget);
			++AddedCount;
		}
		else
		{
			UE_LOG(LogP1, Warning, TEXT("[Shop]   행 %s — CreateWidget 실패(ItemSlotWidgetClass=%s)"),
				*RowName.ToString(), *ItemSlotWidgetClass->GetName());
		}
	}

	UE_LOG(LogP1, Log, TEXT("[Shop] ShowCategory(%s) 완료 — %d개 아이템 표시"), *ActiveCategoryName, AddedCount);
}

void UP1ShopItemListWidget::RefreshOwnedBadges()
{
	ShowCategory(ActiveCategory);
}

void UP1ShopItemListWidget::HandleSlotSelected(FName ItemRowName)
{
	OnItemSelected.Broadcast(ItemRowName);
}

void UP1ShopItemListWidget::HandleInventoryChanged()
{
	RefreshOwnedBadges();
}
