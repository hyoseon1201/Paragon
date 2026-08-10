// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Shop/P1ShopUniqueAbilityWidget.h"
#include "Components/TextBlock.h"
#include "Player/P1ShopTypes.h"

void UP1ShopUniqueAbilityWidget::SetAbilityData(const FP1ItemUniqueAbility& Ability)
{
	if (AbilityNameText)
	{
		AbilityNameText->SetText(Ability.AbilityName);
	}
	if (AbilityDescriptionText)
	{
		AbilityDescriptionText->SetText(Ability.AbilityDescription);
	}
}
