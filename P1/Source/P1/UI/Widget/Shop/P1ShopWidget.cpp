// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Shop/P1ShopWidget.h"
#include "UI/Widget/Shop/P1ShopItemListWidget.h"
#include "UI/Widget/Shop/P1ShopItemDetailWidget.h"
#include "UI/Widget/Shop/P1ShopStatsWidget.h"
#include "UI/Widget/Shop/P1ShopInventoryWidget.h"
#include "UI/WidgetController/P1ShopWidgetController.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Player/P1PlayerState.h"
#include "Player/P1PlayerController.h"

void UP1ShopWidget::OnWidgetControllerSet()
{
	UP1ShopWidgetController* Controller = CastChecked<UP1ShopWidgetController>(WidgetController);

	Controller->OnGoldChanged.AddDynamic(this, &UP1ShopWidget::OnGoldChanged);

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UP1ShopWidget::HandleCloseClicked);
	}
	if (CarryTabButton)
	{
		CarryTabButton->OnClicked.AddDynamic(this, &UP1ShopWidget::HandleCarryTabClicked);
	}
	if (MageTabButton)
	{
		MageTabButton->OnClicked.AddDynamic(this, &UP1ShopWidget::HandleMageTabClicked);
	}
	if (AssassinTabButton)
	{
		AssassinTabButton->OnClicked.AddDynamic(this, &UP1ShopWidget::HandleAssassinTabClicked);
	}
	if (TankTabButton)
	{
		TankTabButton->OnClicked.AddDynamic(this, &UP1ShopWidget::HandleTankTabClicked);
	}
	if (FighterTabButton)
	{
		FighterTabButton->OnClicked.AddDynamic(this, &UP1ShopWidget::HandleFighterTabClicked);
	}
	if (SupportTabButton)
	{
		SupportTabButton->OnClicked.AddDynamic(this, &UP1ShopWidget::HandleSupportTabClicked);
	}

	// 컨트롤러를 자식들에게도 그대로 전파 — 각자 필요한 델리게이트를 스스로 구독하고 초기화한다
	// (부모가 매번 계산해서 떠먹여주는 대신, D1류 GAS 위젯컨트롤러 패턴과 통일).
	if (ItemListWidget)
	{
		ItemListWidget->SetWidgetController(Controller);
		ItemListWidget->OnItemSelected.AddDynamic(this, &UP1ShopWidget::HandleItemSelected);
	}
	if (ItemDetailWidget)
	{
		ItemDetailWidget->SetWidgetController(Controller);
	}
	if (StatsWidget)
	{
		StatsWidget->SetWidgetController(Controller);
	}
	if (InventoryWidget)
	{
		InventoryWidget->SetWidgetController(Controller);
	}
}

AP1PlayerState* UP1ShopWidget::GetOwningP1PlayerState() const
{
	return GetOwningPlayerState<AP1PlayerState>();
}

void UP1ShopWidget::OnGoldChanged(float NewGold)
{
	if (GoldText)
	{
		GoldText->SetText(FText::AsNumber(FMath::RoundToInt(NewGold)));
	}
}

void UP1ShopWidget::HandleCloseClicked()
{
	// 자기 Visibility만 바꾸면 안 된다 — 입력모드/마우스 커서 복구까지 AP1PlayerController::CloseShop()이
	// 담당해야 P키 토글로 닫을 때와 동일하게 동작한다(예전엔 여기서 SetVisibility만 해서, X버튼으로
	// 닫으면 마우스 커서가 안 사라지고 GameAndUI 입력모드에 남는 버그가 있었음).
	if (AP1PlayerController* PC = Cast<AP1PlayerController>(GetOwningPlayer()))
	{
		PC->CloseShop();
	}
}

void UP1ShopWidget::HandleCarryTabClicked() { SetActiveCategory(EP1ItemCategory::Carry); }
void UP1ShopWidget::HandleMageTabClicked() { SetActiveCategory(EP1ItemCategory::Mage); }
void UP1ShopWidget::HandleAssassinTabClicked() { SetActiveCategory(EP1ItemCategory::Assassin); }
void UP1ShopWidget::HandleTankTabClicked() { SetActiveCategory(EP1ItemCategory::Tank); }
void UP1ShopWidget::HandleFighterTabClicked() { SetActiveCategory(EP1ItemCategory::Fighter); }
void UP1ShopWidget::HandleSupportTabClicked() { SetActiveCategory(EP1ItemCategory::Support); }

void UP1ShopWidget::SetActiveCategory(EP1ItemCategory NewCategory)
{
	if (ActiveCategory == NewCategory)
	{
		return;
	}
	ActiveCategory = NewCategory;
	if (ItemListWidget)
	{
		ItemListWidget->ShowCategory(ActiveCategory);
	}
}

void UP1ShopWidget::HandleItemSelected(FName ItemRowName)
{
	if (ItemDetailWidget)
	{
		ItemDetailWidget->ShowItem(ItemRowName);
	}
}
