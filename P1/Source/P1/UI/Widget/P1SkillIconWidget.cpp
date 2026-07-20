// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1SkillIconWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Widget.h"
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

	// 초기 상태는 항상 숨김 — 컨트롤러가 실제 투자 가능 여부를 알려주면(OnAbilityInvestStateChanged)
	// 그때 SetInvestButtonVisible이 켠다. 투자 불가 슬롯(LMB/Passive)은 애초에 그 브로드캐스트를
	// 받지 않으므로 계속 숨김 상태로 남는다. Collapsed가 아니라 Hidden을 쓰는 이유: InvestButton이
	// VerticalBox에서 아이콘 위 칸을 차지하는 구조라, Collapsed(레이아웃 공간까지 제거)를 쓰면 숨겨질
	// 때마다 아이콘이 위로 밀려 올라간다 — Hidden은 안 보이기만 하고 공간은 그대로 유지해 위치가 고정된다.
	UE_LOG(LogP1, Log, TEXT("[SkillIconWidget] NativeConstruct on %s | InvestButton 바인딩=%s"),
		*GetName(), InvestButton ? TEXT("O") : TEXT("X(WBP에서 이름이 정확히 'InvestButton'인지 확인)"));

	if (InvestButton)
	{
		InvestButton->SetVisibility(ESlateVisibility::Hidden);
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
		// 잠긴(미투자) 어빌리티는 쿨다운이 풀려도 계속 어둡게 유지 — 실전에서는 잠긴 어빌리티가
		// 애초에 쿨다운을 탈 수 없어 겹칠 일이 없지만 방어적으로 체크.
		SkillIconImage->SetColorAndOpacity(bIsLocked ? CooldownTint : ReadyTint);
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

void UP1SkillIconWidget::SetInvestButtonVisible(bool bVisible)
{
	if (InvestButton)
	{
		// SelfHitTestInvisible — 순수 표시기라 클릭을 받을 필요가 없고, 마우스 입력을 가로채면 안 된다.
		// Collapsed가 아니라 Hidden — 레이아웃 공간을 유지해 꺼질 때 스킬 아이콘이 위로 밀리지 않게 한다.
		InvestButton->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UP1SkillIconWidget::SetLocked(bool bLocked)
{
	bIsLocked = bLocked;

	// 쿨다운 중엔 쿨다운 쪽 틴트가 우선(어차피 같은 CooldownTint라 결과는 같지만, 진행 중인 쿨다운
	// 타이머 로직(NativeTick/UpdateCooldownDisplay)을 건드리지 않기 위해 여기서 색만 갈아끼운다).
	if (SkillIconImage && !bOnCooldown)
	{
		SkillIconImage->SetColorAndOpacity(bIsLocked ? CooldownTint : ReadyTint);
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
