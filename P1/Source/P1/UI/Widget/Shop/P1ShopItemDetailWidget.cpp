// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Shop/P1ShopItemDetailWidget.h"
#include "UI/Widget/Shop/P1ShopStatChipWidget.h"
#include "UI/Widget/Shop/P1ShopUniqueAbilityWidget.h"
#include "UI/WidgetController/P1ShopWidgetController.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Player/P1PlayerState.h"
#include "Player/P1ShopTypes.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"

void UP1ShopItemDetailWidget::OnWidgetControllerSet()
{
	ShowItem(NAME_None);
}

UP1ShopWidgetController* UP1ShopItemDetailWidget::GetShopController() const
{
	return Cast<UP1ShopWidgetController>(WidgetController);
}

void UP1ShopItemDetailWidget::ShowItem(FName InItemRowName)
{
	SelectedItemRowName = InItemRowName;

	UP1ShopWidgetController* Controller = GetShopController();
	AP1PlayerState* PS = Controller ? Controller->GetP1PlayerState() : nullptr;
	UDataTable* ShopItemTable = PS ? PS->GetShopItemTable() : nullptr;
	const FP1ShopItemData* ItemData = (ShopItemTable && !SelectedItemRowName.IsNone())
		? ShopItemTable->FindRow<FP1ShopItemData>(SelectedItemRowName, TEXT("UP1ShopItemDetailWidget::ShowItem"))
		: nullptr;

	if (!ItemData)
	{
		if (DetailPanelRoot)
		{
			DetailPanelRoot->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (StatsContainer)
		{
			StatsContainer->ClearChildren();
		}
		if (UniqueAbilitiesContainer)
		{
			UniqueAbilitiesContainer->ClearChildren();
		}
		return;
	}

	if (DetailPanelRoot)
	{
		DetailPanelRoot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (NameText)
	{
		NameText->SetText(ItemData->DisplayName);
	}
	if (PriceText)
	{
		PriceText->SetText(FText::AsNumber(ItemData->Price));
	}
	if (IconImage && ItemData->Icon)
	{
		IconImage->SetBrushFromTexture(ItemData->Icon);
	}

	if (StatsContainer && StatChipWidgetClass)
	{
		StatsContainer->ClearChildren();
		for (const FP1ItemDisplayStat& Stat : ItemData->DisplayStats)
		{
			if (UP1ShopStatChipWidget* Chip = CreateWidget<UP1ShopStatChipWidget>(GetOwningPlayer(), StatChipWidgetClass))
			{
				Chip->SetStatData(Stat);
				StatsContainer->AddChild(Chip);
			}
		}
	}

	if (UniqueAbilitiesContainer && UniqueAbilityWidgetClass)
	{
		UniqueAbilitiesContainer->ClearChildren();
		for (const FP1ItemUniqueAbility& Ability : ItemData->UniqueAbilities)
		{
			if (UP1ShopUniqueAbilityWidget* AbilityWidget = CreateWidget<UP1ShopUniqueAbilityWidget>(GetOwningPlayer(), UniqueAbilityWidgetClass))
			{
				AbilityWidget->SetAbilityData(Ability);
				UniqueAbilitiesContainer->AddChild(AbilityWidget);
			}
		}
	}
}
