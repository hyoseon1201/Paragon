// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1HUDInventorySlotWidget.generated.h"

class UImage;
struct FP1ShopItemData;

// 메인 HUD 우측하단 인벤토리 한 칸 — 아이콘만 보여주는 완전 읽기전용 위젯(클릭 인터랙션 없음).
// 판매는 오직 상점 화면(UP1ShopInventorySlotWidget)의 우클릭으로만 가능하다 — 상점을 안 열어도
// 보이는 이 HUD 버전에서까지 팔 수 있게 하면 실수로 파는 사고가 나기 쉬워서 의도적으로 뺐다.
UCLASS()
class P1_API UP1HUDInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ItemData가 nullptr이면 빈 슬롯(아이콘 없음)으로 표시한다.
	void SetSlotData(const FP1ShopItemData* ItemData);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;
};
