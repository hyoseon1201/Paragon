// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1FloatingStatusWidget.h"
#include "UI/WidgetController/P1FloatingStatusWidgetController.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UP1FloatingStatusWidget::OnWidgetControllerSet()
{
	UP1FloatingStatusWidgetController* Controller = CastChecked<UP1FloatingStatusWidgetController>(WidgetController);

	Controller->OnHealthChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnHealthChanged);
	Controller->OnMaxHealthChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnMaxHealthChanged);
	Controller->OnManaChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnManaChanged);
	Controller->OnMaxManaChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnMaxManaChanged);
}

void UP1FloatingStatusWidget::OnHealthChanged(float NewValue)   { CachedHealth = NewValue;    RefreshBars(); }
void UP1FloatingStatusWidget::OnMaxHealthChanged(float NewValue) { CachedMaxHealth = NewValue; RefreshBars(); }
void UP1FloatingStatusWidget::OnManaChanged(float NewValue)      { CachedMana = NewValue;      RefreshBars(); }
void UP1FloatingStatusWidget::OnMaxManaChanged(float NewValue)   { CachedMaxMana = NewValue;   RefreshBars(); }

void UP1FloatingStatusWidget::RefreshBars()
{
	if (HPBar)
	{
		HPBar->SetPercent(CachedMaxHealth > 0.f ? CachedHealth / CachedMaxHealth : 0.f);
	}
	if (MPBar)
	{
		MPBar->SetPercent(CachedMaxMana > 0.f ? CachedMana / CachedMaxMana : 0.f);
	}
}
