// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Shop/P1ShopStatsWidget.h"
#include "UI/WidgetController/P1ShopWidgetController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "Components/TextBlock.h"

void UP1ShopStatsWidget::OnWidgetControllerSet()
{
	UP1ShopWidgetController* Controller = CastChecked<UP1ShopWidgetController>(WidgetController);
	UAbilitySystemComponent* ASC = Controller->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	BindStat(ASC, UP1AttributeSet::GetPhysicalPowerAttribute(), PhysicalPowerText, false);
	BindStat(ASC, UP1AttributeSet::GetAttackSpeedAttribute(), AttackSpeedText, true, /*bAlreadyPercent=*/true);
	BindStat(ASC, UP1AttributeSet::GetMagicalPowerAttribute(), MagicalPowerText, false);
	BindStat(ASC, UP1AttributeSet::GetPhysicalPenetrationAttribute(), PhysicalPenetrationText, false);
	BindStat(ASC, UP1AttributeSet::GetMagicalPenetrationAttribute(), MagicalPenetrationText, false);
	BindStat(ASC, UP1AttributeSet::GetPhysicalArmorAttribute(), PhysicalArmorText, false);
	BindStat(ASC, UP1AttributeSet::GetMagicalArmorAttribute(), MagicalArmorText, false);
	BindStat(ASC, UP1AttributeSet::GetMaxHealthAttribute(), MaxHealthText, false);
	BindStat(ASC, UP1AttributeSet::GetHealthRegenAttribute(), HealthRegenText, false);
	BindStat(ASC, UP1AttributeSet::GetLifeStealAttribute(), LifeStealText, true);
	BindStat(ASC, UP1AttributeSet::GetMaxManaAttribute(), MaxManaText, false);
	BindStat(ASC, UP1AttributeSet::GetManaRegenAttribute(), ManaRegenText, false);
	BindStat(ASC, UP1AttributeSet::GetAbilityHasteAttribute(), AbilityHasteText, false);
	BindStat(ASC, UP1AttributeSet::GetTenacityAttribute(), TenacityText, true);
	BindStat(ASC, UP1AttributeSet::GetCriticalChanceAttribute(), CriticalChanceText, true);
	BindStat(ASC, UP1AttributeSet::GetCriticalDamageAttribute(), CriticalDamageText, true);
}

void UP1ShopStatsWidget::BindStat(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, UTextBlock* TargetText, bool bAsPercent, bool bAlreadyPercent)
{
	if (!TargetText)
	{
		return;
	}

	bool bFound = false;
	UpdateStatText(TargetText, ASC->GetGameplayAttributeValue(Attribute, bFound), bAsPercent, bAlreadyPercent);

	ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(
		this, &UP1ShopStatsWidget::HandleAttributeChanged, TargetText, bAsPercent, bAlreadyPercent);
}

void UP1ShopStatsWidget::HandleAttributeChanged(const FOnAttributeChangeData& Data, UTextBlock* TargetText, bool bAsPercent, bool bAlreadyPercent)
{
	UpdateStatText(TargetText, Data.NewValue, bAsPercent, bAlreadyPercent);
}

void UP1ShopStatsWidget::UpdateStatText(UTextBlock* TargetText, float Value, bool bAsPercent, bool bAlreadyPercent)
{
	if (bAsPercent)
	{
		const float DisplayValue = bAlreadyPercent ? Value : Value * 100.0f;
		TargetText->SetText(FText::FromString(FString::Printf(TEXT("%.0f%%"), DisplayValue)));
	}
	else
	{
		TargetText->SetText(FText::AsNumber(FMath::RoundToInt(Value)));
	}
}
