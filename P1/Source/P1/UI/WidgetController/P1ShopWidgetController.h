// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/P1WidgetController.h"
#include "GameplayEffectTypes.h"
#include "P1ShopWidgetController.generated.h"

class AP1PlayerState;
class UAbilitySystemComponent;

// 골드 변경 — 카탈로그/보유 아이템 칸이 "지금 살 수 있는지" 등을 표시하고 싶어질 수 있어 함께 알린다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopGoldChangedSignature, float, NewGold);

// 인벤토리(보유 아이템) 변경 — 구매/판매 성공 시 브로드캐스트. 페이로드 없이 "다시 읽어라"는 신호만
// 준다 — 위젯이 PlayerState->GetInventory()로 직접 재조회한다(항목 하나하나를 델리게이트로 실어
// 나르기보다 통짜로 다시 그리는 게 간단하고, 보유 슬롯 개수가 최대 6개라 비용도 무시할 수준).
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopInventoryChangedSignature);

// 상점 화면(WBP_Shop) 전용 WidgetController — 골드/인벤토리 변경을 구독해 브로드캐스트한다.
// 카탈로그(ShopItemTable) 자체는 정적 데이터라 델리게이트 없이 위젯이 PlayerState에서 직접 1회 읽는다.
UCLASS(BlueprintType)
class P1_API UP1ShopWidgetController : public UP1WidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	// UP1ShopStatsWidget이 각 전투 스탯 속성 변경 델리게이트를 직접 구독하기 위한 접근자
	// (베이스 클래스의 protected 멤버를 노출만 함, Blueprint에 값을 실어 나를 필요는 없어 별도
	// 델리게이트 없이 네이티브 델리게이트를 각 위젯이 직접 구독하게 한다).
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	UPROPERTY(BlueprintAssignable, Category = "Events|Shop")
	FOnShopGoldChangedSignature OnGoldChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Shop")
	FOnShopInventoryChangedSignature OnInventoryChanged;

private:
	void OnGoldAttributeChanged(const FOnAttributeChangeData& Data);
	void OnInventoryChanged_Internal();
};
