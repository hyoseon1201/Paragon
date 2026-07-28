// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "ScalableFloat.h"
#include "P1GameplayAbility_ContainmentFence.generated.h"

class UAnimMontage;
class AGameplayAbilityTargetActor;
class AP1ContainmentFence;

// E — Containment Fence (Dekker).
// AssaultTheGates와 동일한 지면 장판 조준(WaitTargetData+GroundDecal, 미확정 시 미커밋) 후, 확정 위치에
// AP1ContainmentFence를 배치한다. 데미지가 전혀 없는 순수 CC라 UP1DamageGameplayAbility가 아니라
// 베이스 UP1GameplayAbility를 상속 — 실제 이동 차단 판정은 이 어빌리티가 아니라 스폰된 액터가 전담한다
// (어빌리티는 "언제/어디에 배치할지"만 결정, GAS 로직은 확정 시점의 코스트/쿨다운 커밋이 전부).
UCLASS()
class P1_API UP1GameplayAbility_ContainmentFence : public UP1GameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_ContainmentFence();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data);

	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& Data);

private:
	void BeginTargeting();
	void SetTargetingState(bool bEnable);

	// --- 조준 ---
	UPROPERTY(EditDefaultsOnly, Category = "ContainmentFence|Targeting")
	TSubclassOf<AGameplayAbilityTargetActor> TargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "ContainmentFence|Targeting")
	float MaxRange = 1000.0f;

	// --- 배치 ---
	UPROPERTY(EditDefaultsOnly, Category = "ContainmentFence")
	TSubclassOf<AP1ContainmentFence> FenceActorClass;

	// 장판/이펙트 반경 — 원 설명에 레벨별 스케일 언급이 없어 고정값(장판 조준 인디케이터 크기와도 공유).
	UPROPERTY(EditDefaultsOnly, Category = "ContainmentFence")
	float FenceRadius = 350.0f;

	// 이동 차단 지속시간(초) — 레벨 1~5: 2.4/2.55/2.7/2.85/3.0.
	UPROPERTY(EditDefaultsOnly, Category = "ContainmentFence")
	FScalableFloat FenceDuration = FScalableFloat(2.4f);

	// 배치 순간 1회 재생(fire-and-forget) — 없으면 그냥 생략.
	UPROPERTY(EditDefaultsOnly, Category = "ContainmentFence")
	TObjectPtr<UAnimMontage> CastMontage;

	bool bTargetingActive = false;
};
