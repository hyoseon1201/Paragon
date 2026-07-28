// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1FloatingStatusWidget.h"
#include "UI/WidgetController/P1FloatingStatusWidgetController.h"
#include "UI/Widget/P1SegmentedBarWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/ProgressBar.h"
#include "P1.h"

void UP1FloatingStatusWidget::OnWidgetControllerSet()
{
	UP1FloatingStatusWidgetController* Controller = CastChecked<UP1FloatingStatusWidgetController>(WidgetController);

	Controller->OnHealthChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnHealthChanged);
	Controller->OnMaxHealthChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnMaxHealthChanged);
	Controller->OnManaChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnManaChanged);
	Controller->OnMaxManaChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnMaxManaChanged);
	Controller->OnStunStateChanged.AddDynamic(this, &UP1FloatingStatusWidget::OnStunStateChanged);
}

void UP1FloatingStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsStunned)
	{
		return;
	}

	// 바 시각용 카운트다운만 — 0 밑으론 안 내려가게 클램프하되 여기서 자동으로 bIsStunned=false로
	// 숨기지는 않는다. 숨김은 오직 태그가 빠질 때(컨트롤러의 OnStunStateChanged(false))만 처리한다.
	// 시뮬레이티드 프록시에는 스턴 GE가 없어 정확한 지속시간을 몰라 폴백값으로 카운트다운하는데, 여기서
	// 자동 숨김을 하면 실제 스턴이 안 끝났는데 프록시 화면에서만 바가 먼저 사라지기 때문.
	RemainingStunDuration = FMath::Max(0.f, RemainingStunDuration - InDeltaTime);
	UpdateStunDisplay();
}

void UP1FloatingStatusWidget::SetCharacterName(const FText& NewName)
{
	if (NameText)
	{
		NameText->SetText(NewName);
	}
}

void UP1FloatingStatusWidget::SetLevel(int32 NewLevel)
{
	if (LevelText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv. %d"), NewLevel)));
	}
}

void UP1FloatingStatusWidget::OnHealthChanged(float NewValue)   { CachedHealth = NewValue;    RefreshBars(); }
void UP1FloatingStatusWidget::OnMaxHealthChanged(float NewValue) { CachedMaxHealth = NewValue; RefreshBars(); }
void UP1FloatingStatusWidget::OnManaChanged(float NewValue)      { CachedMana = NewValue;      RefreshBars(); }
void UP1FloatingStatusWidget::OnMaxManaChanged(float NewValue)   { CachedMaxMana = NewValue;   RefreshBars(); }

void UP1FloatingStatusWidget::RefreshBars()
{
	if (HPBar)
	{
		HPBar->SetValues(CachedHealth, CachedMaxHealth);
	}
	if (MPBar)
	{
		MPBar->SetValues(CachedMana, CachedMaxMana);
	}
}

void UP1FloatingStatusWidget::OnStunStateChanged(bool bNewIsStunned, float Duration)
{
	if (bNewIsStunned)
	{
		// Duration은 컨트롤러가 넘겨주는 "남은시간"이다. 바는 이 남은시간을 100%로 잡아 0까지 줄어든다.
		// 스턴 시작 직후 처음엔 폴백(1초)으로 왔다가 StunEndServerTime이 도착하면 정확한 남은시간으로
		// 다시 브로드캐스트되는데, 그 보정값을 반영해야 하므로 매 true 브로드캐스트마다 리셋한다(보정은
		// 스턴 시작 1~2프레임 안에서 거의 같은 값으로 일어나 눈에 안 띈다). 이후엔 브로드캐스트가 없고
		// NativeTick이 스스로 카운트다운하며, 숨김은 태그가 빠질 때(false 브로드캐스트)만 일어난다.
		bIsStunned = true;
		TotalStunDuration = FMath::Max(0.01f, Duration);
		RemainingStunDuration = TotalStunDuration;
	}
	else
	{
		bIsStunned = false;
		RemainingStunDuration = 0.f;
	}
	UpdateStunDisplay();
}

void UP1FloatingStatusWidget::UpdateStunDisplay()
{
	// 이 위젯은 UP1FloatingWidgetComponent(WidgetComponent) 위에 렌더타겟 텍스처로 그려진다. 그 컴포넌트는
	// Slate 트리가 리페인트를 요구할 때만 텍스처를 재갱신하는데, 자식의 콘텐츠 변경(SetText/SetPercent —
	// HP/MP 바가 쓰는 방식)은 그 무효화를 정확히 일으켜 갱신되지만, 자식의 Visibility/RenderOpacity 변경은
	// 이 셋업에서 렌더타겟 재갱신으로 이어지지 않아 값만 바뀌고 화면엔 옛 프레임이 남았다(Visibility로도
	// Opacity로도 동일하게 실패, Reflector엔 새 값이 보이는데 텍스처는 안 바뀜). 그래서 표시/숨김을 전부
	// 콘텐츠 변경으로만 처리한다 — 스턴 아닐 땐 라벨=빈문자열, 바=0%.
	if (StunLabelText)
	{
		StunLabelText->SetText(bIsStunned ? FText::FromString(TEXT("Stun")) : FText::GetEmpty());
	}
	if (StunProgressBar)
	{
		// 1.0(방금 시작)→0.0(곧 해제)로 줄어든다 — FillType=LeftToRight면 바가 왼쪽부터 비워지는 느낌.
		// 스턴이 아니면 0%로 비워 숨김을 표현(WBP에서 Background 브러시는 투명하게 둘 것).
		const float Percent = (bIsStunned && TotalStunDuration > 0.f) ? (RemainingStunDuration / TotalStunDuration) : 0.f;
		StunProgressBar->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));
	}
}
