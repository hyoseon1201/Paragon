// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Shop/P1ShopStatChipWidget.h"
#include "Components/TextBlock.h"
#include "Player/P1ShopTypes.h"

void UP1ShopStatChipWidget::SetStatData(const FP1ItemDisplayStat& Stat)
{
	if (!ValueText)
	{
		return;
	}

	ValueText->SetText(Stat.bAsPercent
		? FText::FromString(FString::Printf(TEXT("+%.0f%% %s"), Stat.Value, *Stat.Label.ToString()))
		: FText::FromString(FString::Printf(TEXT("+%.0f %s"), Stat.Value, *Stat.Label.ToString())));
}
