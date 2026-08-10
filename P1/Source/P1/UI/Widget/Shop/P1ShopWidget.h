// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "Player/P1ShopTypes.h"
#include "P1ShopWidget.generated.h"

class UTextBlock;
class UButton;
class UP1ShopItemListWidget;
class UP1ShopItemDetailWidget;
class UP1ShopStatsWidget;
class UP1ShopInventoryWidget;
class AP1PlayerState;

// 상점 화면 최상위 — 토글 키로 열고 닫는다(AP1PlayerController가 Visibility/입력모드 전환을 담당, 이
// 위젯은 자기 표시 내용만 신경 쓴다). LoL 아레나 상점 참고 3열 레이아웃: 좌측 UP1ShopStatsWidget(현재
// 전투 스탯) / 중앙 UP1ShopItemListWidget(역할군 필터링된 카탈로그 목록)+그 아래 UP1ShopInventoryWidget
// (보유 아이템 6칸, 우클릭 판매) / 우측 UP1ShopItemDetailWidget(선택한 아이템 상세, 더블클릭 구매) —
// 이 클래스 자신은 역할군 탭 버튼 6개를 관리하고 컨트롤러를 자식들에게 전파 + 목록의 선택 이벤트를
// 상세 패널로 전달하는 "배선" 역할만 한다(Scoreboard의 전체→팀→개인 3중 구조와 같은 이유로 분리).
UCLASS()
class P1_API UP1ShopWidget : public UP1UserWidget
{
	GENERATED_BODY()

protected:
	virtual void OnWidgetControllerSet() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	// 역할군 탭 6개 — 아이템 종류가 고정 6개(EP1ItemCategory)라 동적 생성 없이 이름으로 직접 바인딩.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CarryTabButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MageTabButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AssassinTabButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TankTabButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> FighterTabButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SupportTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1ShopItemListWidget> ItemListWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1ShopItemDetailWidget> ItemDetailWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1ShopStatsWidget> StatsWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1ShopInventoryWidget> InventoryWidget;

private:
	UFUNCTION()
	void OnGoldChanged(float NewGold);
	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleCarryTabClicked();
	UFUNCTION()
	void HandleMageTabClicked();
	UFUNCTION()
	void HandleAssassinTabClicked();
	UFUNCTION()
	void HandleTankTabClicked();
	UFUNCTION()
	void HandleFighterTabClicked();
	UFUNCTION()
	void HandleSupportTabClicked();

	UFUNCTION()
	void HandleItemSelected(FName ItemRowName);

	void SetActiveCategory(EP1ItemCategory NewCategory);

	AP1PlayerState* GetOwningP1PlayerState() const;

	EP1ItemCategory ActiveCategory = EP1ItemCategory::Carry;
};
