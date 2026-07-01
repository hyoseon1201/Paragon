// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1SegmentedBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

// 체력/마나 바. WBP에서 FillBar(ProgressBar), ValueText/RegenText(TextBlock)를 배치하고
// SetValues()로 데이터를 넘기면 C++가 퍼센트와 텍스트를 업데이트한다.
// 구분선은 NativePaint로 ProgressBar 위에 덧그리며, 색상/크기는 WBP 디테일에서 조정한다.
UCLASS()
class P1_API UP1SegmentedBarWidget : public UP1UserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Bar")
	void SetValues(float NewCurrent, float NewMax, float NewRegen = 0.f);

	// 세그먼트 하나당 수치. 구분선 없이 쓰려면 MaxValue보다 크게 설정 (e.g. 99999).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar", meta = (ClampMin = "1.0"))
	float SegmentSize = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor DividerColor = FLinearColor(0.f, 0.f, 0.f, 0.6f);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> FillBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RegenText;

	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	float CurrentValue = 0.f;
	float MaxValue = 1.f;
	float RegenValue = 0.f;

	void RefreshDisplay();
};
