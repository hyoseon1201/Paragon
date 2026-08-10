// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1ShopStatChipWidget.generated.h"

class UTextBlock;
struct FP1ItemDisplayStat;

// 상세 패널 하단 WrapBox 안의 스탯 칩 하나("+45 물리 공격력" 식) — UP1ShopItemDetailWidget이
// FP1ShopItemData::DisplayStats 개수만큼 반복 생성한다. 순수 표시 위젯이라 컨트롤러도, PlayerState
// 참조도 필요 없다(UP1ShopItemSlotWidget과 달리 클릭 인터랙션이 없음).
UCLASS()
class P1_API UP1ShopStatChipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetStatData(const FP1ItemDisplayStat& Stat);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText;
};
