// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1ShopItemDetailWidget.generated.h"

class UTextBlock;
class UImage;
class UWidget;
class UPanelWidget;
class UP1ShopWidgetController;
class UP1ShopStatChipWidget;
class UP1ShopUniqueAbilityWidget;

// 상점 우측 — 좌측 UP1ShopItemListWidget에서 선택된 아이템 하나의 상세정보를 보여주기만 하는
// 순수 표시 패널이다. 구매는 이 패널이 아니라 목록의 UP1ShopItemSlotWidget을 더블클릭해서 하고,
// 판매는 중앙하단 인벤토리 UP1ShopInventorySlotWidget의 우클릭이 담당 — 이 패널엔 클릭 인터랙션이
// 전혀 없다.
UCLASS()
class P1_API UP1ShopItemDetailWidget : public UP1UserWidget
{
	GENERATED_BODY()

public:
	// 목록에서 아이템을 선택했을 때 호출. NAME_None을 넘기면 패널을 감춘다.
	void ShowItem(FName InItemRowName);

protected:
	virtual void OnWidgetControllerSet() override;

	// 상세 패널 전체를 감싸는 최상위 위젯 — 아무것도 선택 안 됐을 때 이걸 Collapsed로 감춘다(선택, 없으면 무시).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DetailPanelRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	// FP1ShopItemData::DisplayStats 개수만큼 UP1ShopStatChipWidget을 채우는 컨테이너 — WrapBox 등
	// 임의 Panel(런타임에 AddChild로 채우므로 Canvas/Overlay 불가).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> StatsContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UP1ShopStatChipWidget> StatChipWidgetClass;

	// FP1ShopItemData::UniqueAbilities 개수만큼 UP1ShopUniqueAbilityWidget을 채우는 컨테이너.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> UniqueAbilitiesContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UP1ShopUniqueAbilityWidget> UniqueAbilityWidgetClass;

private:
	UP1ShopWidgetController* GetShopController() const;

	FName SelectedItemRowName;
};
