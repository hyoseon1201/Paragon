// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1ShopTypes.generated.h"

class UTexture2D;
class UGameplayEffect;
class UGameplayAbility;

// 상점 좌측 목록의 역할군 탭 — 아이템 하나가 여러 역할군에 걸칠 수 있어 FP1ShopItemData::Categories는 배열.
UENUM(BlueprintType)
enum class EP1ItemCategory : uint8
{
	Carry,
	Mage,
	Assassin,
	Tank,
	Fighter,
	Support
};

// 상세 패널에 "+45 물리 공격력" 식으로 나열할 스탯 한 줄 — 순수 표시용이라 FGameplayAttribute가
// 아니라 텍스트 라벨을 직접 받는다(FGameplayAttribute는 리플렉션 기반이라 DataTable JSON 임포트로
// 못 채움). 실제 스탯 적용은 아래 StatEffectClass GE가 담당하므로, 이 배열의 수치는 그 GE의
// Modifier 값과 사람이 직접 맞춰야 한다(자동 동기화 없음).
USTRUCT(BlueprintType)
struct P1_API FP1ItemDisplayStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	float Value = 0.0f;

	// true면 "+12%" 식으로, false면 "+45" 식으로 표시(옴니뱀프/강인함처럼 퍼센트 스탯인 아이템용).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	bool bAsPercent = false;
};

// 상세 패널에 나열할 고유 능력 하나("절단"/"디보어" 같은 이름표 붙은 특수효과) — 깡스탯과 달리
// 이름+설명이 각각 따로 있고, 실제 적용도 별도 GE(EffectClass)로 분리한다(깡스탯 GE에 다 우겨넣지
// 않는 이유: 나중에 고유 능력별로 발동 조건이 생기면 — 예: 기본 공격 시에만 — 그 GE 하나만 손보면
// 됨). 아이템 하나가 고유 능력을 0개~여러 개 가질 수 있어 배열.
USTRUCT(BlueprintType)
struct P1_API FP1ItemUniqueAbility
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FText AbilityName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FText AbilityDescription;

	// 조건 없이 즉시 적용되는(패시브 스탯/디버프 등) 고유 능력이면 이 GE를 구매 시 적용, 판매 시 제거
	// (AP1PlayerState::ActiveItemEffects 참고). "기본 공격 적중 시에만 발동" 같은 반응형 고유 능력은
	// 이 필드 대신 아래 TriggeredAbilityClass를 쓴다 — 둘 다 채울 필요는 없다(대부분 둘 중 하나만 사용).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<UGameplayEffect> EffectClass;

	// 반응형(온-히트 등) 고유 능력이면 이 어빌리티를 구매 시 GiveAbility로 부여, 판매 시 ClearAbility로
	// 회수한다(AP1PlayerState::ActiveItemAbilities 참고) — 어빌리티의 존재 자체가 "이 아이템을 갖고
	// 있다"는 신호다. UP1GameplayAbility_OnHitItemAbility 파생 클래스가 이 용도의 표준 베이스
	// (Event.Character.BasicAttackHitDealt 이벤트로 트리거, 크로노 스트라이크/매너스가 첫 사용처).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<UGameplayAbility> TriggeredAbilityClass;
};

// 상점 아이템 카탈로그 한 줄 — DT_ShopItems 같은 DataTable의 Row로 임포트한다(RowName=아이템 ID).
// LoL 아레나 스타일: 컴포넌트 합성 없이 완성 아이템만 직접 구매(AP1PlayerState 참고).
USTRUCT(BlueprintType)
struct P1_API FP1ShopItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FText DisplayName;

	// 이 아이템이 속하는 역할군(들) — 왼쪽 목록의 역할군 탭 필터링에 쓰인다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TArray<EP1ItemCategory> Categories;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	int32 Price = 0;

	// 판매 시 돌려받는 골드. 0이면 판매 불가(소모품 등)로 취급 — AP1PlayerState::ServerSellItem 참고.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	int32 SellPrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UTexture2D> Icon;

	// 상세 패널에 나열할 스탯 목록(표시용) — 실제 적용은 아래 StatEffectClass가 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TArray<FP1ItemDisplayStat> DisplayStats;

	// Duration=Infinite, Attribute Modifier 여러 개(AttackSpeed/PhysicalPower/MagicalArmor/Tenacity 등) —
	// 아이템이 주는 "깡스탯"을 전부 이 GE 하나에 담는다. 구매 시 자신에게 적용, 판매 시 그 핸들로 제거
	// (AP1PlayerState::ActiveItemEffects가 아이템ID→핸들 목록을 추적).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<UGameplayEffect> StatEffectClass;

	// "절단"/"디보어" 같은 이름 붙은 고유 능력들 — 0개 이상, 각각 자기 GE로 따로 적용/제거된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TArray<FP1ItemUniqueAbility> UniqueAbilities;
};
