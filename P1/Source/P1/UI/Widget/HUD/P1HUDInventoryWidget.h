// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1HUDInventoryWidget.generated.h"

class UPanelWidget;
class UP1HUDInventorySlotWidget;

// 메인 HUD 우측하단 — 상점의 UP1ShopInventoryWidget과 같은 데이터(AP1PlayerState::Inventory)를
// 보여주지만 완전 읽기전용이다(우클릭 판매 없음 — 판매는 상점을 열었을 때만 UP1ShopInventorySlotWidget
// 에서 가능. 상점 밖에서도 팔 수 있게 하면 실수로 파는 사고가 나기 쉬워서 의도적으로 뺌).
//
// 상점 전용 컨트롤러(UP1ShopWidgetController)를 새로 꽂지 않고, 이 위젯이 이미 속한 메인 HUD가 쓰는
// UP1OverlayWidgetController를 그대로 받아 PlayerState만 꺼내 쓴다. 인벤토리 변경 알림도 컨트롤러를
// 거치지 않고 AP1PlayerState::OnInventoryChangedNative에 직접 구독한다 — Overlay 컨트롤러에 상점
// 전용 델리게이트를 옮겨 심지 않기 위함(관심사 분리).
UCLASS()
class P1_API UP1HUDInventoryWidget : public UP1UserWidget
{
	GENERATED_BODY()

protected:
	virtual void OnWidgetControllerSet() override;

	// 6칸을 나열할 컨테이너 — HorizontalBox 등 임의 Panel(런타임에 AddChild로 채우므로 Canvas/Overlay 불가).
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> InventorySlotsContainer;

	// 슬롯을 이 클래스로 생성 — WBP_HUDInventorySlot(parent=UP1HUDInventorySlotWidget) 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UP1HUDInventorySlotWidget> InventorySlotWidgetClass;

private:
	// AP1PlayerState::OnInventoryChangedNative가 네이티브(비Dynamic) 델리게이트라 AddUObject로
	// 구독한다 — UFUNCTION 불필요.
	void HandleInventoryChanged();

	void RefreshInventory();
};
