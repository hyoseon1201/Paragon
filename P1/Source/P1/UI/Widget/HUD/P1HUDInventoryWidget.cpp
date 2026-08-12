// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/HUD/P1HUDInventoryWidget.h"
#include "UI/Widget/HUD/P1HUDInventorySlotWidget.h"
#include "UI/WidgetController/P1OverlayWidgetController.h"
#include "Components/PanelWidget.h"
#include "Components/HorizontalBoxSlot.h"
#include "Player/P1PlayerState.h"
#include "Player/P1ShopTypes.h"
#include "Engine/DataTable.h"
#include "P1.h"

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
		UE_LOG(LogP1, Warning, TEXT("[HUDInventory] RefreshInventory 실패 — InventorySlotsContainer바인딩=%d InventorySlotWidgetClass=%s PS=%s"),
			InventorySlotsContainer != nullptr, *GetNameSafe(InventorySlotWidgetClass), *GetNameSafe(PS));
		return;
	}

	UDataTable* ShopItemTable = PS->GetShopItemTable();

	InventorySlotsContainer->ClearChildren();

	const TArray<FName>& Inventory = PS->GetInventory();
	const int32 SlotCount = PS->GetMaxInventorySlots();

	UE_LOG(LogP1, Log, TEXT("[HUDInventory] RefreshInventory — SlotCount=%d Inventory.Num()=%d ShopItemTable=%s"),
		SlotCount, Inventory.Num(), *GetNameSafe(ShopItemTable));

	int32 AddedCount = 0;
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UP1HUDInventorySlotWidget* SlotWidget = CreateWidget<UP1HUDInventorySlotWidget>(GetOwningPlayer(), InventorySlotWidgetClass);
		if (!SlotWidget)
		{
			UE_LOG(LogP1, Warning, TEXT("[HUDInventory]   슬롯 %d — CreateWidget 실패"), Index);
			continue;
		}

		const FP1ShopItemData* ItemData = nullptr;
		FName ItemRowName = NAME_None;
		if (Inventory.IsValidIndex(Index) && ShopItemTable)
		{
			ItemRowName = Inventory[Index];
			ItemData = ShopItemTable->FindRow<FP1ShopItemData>(ItemRowName, TEXT("UP1HUDInventoryWidget::RefreshInventory"));
		}

		SlotWidget->SetSlotData(ItemData);
		if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(InventorySlotsContainer->AddChild(SlotWidget)))
		{
			// Size Rule 기본값(Fill)을 Auto로 강제 — Fill이면 부모(HUD 레이아웃)가 이 컨테이너를
			// Fill로 늘려놓은 만큼 슬롯 콘텐츠 크기(SizeBox Override)를 무시하고 직사각형으로 늘어난다.
			// Auto여야 SizeBox가 정한 정사각형 크기를 그대로 존중한다.
			HorizontalSlot->SetSize(ESlateSizeRule::Automatic);
			HorizontalSlot->SetPadding(FMargin(10.f));
		}
		++AddedCount;
		UE_LOG(LogP1, Log, TEXT("[HUDInventory]   슬롯 %d 추가 — ItemRowName=%s ItemData=%s"),
			Index, *ItemRowName.ToString(), ItemData ? TEXT("있음") : TEXT("없음(빈칸)"));
	}

	UE_LOG(LogP1, Log, TEXT("[HUDInventory] RefreshInventory 완료 — %d개 슬롯 생성, Container 자식 수=%d"),
		AddedCount, InventorySlotsContainer->GetChildrenCount());
}
