// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/HUD/P1HUDWidget.h"
#include "UI/WidgetController/P1OverlayWidgetController.h"
#include "UI/Widget/P1SegmentedBarWidget.h"

void UP1HUDWidget::OnWidgetControllerSet()
{
	UP1OverlayWidgetController* Controller = CastChecked<UP1OverlayWidgetController>(WidgetController);

	Controller->OnHealthChanged.AddDynamic(this, &UP1HUDWidget::OnHealthChanged);
	Controller->OnMaxHealthChanged.AddDynamic(this, &UP1HUDWidget::OnMaxHealthChanged);
	Controller->OnHealthRegenChanged.AddDynamic(this, &UP1HUDWidget::OnHealthRegenChanged);
	Controller->OnManaChanged.AddDynamic(this, &UP1HUDWidget::OnManaChanged);
	Controller->OnMaxManaChanged.AddDynamic(this, &UP1HUDWidget::OnMaxManaChanged);
	Controller->OnManaRegenChanged.AddDynamic(this, &UP1HUDWidget::OnManaRegenChanged);
}

void UP1HUDWidget::OnHealthChanged(float NewValue)    { CachedHealth = NewValue;     RefreshHealthDisplay(); }
void UP1HUDWidget::OnMaxHealthChanged(float NewValue)  { CachedMaxHealth = NewValue;  RefreshHealthDisplay(); }
void UP1HUDWidget::OnHealthRegenChanged(float NewValue){ CachedHealthRegen = NewValue; RefreshHealthDisplay(); }
void UP1HUDWidget::OnManaChanged(float NewValue)       { CachedMana = NewValue;       RefreshManaDisplay(); }
void UP1HUDWidget::OnMaxManaChanged(float NewValue)    { CachedMaxMana = NewValue;    RefreshManaDisplay(); }
void UP1HUDWidget::OnManaRegenChanged(float NewValue)  { CachedManaRegen = NewValue;  RefreshManaDisplay(); }

void UP1HUDWidget::RefreshHealthDisplay()
{
	if (HealthBar)
	{
		HealthBar->SetValues(CachedHealth, CachedMaxHealth, CachedHealthRegen);
	}
}

void UP1HUDWidget::RefreshManaDisplay()
{
	if (ManaBar)
	{
		ManaBar->SetValues(CachedMana, CachedMaxMana, CachedManaRegen);
	}
}
