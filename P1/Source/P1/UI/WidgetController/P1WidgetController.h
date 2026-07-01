// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "P1WidgetController.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;

// WidgetController 전체에서 공용으로 쓰는 단일값 델리게이트.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedSignature, float, NewValue);

// AP1HUD::GetXxxWidgetController() 에 전달하는 파라미터 묶음.
USTRUCT(BlueprintType)
struct FWidgetControllerParams
{
	GENERATED_BODY()

	FWidgetControllerParams() {}
	FWidgetControllerParams(APlayerController* PC, APlayerState* PS,
		UAbilitySystemComponent* InASC, UAttributeSet* InAS)
		: PlayerController(PC), PlayerState(PS), AbilitySystemComponent(InASC), AttributeSet(InAS) {}

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAttributeSet> AttributeSet = nullptr;
};

// 모든 WidgetController의 베이스.
// 파라미터를 받아 저장하고, 두 단계 초기화(바인딩 → 초기값 브로드캐스트)를 제공한다.
UCLASS(BlueprintType)
class P1_API UP1WidgetController : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "WidgetController")
	void SetWidgetControllerParams(const FWidgetControllerParams& WCParams);

	virtual void BroadcastInitialValues() {}
	virtual void BindCallbacksToDependencies() {}

protected:
	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<APlayerState> PlayerState;

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<UAttributeSet> AttributeSet;
};
