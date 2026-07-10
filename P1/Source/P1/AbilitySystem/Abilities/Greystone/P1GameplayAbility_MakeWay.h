// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "P1GameplayAbility_MakeWay.generated.h"

class UGameplayEffect;
class UParticleSystem;

// Q — Make Way.
// 캐릭터 주변을 따라다니는 화염 회오리. 1초 간격으로 4틱(1,2,3,4초) 동안 캐릭터 "현재" 위치 기준으로
// 반경 내 적을 매번 새로 스캔해 데미지 + 방어력 감소 디버프(최대 4중첩)를 적용한다.
// 고정 타겟에게 반복 적용되는 표준 Periodic GameplayEffect로는 "캐릭터를 따라다니며 매번 재판정"을
// 표현할 수 없어, 어빌리티가 직접 반복 타이머를 소유하고 매 틱 GetEnemiesInRadius()로 재스캔한다.
//
// 코스트/쿨다운 타이밍: "쿨다운은 지속시간이 끝나야 시작"이므로 표준 CommitAbility()로 한번에 처리하지
// 않는다. 캐스트 시점엔 CommitAbilityCost()만, 4번째 틱(지속시간 종료) 직후 EndAbility에서 쿨다운을 적용한다.
UCLASS()
class P1_API UP1GameplayAbility_MakeWay : public UP1DamageGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_MakeWay();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	void OnWhirlwindTick();

	// 회오리 반경 / 높이 필터.
	UPROPERTY(EditDefaultsOnly, Category = "MakeWay")
	float WhirlwindRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "MakeWay")
	float HalfHeight = 150.0f;

	// 틱 간격(초)과 총 틱 수. 기본 1초 간격 × 4틱 = 4초 지속과 "최대 4중첩"이 정확히 대응된다.
	UPROPERTY(EditDefaultsOnly, Category = "MakeWay")
	float TickPeriod = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "MakeWay")
	int32 TotalTicks = 4;

	// 방어력 감소 디버프 GE (Duration 있음, Stacking 설정은 GE 에셋에서). Data.DebuffMagnitude로 크기 전달.
	UPROPERTY(EditDefaultsOnly, Category = "MakeWay")
	TSubclassOf<UGameplayEffect> ArmorShredDebuffEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "MakeWay", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ArmorShredMagnitude = 0.04f;

	// 캐릭터를 따라다니는 회오리 지속 파티클 — 캐스트 시작~지속시간 종료(정상 종료/조기 취소 공통)까지 유지.
	// 소켓을 비워두면 특정 본이 아니라 루트 컴포넌트(액터 전체)에 붙어 캐릭터를 그대로 따라간다.
	UPROPERTY(EditDefaultsOnly, Category = "MakeWay|VFX")
	TObjectPtr<UParticleSystem> WhirlwindParticleTemplate;

	UPROPERTY(EditDefaultsOnly, Category = "MakeWay|VFX")
	FName WhirlwindSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "MakeWay|Debug")
	bool bShowDebug = false;

	FTimerHandle TickTimerHandle;
	int32 CurrentTick = 0;

	// EndAbility가 여러 경로(정상 종료/중단)로 여러 번 불려도 쿨다운이 중복 적용되지 않도록 하는 가드.
	bool bCooldownApplied = false;
};
