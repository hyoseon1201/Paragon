// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1ShopUniqueAbilityWidget.generated.h"

class UTextBlock;
struct FP1ItemUniqueAbility;

// 상세 패널 하단 VerticalBox 안의 고유 능력 한 블록("절단" 이름+설명) — UP1ShopItemDetailWidget이
// FP1ShopItemData::UniqueAbilities 개수만큼 반복 생성한다. 순수 표시 위젯.
UCLASS()
class P1_API UP1ShopUniqueAbilityWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetAbilityData(const FP1ItemUniqueAbility& Ability);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AbilityNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AbilityDescriptionText;
};
