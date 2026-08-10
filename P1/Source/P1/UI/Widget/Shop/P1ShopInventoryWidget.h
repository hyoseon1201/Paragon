// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1ShopInventoryWidget.generated.h"

class UPanelWidget;
class UP1ShopInventorySlotWidget;
class UP1ShopWidgetController;

// 상점 중앙하단 — 현재 보유 중인 아이템을 항상 PlayerState->GetMaxInventorySlots()개만큼(빈 칸 포함)
// 표시한다. 컨트롤러를 전파받아 스스로 OnInventoryChanged를 구독하고(상점의 다른 자식 위젯들과 같은
// 패턴), 우클릭으로 해당 칸을 파는 실제 로직은 각 UP1ShopInventorySlotWidget이 직접 처리한다.
UCLASS()
class P1_API UP1ShopInventoryWidget : public UP1UserWidget
{
	GENERATED_BODY()

protected:
	virtual void OnWidgetControllerSet() override;

	// 6칸을 가로로 나열할 컨테이너 — HorizontalBox 등 임의 Panel(런타임에 AddChild로 채우므로
	// Canvas/Overlay 불가).
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> InventorySlotsContainer;

	// 슬롯을 이 클래스로 생성 — WBP_ShopInventorySlot(parent=UP1ShopInventorySlotWidget) 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UP1ShopInventorySlotWidget> InventorySlotWidgetClass;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshInventory();

	UP1ShopWidgetController* GetShopController() const;
};
