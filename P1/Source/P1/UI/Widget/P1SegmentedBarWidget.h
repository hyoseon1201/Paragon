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

	// FillBar(내부 ProgressBar)의 채움 색상을 사용하는 쪽(WBP_P1Overlay에 배치된 각 인스턴스,
	// 또는 위젯 컨트롤러의 런타임 코드)에서 설정할 수 있게 노출 — HealthBar/ManaBar가 이 위젯을
	// 재사용하면서도 서로 다른 색을 쓸 수 있는 이유. Details 패널에서 인스턴스별로 값을 넣거나,
	// SetFillColor()/SetBackgroundColor()를 코드에서 호출해 런타임에 바꿀 수도 있다.
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetFillColor(FLinearColor NewColor);

	// ProgressBar는 Fill과 달리 Background를 단순 색상 프로퍼티로 노출하지 않아서(Style의
	// BackgroundImage 브러시 틴트로만 제어 가능), 내부적으로 WidgetStyle을 복사·수정해 적용한다.
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetBackgroundColor(FLinearColor NewColor);

	// 세그먼트 하나당 수치. 구분선 없이 쓰려면 MaxValue보다 크게 설정 (e.g. 99999).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar", meta = (ClampMin = "1.0"))
	float SegmentSize = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor DividerColor = FLinearColor(0.f, 0.f, 0.f, 0.6f);

	// 인스턴스 배치 시점(Details 패널)에 지정 가능 — NativePreConstruct에서 FillBar에 적용된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor FillColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor BackgroundColor = FLinearColor(0.f, 0.f, 0.f, 0.5f);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> FillBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RegenText;

	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	float CurrentValue = 0.f;
	float MaxValue = 1.f;
	float RegenValue = 0.f;

	void RefreshDisplay();
};
