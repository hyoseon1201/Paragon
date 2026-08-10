// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "UI/Widget/HUD/P1SkillIconWidget.h"
#include "GameplayTagContainer.h"
#include "P1HUDWidget.generated.h"

class UP1SegmentedBarWidget;
class UP1SkillIconWidget;
class UP1OverlayWidgetController;
class UP1HUDInventoryWidget;
class UTexture2D;
class UTextBlock;

// 화면 하단 중앙 메인 HUD. WBP_HUD(BP)가 이 클래스를 상속하며,
// BP에서 BindWidget 이름과 일치하는 위젯을 배치해야 한다.
UCLASS()
class P1_API UP1HUDWidget : public UP1UserWidget
{
	GENERATED_BODY()

protected:
	virtual void OnWidgetControllerSet() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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

	// ---- 좌하단 플레이어 정보(레벨/KDA/경험치/골드) (Optional) ----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KDAText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UP1SegmentedBarWidget> ExperienceBar;

	// ---- 매치 경과 시간(MM:SS) (Optional) ---- ASC/위젯 컨트롤러를 거치지 않고 AP1GameState를 매
	// 프레임 직접 폴링(FloatingStatusWidget의 스턴 카운트다운과 동일 패턴) — 값이 계속 흐르는 시계라
	// 델리게이트로 매초 브로드캐스트하는 것보다 폴링이 더 단순하다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MatchTimeText;

	// ---- 우측하단 보유 아이템 표시(읽기전용, Optional) ---- 상점의 UP1ShopInventoryWidget과 같은
	// 데이터를 보여주지만 판매 인터랙션은 없다(WBP_HUDInventory, UP1HUDInventoryWidget 참고).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UP1HUDInventoryWidget> InventoryWidget;

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
	UFUNCTION()
	void OnAbilityInvestStateChanged(FGameplayTag InputTag, bool bCanInvest);
	UFUNCTION()
	void OnAbilityLockedStateChanged(FGameplayTag InputTag, bool bLocked);

	// ---- 좌하단 플레이어 정보 ----
	UFUNCTION()
	void OnLevelChanged(int32 NewLevel);
	UFUNCTION()
	void OnKDAChanged(int32 Kills, int32 Deaths, int32 Assists);
	UFUNCTION()
	void OnExperienceChanged(float CurrentXP, float XPRequiredForNextLevel);
	UFUNCTION()
	void OnGoldChanged(float NewValue);

	// InputTag.Ability.BasicAttack/RMB/Q/E/R → SkillIcon_LMB/RMB/Q/E/R 매핑.
	// Passive/Flash는 InputTag가 없어(패시브는 입력 없이 발동, Flash는 미구현) 이 경로로 연결되지 않는다.
	UP1SkillIconWidget* GetSkillIconForInputTag(const FGameplayTag& InputTag) const;

	void RefreshHealthDisplay();
	void RefreshManaDisplay();
	void RefreshMatchTime();

	// 초 단위가 바뀔 때만 SetText를 호출하기 위한 캐시(-1=아직 한 번도 안 찍음, 매 프레임 문자열
	// 포맷팅을 반복하지 않기 위함).
	int32 LastDisplayedMatchSeconds = -1;

	float CachedHealth = 0.f;
	float CachedMaxHealth = 1.f;
	float CachedHealthRegen = 0.f;
	float CachedMana = 0.f;
	float CachedMaxMana = 1.f;
	float CachedManaRegen = 0.f;
};
