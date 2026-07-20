// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1ShopTypes.generated.h"

class UTexture2D;
class UGameplayEffect;

// 상점 아이템 카탈로그 한 줄 — DT_ShopItems 같은 DataTable의 Row로 임포트한다(RowName=아이템 ID).
// LoL 아레나 스타일: 컴포넌트 합성 없이 완성 아이템만 직접 구매(AP1PlayerState 참고).
USTRUCT(BlueprintType)
struct P1_API FP1ShopItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	int32 Price = 0;

	// 판매 시 돌려받는 골드. 0이면 판매 불가(소모품 등)로 취급 — AP1PlayerState::ServerSellItem 참고.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	int32 SellPrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UTexture2D> Icon;

	// Duration=Infinite, Attribute Modifier 여러 개(AttackSpeed/PhysicalPower/MagicalArmor/Tenacity 등) —
	// 아이템이 주는 "깡스탯"을 전부 이 GE 하나에 담는다. 구매 시 자신에게 적용, 판매 시 그 핸들로 제거
	// (AP1PlayerState::ActiveItemStatEffects가 아이템ID→핸들을 추적). 리액티브 패시브(CC 당하면 발동 등)는
	// CC 시스템 자체가 아직 없어 별도 설계 필요 — 지금은 깡스탯만 다룬다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<UGameplayEffect> StatEffectClass;
};
