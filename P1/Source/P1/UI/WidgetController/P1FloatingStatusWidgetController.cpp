// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/WidgetController/P1FloatingStatusWidgetController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/P1AttributeSet.h"

void UP1FloatingStatusWidgetController::BroadcastInitialValues()
{
	bool bFound = false;
	OnHealthChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetHealthAttribute(), bFound));
	OnMaxHealthChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetMaxHealthAttribute(), bFound));
	OnManaChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetManaAttribute(), bFound));
	OnMaxManaChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetMaxManaAttribute(), bFound));
}

void UP1FloatingStatusWidgetController::BindCallbacksToDependencies()
{
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetHealthAttribute())
		.AddUObject(this, &UP1FloatingStatusWidgetController::OnHealthAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UP1FloatingStatusWidgetController::OnMaxHealthAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetManaAttribute())
		.AddUObject(this, &UP1FloatingStatusWidgetController::OnManaAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetMaxManaAttribute())
		.AddUObject(this, &UP1FloatingStatusWidgetController::OnMaxManaAttributeChanged);
}

void UP1FloatingStatusWidgetController::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)   { OnHealthChanged.Broadcast(Data.NewValue); }
void UP1FloatingStatusWidgetController::OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data){ OnMaxHealthChanged.Broadcast(Data.NewValue); }
void UP1FloatingStatusWidgetController::OnManaAttributeChanged(const FOnAttributeChangeData& Data)     { OnManaChanged.Broadcast(Data.NewValue); }
void UP1FloatingStatusWidgetController::OnMaxManaAttributeChanged(const FOnAttributeChangeData& Data)  { OnMaxManaChanged.Broadcast(Data.NewValue); }
