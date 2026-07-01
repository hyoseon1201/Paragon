// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/P1WidgetController.h"
#include "GameplayEffectTypes.h"
#include "P1FloatingStatusWidgetController.generated.h"

// 캐릭터 머리 위 월드스페이스 위젯용 컨트롤러.
// AP1CharacterBase가 per-instance로 생성하며 AP1HUD와 무관하다.
UCLASS(BlueprintType)
class P1_API UP1FloatingStatusWidgetController : public UP1WidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAttributeChangedSignature OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAttributeChangedSignature OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAttributeChangedSignature OnMaxManaChanged;

private:
	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void OnManaAttributeChanged(const FOnAttributeChangeData& Data);
	void OnMaxManaAttributeChanged(const FOnAttributeChangeData& Data);
};
