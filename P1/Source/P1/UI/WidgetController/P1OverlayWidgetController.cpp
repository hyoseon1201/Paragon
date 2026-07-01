// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/WidgetController/P1OverlayWidgetController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/P1AttributeSet.h"

void UP1OverlayWidgetController::BroadcastInitialValues()
{
	bool bFound = false;
	OnHealthChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetHealthAttribute(), bFound));
	OnMaxHealthChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetMaxHealthAttribute(), bFound));
	OnHealthRegenChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetHealthRegenAttribute(), bFound));
	OnManaChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetManaAttribute(), bFound));
	OnMaxManaChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetMaxManaAttribute(), bFound));
	OnManaRegenChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetManaRegenAttribute(), bFound));
}

void UP1OverlayWidgetController::BindCallbacksToDependencies()
{
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetHealthAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnHealthAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnMaxHealthAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetHealthRegenAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnHealthRegenAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetManaAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnManaAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetMaxManaAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnMaxManaAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetManaRegenAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnManaRegenAttributeChanged);
}

void UP1OverlayWidgetController::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)    { OnHealthChanged.Broadcast(Data.NewValue); }
void UP1OverlayWidgetController::OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data)  { OnMaxHealthChanged.Broadcast(Data.NewValue); }
void UP1OverlayWidgetController::OnHealthRegenAttributeChanged(const FOnAttributeChangeData& Data){ OnHealthRegenChanged.Broadcast(Data.NewValue); }
void UP1OverlayWidgetController::OnManaAttributeChanged(const FOnAttributeChangeData& Data)       { OnManaChanged.Broadcast(Data.NewValue); }
void UP1OverlayWidgetController::OnMaxManaAttributeChanged(const FOnAttributeChangeData& Data)    { OnMaxManaChanged.Broadcast(Data.NewValue); }
void UP1OverlayWidgetController::OnManaRegenAttributeChanged(const FOnAttributeChangeData& Data)  { OnManaRegenChanged.Broadcast(Data.NewValue); }
