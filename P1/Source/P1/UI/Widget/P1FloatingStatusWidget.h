// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1FloatingStatusWidget.generated.h"

class UTextBlock;
class UP1SegmentedBarWidget;
class UP1FloatingStatusWidgetController;

// 캐릭터 머리 위 월드스페이스 위젯. 레벨 · HP · MP 바를 표시.
// HP/MP 바는 WBP_P1Overlay의 HealthBar/ManaBar와 동일한 UP1SegmentedBarWidget을 재사용한다 —
// 같은 WBP 에셋을 여기 배치할 때 인스턴스 프로퍼티에서 bShowLabels=false로 두면 숫자 텍스트만 숨겨진다.
UCLASS()
class P1_API UP1FloatingStatusWidget : public UP1UserWidget
{
	GENERATED_BODY()

public:
	// 캐릭터 이름(예: "Greystone") 표시 — Health/Mana처럼 계속 바뀌는 값이 아니라 한 번만 세팅하면
	// 되므로 위젯 컨트롤러 델리게이트를 거치지 않고 캐릭터 쪽에서 직접 호출한다.
	UFUNCTION(BlueprintCallable, Category = "FloatingStatus")
	void SetCharacterName(const FText& NewName);

protected:
	virtual void OnWidgetControllerSet() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1SegmentedBarWidget> HPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1SegmentedBarWidget> MPBar;

private:
	UFUNCTION()
	void OnHealthChanged(float NewValue);
	UFUNCTION()
	void OnMaxHealthChanged(float NewValue);
	UFUNCTION()
	void OnManaChanged(float NewValue);
	UFUNCTION()
	void OnMaxManaChanged(float NewValue);

	void RefreshBars();

	float CachedHealth = 1.f;
	float CachedMaxHealth = 1.f;
	float CachedMana = 0.f;
	float CachedMaxMana = 1.f;
};
