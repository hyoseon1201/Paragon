// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Auth/P1HeroSelectSlotWidget.h"
#include "Characters/P1HeroTypes.h"
#include "Characters/P1HeroCharacter.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"

void UP1HeroSelectSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddDynamic(this, &UP1HeroSelectSlotWidget::HandleSelectClicked);
	}
}

void UP1HeroSelectSlotWidget::SetSlotData(const FP1HeroDefinition& Row)
{
	HeroId = Row.HeroId;

	const AP1HeroCharacter* HeroCDO = Row.HeroClass ? Row.HeroClass->GetDefaultObject<AP1HeroCharacter>() : nullptr;

	if (PortraitImage && HeroCDO && HeroCDO->GetHeroPortrait())
	{
		PortraitImage->SetBrushFromTexture(HeroCDO->GetHeroPortrait());
	}

	SetSelectedVisual(false);
}

void UP1HeroSelectSlotWidget::SetSelectedVisual(bool bSelected)
{
	if (PortraitImage)
	{
		PortraitImage->SetColorAndOpacity(bSelected ? FLinearColor::White : UnselectedTint);
	}
}

void UP1HeroSelectSlotWidget::HandleSelectClicked()
{
	if (HeroId.IsNone())
	{
		return;
	}

	OnSelected.Broadcast(HeroId);
}
