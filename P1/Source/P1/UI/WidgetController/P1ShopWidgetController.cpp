// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/WidgetController/P1ShopWidgetController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "Player/P1PlayerState.h"

UAbilitySystemComponent* UP1ShopWidgetController::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void UP1ShopWidgetController::BroadcastInitialValues()
{
	bool bFound = false;
	OnGoldChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetGoldAttribute(), bFound));
	OnInventoryChanged.Broadcast();
}

void UP1ShopWidgetController::BindCallbacksToDependencies()
{
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetGoldAttribute())
		.AddUObject(this, &UP1ShopWidgetController::OnGoldAttributeChanged);

	if (AP1PlayerState* P1PS = Cast<AP1PlayerState>(PlayerState))
	{
		P1PS->OnInventoryChangedNative.AddUObject(this, &UP1ShopWidgetController::OnInventoryChanged_Internal);
	}
}

void UP1ShopWidgetController::OnGoldAttributeChanged(const FOnAttributeChangeData& Data)
{
	OnGoldChanged.Broadcast(Data.NewValue);
}

void UP1ShopWidgetController::OnInventoryChanged_Internal()
{
	OnInventoryChanged.Broadcast();
}
