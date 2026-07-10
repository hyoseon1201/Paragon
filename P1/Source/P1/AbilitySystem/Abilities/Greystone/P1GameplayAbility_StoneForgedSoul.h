// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "ScalableFloat.h"
#include "P1GameplayAbility_StoneForgedSoul.generated.h"

class UGameplayEffect;
class UAnimMontage;
class UAbilityTask_WaitGameplayEvent;
struct FGameplayEventData;

// R(궁극기) — Stone Forged Soul.
// 별도 조준 없이 서있는 자리에서 발동. 캐스팅+상승+공중대기+하강+착지가 전부 담긴 몽타주 하나를
// 재생하며, 이 몽타주의 루트모션이 실제 상승/하강 이동을 담당한다(목표 지점이 없는 고정 왕복 이동이라
// RMB의 ApplyRootMotionJumpForce 같은 코드 기반 궤적 생성이 필요 없음 — 원본 애니메이션에 실제
// 수직 루트모션이 있는지만 확인하면 됨). 공중에 뜬 동안 중력이 루트모션과 싸우지 않도록
// MovementMode를 일시적으로 MOVE_Flying으로 전환했다가, 착지 시점(Crash 이벤트)에 되돌린다.
//
// 발동 즉시(상승 시작 시점):
//   - 최대체력 비례 즉시 회복, 잃은 체력이 많을수록 최대 MaxHealAmplification배까지 증폭 (C++에서
//     계산 후 SetByCaller로 최종값만 전달 — 단순 계수 곱셈으로 표현 안 되는 공식이라 ExecCalc 대신 이 방식).
//   - State.Invulnerable 태그를 부여하는 무적 GE 적용 — 이 태그는 전 어빌리티 공용이며
//     UP1AttributeSet::PostGameplayEffectExecute가 Damage 처리 시 체크해 무시한다.
//   - 근접도 비례 슬로우(최대 MaxSlowPercent) — Make Way처럼 짧은 간격으로 반복 재스캔해 거리 기반
//     크기로 디버프를 계속 갱신 적용(캐스터는 공중에 고정이지만 적은 움직일 수 있으므로).
// 착지 시점(AnimNotify → Event.Montage.StoneForgedSoul.Crash):
//   - MovementMode를 MOVE_Walking으로 복귀, 슬로우 재스캔 타이머 정지
//   - 그 자리 기준 범위 내 전원에게 물리 데미지(FlatDamage/PhysicalPowerCoefficient는 베이스의
//     UP1DamageGameplayAbility 프로퍼티 재사용)
UCLASS()
class P1_API UP1GameplayAbility_StoneForgedSoul : public UP1DamageGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_StoneForgedSoul();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnMontageFinished();

	// 착지 프레임 이벤트 — 비행 모드 해제 + 범위 데미지 판정.
	UFUNCTION()
	void OnCrashEventReceived(FGameplayEventData Payload);

	void OnSlowTick();

	// 캐스팅+상승+공중대기+하강+착지가 전부 담긴 단일 몽타주. 루트모션이 실제 이동을 담당.
	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul")
	TObjectPtr<UAnimMontage> CastMontage;

	// --- 회복 ---
	// 최대체력 비례 회복 계수 (레벨별 12/14/16% 등). 잃은 체력 비율에 따라 아래 증폭이 곱해진다.
	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Heal")
	FScalableFloat HealPercent = FScalableFloat(0.12f);

	// 잃은 체력 비율 100%(빈사 상태) 기준 최대 증폭 배율. 만피 상태는 1.0배(증폭 없음).
	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Heal")
	float MaxHealAmplification = 2.5f;

	// 즉시 회복 GE — Health += SetByCaller(Data.Heal.Flat), Duration=Instant.
	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Heal")
	TSubclassOf<UGameplayEffect> HealEffectClass;

	// --- 무적(스테이시스) ---
	// State.Invulnerable 태그를 부여하는 Duration GE — 지속시간은 이 GE 자체에 고정(스테이시스 길이와 일치).
	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Stasis")
	TSubclassOf<UGameplayEffect> InvulnerabilityEffectClass;

	// --- 근접도 비례 슬로우 ---
	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Slow")
	TSubclassOf<UGameplayEffect> SlowDebuffEffectClass;

	// 재스캔 간격(초). 짧을수록 슬로우가 적의 실시간 위치를 더 촘촘히 반영.
	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Slow")
	float SlowTickPeriod = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Slow")
	float SlowRadius = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Slow")
	float SlowHalfHeight = 200.0f;

	// 캐스터 바로 아래(거리 0)에서 적용되는 최대 슬로우 비율. 반경 끝에서는 0%로 선형 감쇠.
	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Slow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxSlowPercent = 0.70f;

	// --- 착지 데미지 판정 범위 (데미지 계수 자체는 베이스 UP1DamageGameplayAbility의 FlatDamage 등 재사용) ---
	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Crash")
	float CrashRadius = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Crash")
	float CrashHalfHeight = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "StoneForgedSoul|Debug")
	bool bShowDebug = false;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> CrashEventTask;

	FTimerHandle SlowTickTimerHandle;
};
