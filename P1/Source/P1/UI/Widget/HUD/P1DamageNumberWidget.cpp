// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/HUD/P1DamageNumberWidget.h"
#include "Components/TextBlock.h"

void UP1DamageNumberWidget::SetDamageAmount(float Amount, bool bIsMagicalDamage)
{
	if (DamageText)
	{
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Amount)));
		DamageText->SetColorAndOpacity(bIsMagicalDamage ? MagicalColor : PhysicalColor);
	}
}
