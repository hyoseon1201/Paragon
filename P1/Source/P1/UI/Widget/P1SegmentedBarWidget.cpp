// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1SegmentedBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"

int32 UP1SegmentedBarWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// ProgressBar + TextBlock을 먼저 그린 뒤, 구분선을 그 위에 덧그린다.
	const int32 ChildLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const int32 TotalSegments = FMath::Max(1, FMath::CeilToInt(MaxValue / SegmentSize));
	if (TotalSegments <= 1)
	{
		return ChildLayerId;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FSlateBrush* WhiteBrush = FAppStyle::GetBrush("WhiteBrush");
	constexpr float DividerW = 2.f;

	for (int32 i = 1; i < TotalSegments; ++i)
	{
		const float DividerX = (LocalSize.X / TotalSegments) * i - DividerW * 0.5f;

		FSlateDrawElement::MakeBox(OutDrawElements, ChildLayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2D(DividerW, LocalSize.Y),
				FSlateLayoutTransform(FVector2D(DividerX, 0.f))),
			WhiteBrush, ESlateDrawEffect::None, DividerColor);
	}

	return ChildLayerId + 1;
}

void UP1SegmentedBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshDisplay();
}

void UP1SegmentedBarWidget::SetValues(float NewCurrent, float NewMax, float NewRegen)
{
	CurrentValue = FMath::Max(0.f, NewCurrent);
	MaxValue     = FMath::Max(1.f, NewMax);
	RegenValue   = NewRegen;
	RefreshDisplay();
}

void UP1SegmentedBarWidget::RefreshDisplay()
{
	if (FillBar)
	{
		FillBar->SetPercent(CurrentValue / MaxValue);
	}
	if (ValueText)
	{
		ValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), CurrentValue, MaxValue)));
	}
	if (RegenText)
	{
		RegenText->SetText(FText::FromString(
			FString::Printf(TEXT("+%.1f/s"), RegenValue)));
	}
}
