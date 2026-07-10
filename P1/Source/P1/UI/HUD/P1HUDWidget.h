// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "UI/Widget/P1SkillIconWidget.h"
#include "GameplayTagContainer.h"
#include "P1HUDWidget.generated.h"

class UP1SegmentedBarWidget;
class UP1SkillIconWidget;
class UP1OverlayWidgetController;
class UTexture2D;

// 화면 하단 중앙 메인 HUD. WBP_HUD(BP)가 이 클래스를 상속하며,
// BP에서 BindWidget 이름과 일치하는 위젯을 배치해야 한다.
UCLASS()
class P1_API UP1HUDWidget : public UP1UserWidget
{
	GENERATED_BODY()

protected:
	virtual void OnWidgetControllerSet() override;

	// ---- Health ---- (BP에서 동일 이름으로 배치 필수)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1SegmentedBarWidget> HealthBar;

	// ---- Mana ----
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1SegmentedBarWidget> ManaBar;

	// ---- 스킬 슬롯 아이콘 (Optional) ----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UP1SkillIconWidget> SkillIcon_Passive;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UP1SkillIconWidget> SkillIcon_Q;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UP1SkillIconWidget> SkillIcon_E;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UP1SkillIconWidget> SkillIcon_R;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UP1SkillIconWidget> SkillIcon_LMB;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UP1SkillIconWidget> SkillIcon_RMB;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UP1SkillIconWidget> SkillIcon_Flash;

private:
	// 컨트롤러 델리게이트 핸들러 — float 직접 수신
	UFUNCTION()
	void OnHealthChanged(float NewValue);
	UFUNCTION()
	void OnMaxHealthChanged(float NewValue);
	UFUNCTION()
	void OnHealthRegenChanged(float NewValue);
	UFUNCTION()
	void OnManaChanged(float NewValue);
	UFUNCTION()
	void OnMaxManaChanged(float NewValue);
	UFUNCTION()
	void OnManaRegenChanged(float NewValue);

	// ---- 스킬 아이콘/쿨다운 ----
	UFUNCTION()
	void OnAbilityIconAssigned(FGameplayTag InputTag, UTexture2D* Icon);
	UFUNCTION()
	void OnCooldownStart(FGameplayTag InputTag, float Duration);
	UFUNCTION()
	void OnCooldownClear(FGameplayTag InputTag);

	// InputTag.Ability.BasicAttack/RMB/Q/E/R → SkillIcon_LMB/RMB/Q/E/R 매핑.
	// Passive/Flash는 InputTag가 없어(패시브는 입력 없이 발동, Flash는 미구현) 이 경로로 연결되지 않는다.
	UP1SkillIconWidget* GetSkillIconForInputTag(const FGameplayTag& InputTag) const;

	void RefreshHealthDisplay();
	void RefreshManaDisplay();

	float CachedHealth = 0.f;
	float CachedMaxHealth = 1.f;
	float CachedHealthRegen = 0.f;
	float CachedMana = 0.f;
	float CachedMaxMana = 1.f;
	float CachedManaRegen = 0.f;
};
