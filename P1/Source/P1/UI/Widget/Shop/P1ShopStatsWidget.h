// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1ShopStatsWidget.generated.h"

class UTextBlock;
class UAbilitySystemComponent;
struct FGameplayAttribute;
struct FOnAttributeChangeData;

// 상점 좌측 스탯 패널 — 아이템 구매/판매로 바뀌는 전투 스탯을 실시간으로 보여준다. 전부
// BindWidgetOptional이라 WBP에 원하는 스탯 TextBlock만 골라 배치해도 된다(이름이 안 맞으면 그냥
// 그 줄만 안 뜸).
UCLASS()
class P1_API UP1ShopStatsWidget : public UP1UserWidget
{
	GENERATED_BODY()

protected:
	virtual void OnWidgetControllerSet() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PhysicalPowerText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AttackSpeedText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MagicalPowerText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PhysicalPenetrationText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MagicalPenetrationText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PhysicalArmorText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MagicalArmorText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MaxHealthText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthRegenText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LifeStealText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MaxManaText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ManaRegenText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AbilityHasteText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TenacityText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CriticalChanceText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CriticalDamageText;

private:
	// TargetText가 WBP에 없으면(BindWidgetOptional이라 null 가능) 조용히 스킵.
	// bAlreadyPercent: 어트리뷰트 값 자체가 이미 퍼센트 스케일(예: AttackSpeed=100이 100%)이라
	// ×100을 하면 안 되는 경우 true로 — AttackSpeed 전용이다(P1GameplayAbility_MeleeAttack/RangedAttack의
	// PlayRate 계산이 이 스케일을 그대로 나눠 쓰기 때문에 어트리뷰트 값 자체를 바꿀 수 없음). 나머지
	// 분수 스케일 %스탯(LifeSteal 등, 0.0=0%)은 false로 둔다.
	void BindStat(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, UTextBlock* TargetText, bool bAsPercent, bool bAlreadyPercent = false);
	void HandleAttributeChanged(const FOnAttributeChangeData& Data, UTextBlock* TargetText, bool bAsPercent, bool bAlreadyPercent);
	static void UpdateStatText(UTextBlock* TargetText, float Value, bool bAsPercent, bool bAlreadyPercent);
};
