// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "P1GameplayAbility_MeleeAttack.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UGameplayEffect;
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

	// 어빌리티 활성 중 입력 press — bInputHeld=true, 윈도우 열려 있으면 콤보 예약.
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	// 입력 release — bInputHeld=false.
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	// 콤보 히트 판정에서 실제 데미지를 적용하는 지점 — 서브클래스가 이 훅만 오버라이드해 콤보/몽타주
	// 로직 전체를 복제하지 않고도 데미지 계산 방식을 바꿀 수 있다 (예: 버프로 강화된 공격 변종).
	// 기본 구현: Cleave 기반 배율로 주목표 100% / 그 외 대상 Cleave%만큼 데미지.
	virtual void ApplyComboHitDamage(const TArray<AActor*>& EnemyTargets, AActor* PrimaryTarget, float CleavePct);

	// AnimNotify 히트 프레임마다 호출 — 적중 여부와 무관하게 "스윙이 발생했다"는 시점 자체를 나타낸다.
	// 서브클래스가 이 훅을 오버라이드하면 적을 맞혔는지와 무관하게(허공을 휘둘러도) 매 스윙마다
	// 반응하는 코스메틱 이펙트(예: 버프 지속 중 트레일 재생)를 넣을 수 있다 — Super 호출은 필수.
	UFUNCTION()
	virtual void OnHitEventReceived(FGameplayEventData Payload);

	// UP1GameplayAbility 참고 — 기본공격은 진짜 GAS 쿨다운이 없어(콤보를 막으면 안 되므로)
	// CooldownGameplayEffectClass 대신 이 태그를 스킬 아이콘 UI가 구독하도록 오버라이드한다.
	virtual FGameplayTag GetUICooldownTag() const override;

	// 매 스윙 시작 시(PlayCurrentComboMontage) Data.CooldownDuration SetByCaller로 "다음 공격까지 남은
	// 시간"(BasicAttackTime/AttackSpeed 기반 실제 스윙 소요시간)을 실어 적용 — Cooldown.Ability.BasicAttack
	// 태그만 부여하는 순수 UI용 GE. CooldownGameplayEffectClass가 아니므로 발동 차단과 무관하다.
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> AttackTimerEffectClass;

private:
	void PlayCurrentComboMontage();

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

	// 현재 버튼이 눌린 상태. Spec->InputPressed(서버에서 stale 가능) 대신 이 플래그를 사용한다.
	// 클라/서버 모두 InputPressed()/InputReleased() RPC 콜백으로 정확하게 추적된다.
	bool bInputHeld = false;

	// OnBlendOut에서 다음 콤보를 이미 시작했을 때 OnCompleted의 EndAttack 호출을 막는 플래그.
	bool bContinuingCombo = false;
};
