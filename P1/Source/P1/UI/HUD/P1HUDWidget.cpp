// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/HUD/P1HUDWidget.h"
#include "UI/WidgetController/P1OverlayWidgetController.h"
#include "UI/Widget/P1SegmentedBarWidget.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Engine/Texture2D.h"
#include "P1.h"

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
