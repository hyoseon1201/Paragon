// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "P1GameplayAbility_AssaultTheGates.generated.h"

class UAnimMontage;
class UGameplayEffect;
class AGameplayAbilityTargetActor;
class UAbilityTask_WaitGameplayEvent;
struct FGameplayEventData;

// RMB — Assault The Gates.
// 2단계 스킬: (1) 지면 조준 상태(장판 표시, 코스트/쿨다운 미소모, LMB=확정 / RMB=취소)
//            (2) 확정 시 코스트 소모 → 목표 위치로 도약(MotionWarp) → 착지 시 범위 물리피해
//                → 영웅/보스 적중 시 이동속도 버프 + 쿨다운 35% 감소
UCLASS()
class P1_API UP1GameplayAbility_AssaultTheGates : public UP1DamageGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_AssaultTheGates();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 조준 확정 — 타겟 데이터(착지 위치) 수신.
	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data);

	// 조준 취소 — 무소모 종료.
	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& Data);

	// 착지 몽타주 이벤트 — 서버 범위 피해 판정.
	UFUNCTION()
	void OnLandEventReceived(FGameplayEventData Payload);

	// 재생 위치가 어느 슬롯 세그먼트(애니메이션) 구간에 있는지 주기적으로 로그 (디버깅용).
	void LogCurrentMontageSegment();

	// 몽타주 종료 경로별로 어떤 델리게이트가 실제 발화하는지 구분해서 로그를 남긴다 (디버깅용).
	UFUNCTION()
	void OnLeapMontageCompleted();
	UFUNCTION()
	void OnLeapMontageBlendOut();
	UFUNCTION()
	void OnLeapMontageInterrupted();
	UFUNCTION()
	void OnLeapMontageCancelled();

private:
	void BeginTargeting();
	void ExecuteLeap(const FVector& TargetLocation);
	void LogMontageEvent(const TCHAR* EventName) const;
	void HandleMontageEnd();
	void PerformLandDamage(const FVector& Center);
	void ApplyCooldownWithDuration(float Duration);
	void ReduceCooldown(float Percent);
	void SetTargetingState(bool bEnable);

	// --- 조준 ---
	UPROPERTY(EditDefaultsOnly, Category = "Assault|Targeting")
	TSubclassOf<AGameplayAbilityTargetActor> TargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Assault|Targeting")
	float MaxRange = 1200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Assault|Targeting")
	float AOERadius = 300.0f;

	// --- 도약 ---
	UPROPERTY(EditDefaultsOnly, Category = "Assault|Leap")
	TObjectPtr<UAnimMontage> LeapMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Assault|Leap")
	FName WarpTargetName = TEXT("AssaultTarget");

	// JumpForce 포물선 아크 정점 높이(cm). 애니메이션 루트모션과 무관하게 코드로 아크 생성.
	UPROPERTY(EditDefaultsOnly, Category = "Assault|Leap")
	float LeapHeight = 400.0f;

	// 도약 소요 시간(초). 몽타주 착지 노티파이 타이밍과 맞추면 자연스럽다.
	UPROPERTY(EditDefaultsOnly, Category = "Assault|Leap")
	float LeapDuration = 0.8f;

	// AOE 높이 필터 (착지 지점 기준 ± cm).
	UPROPERTY(EditDefaultsOnly, Category = "Assault")
	float AOEHalfHeight = 200.0f;

	// --- 쿨다운 ---
	// 기본 쿨다운(초). 쿨다운 GE는 SetByCaller(Data.CooldownDuration)로 이 값을 사용.
	UPROPERTY(EditDefaultsOnly, Category = "Assault|Cooldown")
	float BaseCooldown = 12.0f;

	// 영웅/보스 적중 시 쿨다운 감소율 (0~1).
	UPROPERTY(EditDefaultsOnly, Category = "Assault|Cooldown", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CooldownReductionOnHeroHit = 0.35f;

	// --- 보상 ---
	// 영웅/보스 적중 시 자신에게 적용할 이동속도 버프 GE (+25%, 2.5초 등).
	UPROPERTY(EditDefaultsOnly, Category = "Assault|Reward")
	TSubclassOf<UGameplayEffect> MoveSpeedBuffEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Assault|Debug")
	bool bShowDebug = false;

	// 현재 조준 상태 태그가 부여돼 있는지.
	bool bTargetingActive = false;

	// 확정된 착지 위치 — AOE 판정 중심으로 사용 (클라/서버 동일).
	FVector ConfirmedLocation = FVector::ZeroVector;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> LandEventTask;

	// 세그먼트 추적 디버그용 타이머 및 마지막으로 로그한 세그먼트 이름(중복 로그 방지).
	FTimerHandle MontageSegmentTimerHandle;
	FName LastLoggedSegmentName;
};
