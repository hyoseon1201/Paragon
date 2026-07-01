// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1FloatingStatusWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UP1FloatingStatusWidgetController;

// 캐릭터 머리 위 월드스페이스 위젯. 레벨 · HP · MP 바를 표시.
UCLASS()
class P1_API UP1FloatingStatusWidget : public UP1UserWidget
{
	GENERATED_BODY()

protected:
	virtual void OnWidgetControllerSet() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> MPBar;

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
