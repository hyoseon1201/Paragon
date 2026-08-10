// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "P1WidgetController.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class AP1PlayerState;

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

	// 타입 캐스팅된 PlayerState 접근자 — 모든 서브클래스(Overlay/Shop/Scoreboard 등)가 공용으로 쓴다.
	// 자식 위젯이 PlayerState의 게터/네이티브 델리게이트(GetInventory, OnInventoryChangedNative 등)에
	// 직접 접근하고 싶을 때 굳이 컨트롤러마다 같은 Cast를 중복 구현하지 않도록 여기 하나로 모아둔다.
	AP1PlayerState* GetP1PlayerState() const;

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
