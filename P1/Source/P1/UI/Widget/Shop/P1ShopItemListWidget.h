// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "Player/P1ShopTypes.h"
#include "P1ShopItemListWidget.generated.h"

class UPanelWidget;
class UP1ShopItemSlotWidget;
class UP1ShopWidgetController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopItemListSelectionChanged, FName, ItemRowName);

// 상점 좌측 — 역할군 하나를 받아 그 역할군에 속한 카탈로그 아이템만 나열한다. UP1ShopWidget이
// SetWidgetController()로 전파해준 UP1ShopWidgetController를 통해 PlayerState/인벤토리 변경 알림에
// 스스로 접근한다(부모가 매번 데이터를 계산해 넘겨주는 대신, 이 위젯이 필요한 걸 직접 구독/조회).
UCLASS()
class P1_API UP1ShopItemListWidget : public UP1UserWidget
{
	GENERATED_BODY()

public:
	// 역할군을 지정해 목록을 다시 그린다(탭 전환 시) — UP1ShopWidget이 호출.
	void ShowCategory(EP1ItemCategory Category);

	// 현재 표시 중인 역할군을 유지한 채 보유 배지만 갱신 — OnInventoryChanged 구독을 통해 스스로 호출.
	void RefreshOwnedBadges();

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnShopItemListSelectionChanged OnItemSelected;

protected:
	virtual void OnWidgetControllerSet() override;

	// 목록을 나열할 컨테이너 — WrapBox 등 임의 Panel(런타임에 AddChild로 채우므로 Canvas/Overlay 불가).
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> CatalogContainer;

	// 목록 칸을 이 클래스로 생성 — WBP_ShopItemSlot(parent=UP1ShopItemSlotWidget) 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UP1ShopItemSlotWidget> ItemSlotWidgetClass;

private:
	UFUNCTION()
	void HandleSlotSelected(FName ItemRowName);
	UFUNCTION()
	void HandleInventoryChanged();

	UP1ShopWidgetController* GetShopController() const;

	EP1ItemCategory ActiveCategory = EP1ItemCategory::Carry;
};
