// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/HUD/P1HUDWidget.h"
#include "UI/WidgetController/P1OverlayWidgetController.h"
#include "UI/Widget/HUD/P1SegmentedBarWidget.h"
#include "UI/Widget/HUD/P1HUDInventoryWidget.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameModes/P1GameState.h"
#include "P1.h"

void UP1HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshMatchTime();
}

void UP1HUDWidget::RefreshMatchTime()
{
	if (!MatchTimeText)
	{
		return;
	}

	const AP1GameState* P1GS = GetWorld() ? GetWorld()->GetGameState<AP1GameState>() : nullptr;
	const int32 ElapsedSeconds = P1GS ? FMath::FloorToInt(P1GS->GetElapsedMatchTime()) : 0;
	if (ElapsedSeconds == LastDisplayedMatchSeconds)
	{
		return;
	}

	LastDisplayedMatchSeconds = ElapsedSeconds;
	MatchTimeText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), ElapsedSeconds / 60, ElapsedSeconds % 60)));
}

void UP1HUDWidget::OnWidgetControllerSet()
{
	UP1OverlayWidgetController* Controller = CastChecked<UP1OverlayWidgetController>(WidgetController);

	Controller->OnHealthChanged.AddDynamic(this, &UP1HUDWidget::OnHealthChanged);
	Controller->OnMaxHealthChanged.AddDynamic(this, &UP1HUDWidget::OnMaxHealthChanged);
	Controller->OnHealthRegenChanged.AddDynamic(this, &UP1HUDWidget::OnHealthRegenChanged);
	Controller->OnManaChanged.AddDynamic(this, &UP1HUDWidget::OnManaChanged);
	Controller->OnMaxManaChanged.AddDynamic(this, &UP1HUDWidget::OnMaxManaChanged);
	Controller->OnManaRegenChanged.AddDynamic(this, &UP1HUDWidget::OnManaRegenChanged);

	Controller->OnAbilityIconAssigned.AddDynamic(this, &UP1HUDWidget::OnAbilityIconAssigned);
	Controller->OnCooldownStart.AddDynamic(this, &UP1HUDWidget::OnCooldownStart);
	Controller->OnCooldownClear.AddDynamic(this, &UP1HUDWidget::OnCooldownClear);
	Controller->OnAbilityInvestStateChanged.AddDynamic(this, &UP1HUDWidget::OnAbilityInvestStateChanged);
	Controller->OnAbilityLockedStateChanged.AddDynamic(this, &UP1HUDWidget::OnAbilityLockedStateChanged);

	Controller->OnLevelChanged.AddDynamic(this, &UP1HUDWidget::OnLevelChanged);
	Controller->OnKDAChanged.AddDynamic(this, &UP1HUDWidget::OnKDAChanged);
	Controller->OnExperienceChanged.AddDynamic(this, &UP1HUDWidget::OnExperienceChanged);
	Controller->OnGoldChanged.AddDynamic(this, &UP1HUDWidget::OnGoldChanged);

	// 우측하단 인벤토리 — 상점과 별도 컨트롤러 없이 같은 UP1OverlayWidgetController를 그대로 전파한다
	// (UP1HUDInventoryWidget은 PlayerState만 꺼내 쓰고, 인벤토리 변경은 AP1PlayerState::
	// OnInventoryChangedNative를 직접 구독 — Overlay 컨트롤러에 상점 델리게이트를 추가하지 않기 위함).
	if (InventoryWidget)
	{
		InventoryWidget->SetWidgetController(Controller);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[HUDWidget] InventoryWidget 바인딩 실패 — WBP_HUD에 'InventoryWidget'이라는 이름의 UP1HUDInventoryWidget 인스턴스가 없습니다. RefreshInventory()가 호출되지 않습니다"));
	}

	UE_LOG(LogP1, Log, TEXT("[HUDWidget][AbilityIcon] OnWidgetControllerSet — OnAbilityIconAssigned 등 3개 델리게이트 구독 완료. SkillIcon 바인딩: LMB=%s RMB=%s Q=%s E=%s R=%s Passive=%s"),
		SkillIcon_LMB ? TEXT("O") : TEXT("X(미배치)"),
		SkillIcon_RMB ? TEXT("O") : TEXT("X(미배치)"),
		SkillIcon_Q ? TEXT("O") : TEXT("X(미배치)"),
		SkillIcon_E ? TEXT("O") : TEXT("X(미배치)"),
		SkillIcon_R ? TEXT("O") : TEXT("X(미배치)"),
		SkillIcon_Passive ? TEXT("O") : TEXT("X(미배치)"));
}

void UP1HUDWidget::OnAbilityIconAssigned(FGameplayTag InputTag, UTexture2D* Icon)
{
	UP1SkillIconWidget* SkillIcon = GetSkillIconForInputTag(InputTag);
	UE_LOG(LogP1, Log, TEXT("[HUDWidget][AbilityIcon] OnAbilityIconAssigned 수신 — InputTag=%s | Icon=%s | 매칭된 SkillIcon 위젯=%s"),
		*InputTag.ToString(), Icon ? *Icon->GetName() : TEXT("NULL(미설정)"),
		SkillIcon ? TEXT("찾음") : TEXT("못찾음(WBP 미배치 또는 매핑 없음)"));

	if (SkillIcon)
	{
		SkillIcon->SetSkillIcon(Icon);
	}
}

void UP1HUDWidget::OnAbilityInvestStateChanged(FGameplayTag InputTag, bool bCanInvest)
{
	if (UP1SkillIconWidget* SkillIcon = GetSkillIconForInputTag(InputTag))
	{
		SkillIcon->SetInvestButtonVisible(bCanInvest);
	}
}

void UP1HUDWidget::OnAbilityLockedStateChanged(FGameplayTag InputTag, bool bLocked)
{
	if (UP1SkillIconWidget* SkillIcon = GetSkillIconForInputTag(InputTag))
	{
		SkillIcon->SetLocked(bLocked);
	}
}

void UP1HUDWidget::OnCooldownStart(FGameplayTag InputTag, float Duration)
{
	UP1SkillIconWidget* SkillIcon = GetSkillIconForInputTag(InputTag);
	UE_LOG(LogP1, Log, TEXT("[HUDWidget][Cooldown] OnCooldownStart 수신 — InputTag=%s Duration=%.2f | 매칭된 SkillIcon 위젯=%s"),
		*InputTag.ToString(), Duration, SkillIcon ? TEXT("찾음") : TEXT("못찾음"));

	if (SkillIcon)
	{
		SkillIcon->StartCooldown(Duration);
	}
}

void UP1HUDWidget::OnCooldownClear(FGameplayTag InputTag)
{
	UP1SkillIconWidget* SkillIcon = GetSkillIconForInputTag(InputTag);
	UE_LOG(LogP1, Log, TEXT("[HUDWidget][Cooldown] OnCooldownClear 수신 — InputTag=%s | 매칭된 SkillIcon 위젯=%s"),
		*InputTag.ToString(), SkillIcon ? TEXT("찾음") : TEXT("못찾음"));

	if (SkillIcon)
	{
		SkillIcon->ClearCooldown();
	}
}

UP1SkillIconWidget* UP1HUDWidget::GetSkillIconForInputTag(const FGameplayTag& InputTag) const
{
	if (InputTag == TAG_InputTag_Ability_BasicAttack) return SkillIcon_LMB;
	if (InputTag == TAG_InputTag_Ability_RMB)         return SkillIcon_RMB;
	if (InputTag == TAG_InputTag_Ability_Q)           return SkillIcon_Q;
	if (InputTag == TAG_InputTag_Ability_E)           return SkillIcon_E;
	if (InputTag == TAG_InputTag_Ability_R)           return SkillIcon_R;
	if (InputTag == TAG_InputTag_Ability_Passive)     return SkillIcon_Passive;
	return nullptr;
}

void UP1HUDWidget::OnHealthChanged(float NewValue)    { CachedHealth = NewValue;     RefreshHealthDisplay(); }
void UP1HUDWidget::OnMaxHealthChanged(float NewValue)  { CachedMaxHealth = NewValue;  RefreshHealthDisplay(); }
void UP1HUDWidget::OnHealthRegenChanged(float NewValue){ CachedHealthRegen = NewValue; RefreshHealthDisplay(); }
void UP1HUDWidget::OnManaChanged(float NewValue)       { CachedMana = NewValue;       RefreshManaDisplay(); }
void UP1HUDWidget::OnMaxManaChanged(float NewValue)    { CachedMaxMana = NewValue;    RefreshManaDisplay(); }
void UP1HUDWidget::OnManaRegenChanged(float NewValue)  { CachedManaRegen = NewValue;  RefreshManaDisplay(); }

void UP1HUDWidget::RefreshHealthDisplay()
{
	if (HealthBar)
	{
		HealthBar->SetValues(CachedHealth, CachedMaxHealth, CachedHealthRegen);
	}
}

void UP1HUDWidget::RefreshManaDisplay()
{
	if (ManaBar)
	{
		ManaBar->SetValues(CachedMana, CachedMaxMana, CachedManaRegen);
	}
}

void UP1HUDWidget::OnLevelChanged(int32 NewLevel)
{
	if (LevelText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv. %d"), NewLevel)));
	}
}

void UP1HUDWidget::OnKDAChanged(int32 Kills, int32 Deaths, int32 Assists)
{
	if (KDAText)
	{
		KDAText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d / %d"), Kills, Deaths, Assists)));
	}
}

void UP1HUDWidget::OnExperienceChanged(float CurrentXP, float XPRequiredForNextLevel)
{
	if (ExperienceBar)
	{
		// XPRequired=0(테이블 미설정 또는 만렙)이면 SegmentedBar가 0으로 나누지 않도록 1로 클램프.
		ExperienceBar->SetValues(CurrentXP, FMath::Max(XPRequiredForNextLevel, 1.0f));
	}
}

void UP1HUDWidget::OnGoldChanged(float NewValue)
{
	if (GoldText)
	{
		GoldText->SetText(FText::FromString(FString::Printf(TEXT("%d G"), FMath::RoundToInt(NewValue))));
	}
}
