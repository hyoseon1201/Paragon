// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1ShopItemSlotWidget.generated.h"

class UTextBlock;
class UImage;
class UButton;
class UWidget;
class AP1PlayerState;
struct FP1ShopItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopItemSlotSelected, FName, ItemRowName);

// 상점 좌측 목록의 아이템 한 칸 — 클릭하면 선택만 하고(FOnShopItemSlotSelected 브로드캐스트) 오른쪽
// 상세정보 패널에 표시되며, **더블클릭하면 바로 구매**한다(ServerBuyItem). ASC/위젯 컨트롤러가
// 필요 없는 순수 표시+클릭 위젯이라 UP1UserWidget이 아니라 UUserWidget을 직접 상속
// (UP1ScoreboardPlayerRowWidget과 동일 이유) — 구매 자체엔 컨트롤러가 필요 없고 PlayerState
// 참조 하나면 충분해서 SetSlotData()로 직접 받는다.
UCLASS()
class P1_API UP1ShopItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 카탈로그 목록을 채울 때마다 호출. bInOwned는 보유 배지 표시 여부만 결정한다(가격은 항상 구매가).
	void SetSlotData(AP1PlayerState* InPlayerState, FName InItemRowName, const FP1ShopItemData& ItemData, bool bInOwned);

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnShopItemSlotSelected OnSelected;

protected:
	virtual void NativeConstruct() override;

	// 더블클릭으로 구매 — WBP 루트의 Visibility가 Visible이어야 이벤트를 받는다(SelfHitTestInvisible이면 안 들어옴).
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	// 아이콘 하나만으로 충분한 디자인이면 WBP에 안 둬도 된다(이름은 우측 상세 패널이 이미 보여줌).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	// 이 칸을 선택하는 버튼 — 배경 전체를 덮는 투명 버튼으로 만들면 자연스럽다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SelectButton;

	// 보유 중인 아이템 위에 표시할 배지(체크 아이콘 등) — 임의 위젯 타입이라 UWidget으로 받아 Visibility만 토글.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> OwnedBadge;

private:
	UFUNCTION()
	void HandleSelectClicked();

	TWeakObjectPtr<AP1PlayerState> OwningPlayerState;
	FName ItemRowName;
};
