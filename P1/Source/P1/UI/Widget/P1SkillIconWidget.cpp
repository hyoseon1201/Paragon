// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1SkillIconWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"

void UP1SkillIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CooldownOverlay)
	{
		// WBP에서 CooldownOverlay의 Brush Material로 설정된 머터리얼로 MID 생성
		if (UMaterialInterface* Mat = Cast<UMaterialInterface>(CooldownOverlay->GetBrush().GetResourceObject()))
		{
			CooldownMID = UMaterialInstanceDynamic::Create(Mat, this);
			CooldownOverlay->SetBrushFromMaterial(CooldownMID);
		}
		CooldownOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CooldownText)
	{
		CooldownText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UP1SkillIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bOnCooldown) return;

	RemainingCooldown -= InDeltaTime;
	if (RemainingCooldown <= 0.f)
	{
		ClearCooldown();
		return;
	}
	UpdateCooldownDisplay();
}

void UP1SkillIconWidget::SetSkillIcon(UTexture2D* IconTexture)
{
	if (SkillIconImage && IconTexture)
	{
		SkillIconImage->SetBrushFromTexture(IconTexture);
	}
}

void UP1SkillIconWidget::StartCooldown(float TotalCooldown)
{
	TotalCooldownTime = FMath::Max(0.01f, TotalCooldown);
	RemainingCooldown = TotalCooldownTime;
	bOnCooldown = true;
	UpdateCooldownDisplay();
}

void UP1SkillIconWidget::ClearCooldown()
{
	bOnCooldown = false;
	RemainingCooldown = 0.f;

	if (CooldownMID)
	{
		CooldownMID->SetScalarParameterValue(TEXT("CooldownPercent"), 0.f);
	}
	if (CooldownOverlay)
	{
		CooldownOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CooldownText)
	{
		CooldownText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UP1SkillIconWidget::UpdateCooldownDisplay()
{
	const float Percent = RemainingCooldown / TotalCooldownTime;

	if (CooldownMID)
	{
		// CooldownPercent: 1.0=풀쿨(전체 어둠), 0.0=준비
		// 머터리얼에서 UV.Y < CooldownPercent 영역을 어둡게 처리하면 아래→위 해제
		CooldownMID->SetScalarParameterValue(TEXT("CooldownPercent"), Percent);
		if (CooldownOverlay)
		{
			CooldownOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
	if (CooldownText)
	{
		CooldownText->SetText(FText::FromString(FormatCooldown(RemainingCooldown)));
		CooldownText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

FString UP1SkillIconWidget::FormatCooldown(float Seconds) const
{
	// 10초 초과 → 정수, 이하 → 소수점 1자리 (LoL 스타일)
	if (Seconds > 10.f)
	{
		return FString::Printf(TEXT("%.0f"), Seconds);
	}
	return FString::Printf(TEXT("%.1f"), Seconds);
}
