// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "P1GameplayAbility_MeleeAttack.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
struct FGameplayEventData;

// 근접 기본공격 — 콤보 지원.
// BasicAttackHit 이벤트 수신 시점이 데미지 판정 + 콤보 윈도우 오픈을 겸한다.
// 윈도우 중 입력(press 또는 hold)이 감지되면 몽타주 종료 후 다음 콤보로 진행한다.
UCLASS()
class P1_API UP1GameplayAbility_MeleeAttack : public UP1DamageGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_MeleeAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 어빌리티 활성 중 입력 press 감지 — 콤보 윈도우가 열려 있으면 다음 콤보를 예약한다.
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

private:
	void PlayCurrentComboMontage();

	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageBlendingOut();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	void EndAttack();
	float GetComputedMontagePlayRate(const UAnimMontage* Montage) const;

	// 콤보 몽타주 배열. [0]=1타, [1]=2타, ... 순환 재생.
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Combo")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

	// AttackRange 어트리뷰트를 반지름으로 하는 구 오버랩 후 캐릭터 정면 180° 필터링 → 반원 판정.
	// 반원 높이 범위 (캐릭터 중심 기준 ± HalfHeight cm). 지나치게 낮은 적이나 공중 타겟 제외에 사용.
	UPROPERTY(EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "50.0"))
	float AttackHalfHeight = 100.0f;

	// true면 PIE에서 타격 판정 구체와 주목표를 화면에 그린다 (Shipping 빌드에서는 자동 제거).
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Debug")
	bool bShowDebugAttack = false;

	// 현재 재생 중인 히트 이벤트 태스크 — 콤보 전환 시 명시적으로 종료해 중복 수신을 방지한다.
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveHitEventTask;

	int32 CurrentComboIndex = 0;

	// BasicAttackHit 이후 ~ 몽타주 종료 전 구간. 이 구간에서 입력을 받으면 다음 콤보를 예약.
	bool bComboWindowOpen = false;

	// 콤보 윈도우 중 입력이 감지됨 — 몽타주 종료 시 다음 콤보 재생 트리거.
	bool bNextComboQueued = false;

	// OnBlendOut에서 다음 콤보를 이미 시작했을 때 OnCompleted의 EndAttack 호출을 막는 플래그.
	bool bContinuingCombo = false;
};
