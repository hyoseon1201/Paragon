// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/P1WidgetController.h"
#include "GameplayEffectTypes.h"
#include "P1OverlayWidgetController.generated.h"

// 인게임 HUD(HP·MP·리젠·스킬슬롯 등)용 WidgetController.
// ASC 속성 변경을 구독하고 위젯에 브로드캐스트한다.
UCLASS(BlueprintType)
class P1_API UP1OverlayWidgetController : public UP1WidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	// ---- Health ----
	UPROPERTY(BlueprintAssignable, Category = "Events|Health")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Health")
	FOnAttributeChangedSignature OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Health")
	FOnAttributeChangedSignature OnHealthRegenChanged;

	// ---- Mana ----
	UPROPERTY(BlueprintAssignable, Category = "Events|Mana")
	FOnAttributeChangedSignature OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Mana")
	FOnAttributeChangedSignature OnMaxManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Mana")
	FOnAttributeChangedSignature OnManaRegenChanged;

private:
	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void OnHealthRegenAttributeChanged(const FOnAttributeChangeData& Data);
	void OnManaAttributeChanged(const FOnAttributeChangeData& Data);
	void OnMaxManaAttributeChanged(const FOnAttributeChangeData& Data);
	void OnManaRegenAttributeChanged(const FOnAttributeChangeData& Data);
};
