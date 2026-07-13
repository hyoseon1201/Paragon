// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/P1WidgetController.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "P1OverlayWidgetController.generated.h"

class UTexture2D;
class UGameplayAbility;

// 스킬 아이콘 배정(1회성) — 어빌리티의 InputTag와 AbilityIcon을 전달.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityIconAssignedSignature, FGameplayTag, InputTag, UTexture2D*, Icon);

// 쿨다운 시작 — InputTag가 어느 슬롯인지, Duration은 남은 시간(초).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCooldownStartSignature, FGameplayTag, InputTag, float, Duration);

// 쿨다운 해제.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCooldownClearSignature, FGameplayTag, InputTag);

// 캐릭터 레벨 변경.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelChangedSignature, int32, NewLevel);

// Kills/Deaths/Assists — 항상 세 값을 함께 표시하므로 하나로 묶어 브로드캐스트.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnKDAChangedSignature, int32, Kills, int32, Deaths, int32, Assists);

// 경험치 — CurrentXP/XPRequired 쌍(바 채우기용). CharacterLevel이 바뀌면 XPRequired도 바뀌므로
// Experience 값 자체가 안 바뀌어도 재브로드캐스트가 필요할 수 있다(레벨업 순간).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExperienceChangedSignature, float, CurrentXP, float, XPRequiredForNextLevel);

// 인게임 HUD(HP·MP·리젠·스킬슬롯 등)용 WidgetController.
// ASC 속성 변경을 구독하고 위젯에 브로드캐스트한다.
UCLASS(BlueprintType)
class P1_API UP1OverlayWidgetController : public UP1WidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	// ---- Health ----
	UPROPERTY(BlueprintAssignable, Category = "Events|Health")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Health")
	FOnAttributeChangedSignature OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Health")
	FOnAttributeChangedSignature OnHealthRegenChanged;

	// ---- Mana ----
	UPROPERTY(BlueprintAssignable, Category = "Events|Mana")
	FOnAttributeChangedSignature OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Mana")
	FOnAttributeChangedSignature OnMaxManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Mana")
	FOnAttributeChangedSignature OnManaRegenChanged;

	// ---- Skill Icon / Cooldown ----
	UPROPERTY(BlueprintAssignable, Category = "Events|Ability")
	FOnAbilityIconAssignedSignature OnAbilityIconAssigned;

	UPROPERTY(BlueprintAssignable, Category = "Events|Ability")
	FOnCooldownStartSignature OnCooldownStart;

	UPROPERTY(BlueprintAssignable, Category = "Events|Ability")
	FOnCooldownClearSignature OnCooldownClear;

	// ---- Progression (레벨/KDA/경험치/골드) ----
	UPROPERTY(BlueprintAssignable, Category = "Events|Progression")
	FOnLevelChangedSignature OnLevelChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Progression")
	FOnKDAChangedSignature OnKDAChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Progression")
	FOnExperienceChangedSignature OnExperienceChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Progression")
	FOnAttributeChangedSignature OnGoldChanged;

private:
	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void OnHealthRegenAttributeChanged(const FOnAttributeChangeData& Data);
	void OnManaAttributeChanged(const FOnAttributeChangeData& Data);
	void OnMaxManaAttributeChanged(const FOnAttributeChangeData& Data);
	void OnManaRegenAttributeChanged(const FOnAttributeChangeData& Data);
	void OnGoldAttributeChanged(const FOnAttributeChangeData& Data);
	void OnExperienceAttributeChanged(const FOnAttributeChangeData& Data);

	// AP1PlayerState의 네이티브 델리게이트 핸들러 (레벨/KDA — GAS 어트리뷰트가 아닌 plain int)
	void OnCharacterLevelChanged(int32 NewLevel);
	void OnKDAChanged_Internal(int32 Kills, int32 Deaths, int32 Assists);

	// 현재 Experience/XPRequired를 함께 브로드캐스트하는 공용 헬퍼 — Experience 변경, 레벨 변경(=XPRequired 변경)
	// 양쪽에서 호출된다.
	void BroadcastExperience();

	// ASC->AreAbilitiesGiven()이 true가 된 시점(즉시 또는 AbilitiesGivenDelegate 경유)에 호출된다.
	// GetActivatableAbilities()를 순회해 아이콘을 1회 배정하고, 어빌리티마다 자기 쿨다운 태그의
	// 변경 이벤트를 구독한다.
	void BroadcastAbilityInfo();
	void OnAbilityCooldownTagChanged(const FGameplayTag CooldownTag, int32 NewCount,
		FGameplayTag InputTag, TWeakObjectPtr<UGameplayAbility> AbilityWeak, FGameplayAbilitySpecHandle Handle);
};
