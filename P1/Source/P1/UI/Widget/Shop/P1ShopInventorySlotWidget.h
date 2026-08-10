// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1ShopInventorySlotWidget.generated.h"

class UImage;
class AP1PlayerState;
struct FP1ShopItemData;

// 상점 중앙하단 인벤토리 한 칸 — 아이콘만 표시하고(이름/가격 없음, 우측 상세 패널이 그 역할을 대신
// 하지 않음 — 애초에 인벤토리 칸은 상세 조회 대상이 아니라 우클릭으로 바로 판매하는 대상), 우클릭
// 하면 즉시 판매한다. ASC/위젯 컨트롤러가 필요 없는 순수 표시+클릭 위젯이라 UP1UserWidget이 아니라
// UUserWidget을 직접 상속(UP1ShopItemSlotWidget과 동일 이유).
UCLASS()
class P1_API UP1ShopInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ItemData가 nullptr이면 빈 슬롯(아이콘 없음, 우클릭해도 아무 일 없음)으로 표시한다 —
	// UP1ShopInventoryWidget이 항상 MaxInventorySlots개를 채우면서 남는 칸에 넘긴다.
	void SetSlotData(AP1PlayerState* InPlayerState, FName InItemRowName, const FP1ShopItemData* ItemData);

protected:
	// 마우스 이벤트를 직접 받아야 해서(우클릭=판매, UButton은 좌클릭만 지원) NativeOnMouseButtonUp을
	// 오버라이드한다 — WBP 루트의 Visibility가 Visible(SelfHitTestInvisible 아님)이어야 이벤트를 받는다.
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

private:
	TWeakObjectPtr<AP1PlayerState> OwningPlayerState;
	FName ItemRowName;
};
