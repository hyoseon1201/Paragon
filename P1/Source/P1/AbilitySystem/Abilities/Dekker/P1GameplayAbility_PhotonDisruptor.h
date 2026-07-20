// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "ScalableFloat.h"
#include "P1GameplayAbility_PhotonDisruptor.generated.h"

class UAnimMontage;
class UGameplayEffect;
class UParticleSystem;

// Q — Photon Disruptor (Dekker).
// "드론이 하늘 위로 날아가 빔을 유지한 채 사거리 끝 지점까지 이동하며, 이동한 지표면에 짧은 간격으로
// 블래스트를 터뜨려 경로상의 적에게 마법 데미지+슬로우를 입힌다"는 연출이라, 실제 데미지 판정은
// 물리 충돌이 아니라 Make Way(Q, 그레이스톤)와 동일한 "어빌리티 소유 반복 타이머 + GetEnemiesInRadius()"
// 패턴으로 처리한다 — 회오리(캐릭터를 따라다니며 재중심)와의 차이는, 중심점이 캐스트 시점에 계산해둔
// 시작~끝 지점을 따라 시간에 비례해 이동한다는 것뿐이다.
// 하늘의 드론/빔 비주얼은 이미 완성된 캐스케이드 이펙트(P_Dekker_SkyBeam_Pod 등, 드론+빔이 하나로 합쳐진
// 자체 완결형 파티클)를 그대로 참조해 AP1CharacterBase::MulticastPlayMovingParticleEffect()로 재생한다 —
// Start/End/Duration이 결정적이라 각 클라이언트가 로컬 타이머만으로 독립 재생하고, 위치를 프레임마다
// 리플리케이트할 필요가 없다.
UCLASS()
class P1_API UP1GameplayAbility_PhotonDisruptor : public UP1DamageGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_PhotonDisruptor();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	void OnBlastTick();

	// 캐스팅 연출용 몽타주 — 스윕 지속시간과는 별개로 재생만 하고 끝(어빌리티 종료를 기다리지 않음).
	// 미설정 시 재생 생략.
	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor")
	TObjectPtr<UAnimMontage> CastMontage;

	// 시작 지점(캐릭터 위치)에서 조준 방향으로 이 거리만큼 떨어진 x,y지점까지 스윕한다.
	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor|Sweep")
	float SweepRange = 1200.0f;

	// 드론이 날아다니는 높이(지면 기준). 순수 비주얼 값 — 판정에는 영향 없음(판정은 항상 지표면 기준).
	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor|Sweep")
	float SkyHeight = 700.0f;

	// 드론이 시작~끝 지점을 이동하는 데 걸리는 전체 시간 — 이 값을 줄이면 드론(SkyDronePodEffect)이
	// 더 빠르게 이동한다. 블래스트 판정 간격(BlastTickInterval)과 독립적이라, 드론 속도만 따로 조절해도
	// 데미지 판정 빈도에는 영향이 없다 — 매 틱마다 "실제 경과시간/SweepDuration" 비율로 지표면 좌표를
	// 다시 계산하므로 블래스트는 항상 드론이 실제로 있는 위치를 그대로 따라간다.
	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor|Sweep")
	float SweepDuration = 1.6f;

	// 블래스트(데미지 판정+VFX) 샘플링 간격(초) — SweepDuration과 무관하게 얼마나 자주 판정할지만 결정한다.
	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor|Sweep")
	float BlastTickInterval = 0.2f;

	// 매 틱 판정 반경 / 높이 필터(GetEnemiesInRadius).
	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor|Sweep")
	float BlastRadius = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor|Sweep")
	float BlastHalfHeight = 200.0f;

	// 하늘을 나는 드론/빔 비주얼(예: P_Dekker_SkyBeam_Pod) — 드론+빔이 하나로 합쳐진 자체 완결형
	// 캐스케이드 이펙트를 그대로 참조한다. 미설정 시 비주얼 없이 판정만 진행.
	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor|VFX")
	TObjectPtr<UParticleSystem> SkyDronePodEffect;

	// 매 틱 지표면에 터지는 블래스트 이펙트.
	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor|VFX")
	TObjectPtr<UParticleSystem> GroundBlastEffect;

	// GroundBlastEffect 크기 배율 — 에셋 자체를 수정하지 않고 여기서 줄이거나 키울 수 있다. 기본값 (1,1,1).
	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor|VFX")
	FVector GroundBlastEffectScale = FVector(1.0f);

	// 슬로우 디버프 GE — Duration=1초 고정, Data.DebuffMagnitude(SetByCaller)로 이 어빌리티 레벨별 크기를 주입.
	// Debuff.MovementSlow 태그 공유(Sacred Oath/Stone Forged Soul과 동일 태그, 새로 안 만듦).
	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor")
	TSubclassOf<UGameplayEffect> SlowDebuffEffectClass;

	// 레벨 1~5: 30/32.5/35/37.5/40% — 커브테이블로 GetValueAtLevel(GetAbilityLevel())이 평가.
	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor")
	FScalableFloat SlowPercent = FScalableFloat(0.30f);

	UPROPERTY(EditDefaultsOnly, Category = "PhotonDisruptor|Debug")
	bool bShowDebug = false;

private:
	// 발사 방향(수평) — 논타겟 스킬샷 컨벤션과 동일하게 카메라 조준 방향 기준.
	FVector GetAimDirectionXY() const;

	// 지정 XY 지점에서 아래로 트레이스해 실제 지표면 Z를 구한다(AP1TargetActor_GroundDecal과 동일 패턴).
	// 트레이스가 아무것도 못 맞히면 SourceCharacter 높이를 그대로 사용.
	FVector SnapToGround(const FVector& XYSource, const AActor* IgnoreActor) const;

	FTimerHandle BlastTimerHandle;
	int32 CurrentTick = 0;
	FVector GroundStart = FVector::ZeroVector;
	FVector GroundEnd = FVector::ZeroVector;

	// OnBlastTick이 "지금까지 얼마나 지났는지"를 계산하는 기준 시각.
	double SweepStartTime = 0.0;

	// EndAbility가 여러 경로(정상 종료/중단)로 여러 번 불려도 쿨다운이 중복 적용되지 않도록 하는 가드.
	bool bCooldownApplied = false;
};
