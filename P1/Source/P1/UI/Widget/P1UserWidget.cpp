// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1UserWidget.h"

void UP1UserWidget::SetWidgetController(UP1WidgetController* InWidgetController)
{
	WidgetController = InWidgetController;
	OnWidgetControllerSet();
}
