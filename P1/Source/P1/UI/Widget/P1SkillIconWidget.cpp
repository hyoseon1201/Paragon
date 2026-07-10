// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1SkillIconWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"
#include "P1.h"

void UP1SkillIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SkillIconImage)
	{
		SkillIconImage->SetColorAndOpacity(ReadyTint);
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
	UE_LOG(LogP1, Log, TEXT("[SkillIconWidget] SetSkillIcon(%s=%s) on %s | SkillIconImage 바인딩=%s"),
		IconTexture ? TEXT("텍스처") : TEXT("NULL"),
		IconTexture ? *IconTexture->GetName() : TEXT("N/A"),
		*GetName(),
		SkillIconImage ? TEXT("O") : TEXT("X(WBP에서 SkillIconImage 이름의 Image 위젯 미배치)"));

	if (!IconTexture)
	{
		return;
	}

	if (SkillIconImage)
	{
		SkillIconImage->SetBrushFromTexture(IconTexture);
	}
}

void UP1SkillIconWidget::StartCooldown(float TotalCooldown)
{
	UE_LOG(LogP1, Log, TEXT("[SkillIconWidget] StartCooldown(%.2f) on %s | CooldownOverlay 바인딩=%s"),
		TotalCooldown, *GetName(), CooldownOverlay ? TEXT("O") : TEXT("X(WBP에서 CooldownOverlay 이름의 ProgressBar 미배치)"));

	TotalCooldownTime = FMath::Max(0.01f, TotalCooldown);
	RemainingCooldown = TotalCooldownTime;
	bOnCooldown = true;

	if (SkillIconImage)
	{
		SkillIconImage->SetColorAndOpacity(CooldownTint);
	}

	UpdateCooldownDisplay();
}

void UP1SkillIconWidget::ClearCooldown()
{
	bOnCooldown = false;
	RemainingCooldown = 0.f;

	if (SkillIconImage)
	{
		SkillIconImage->SetColorAndOpacity(ReadyTint);
	}
	if (CooldownOverlay)
	{
		CooldownOverlay->SetPercent(0.f);
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

	if (CooldownOverlay)
	{
		// Percent: 1.0=풀쿨(전체 채움), 0.0=준비. FillType=BottomToTop이면 위쪽부터 순서대로 사라진다.
		CooldownOverlay->SetPercent(Percent);
		CooldownOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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
