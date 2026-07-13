// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1FloatingStatusWidget.h"
#include "UI/WidgetController/P1FloatingStatusWidgetController.h"
#include "UI/Widget/P1SegmentedBarWidget.h"
#include "Components/TextBlock.h"

void UP1FloatingStatusWidget::OnWidgetControllerSet()
{
	UP1FloatingStatusWidgetController* Controller = CastChecked<UP1FloatingStatusWidgetController>(WidgetController);

	Controller->OnHealthChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnHealthChanged);
	Controller->OnMaxHealthChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnMaxHealthChanged);
	Controller->OnManaChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnManaChanged);
	Controller->OnMaxManaChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnMaxManaChanged);
}

void UP1FloatingStatusWidget::SetCharacterName(const FText& NewName)
{
	if (NameText)
	{
		NameText->SetText(NewName);
	}
}

void UP1FloatingStatusWidget::OnHealthChanged(float NewValue)   { CachedHealth = NewValue;    RefreshBars(); }
void UP1FloatingStatusWidget::OnMaxHealthChanged(float NewValue) { CachedMaxHealth = NewValue; RefreshBars(); }
void UP1FloatingStatusWidget::OnManaChanged(float NewValue)      { CachedMana = NewValue;      RefreshBars(); }
void UP1FloatingStatusWidget::OnMaxManaChanged(float NewValue)   { CachedMaxMana = NewValue;   RefreshBars(); }

void UP1FloatingStatusWidget::RefreshBars()
{
	if (HPBar)
	{
		HPBar->SetValues(CachedHealth, CachedMaxHealth);
	}
	if (MPBar)
	{
		MPBar->SetValues(CachedMana, CachedMaxMana);
	}
}
