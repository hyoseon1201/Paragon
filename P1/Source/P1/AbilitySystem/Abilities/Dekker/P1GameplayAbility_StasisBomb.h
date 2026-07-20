// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "ScalableFloat.h"
#include "P1GameplayAbility_StasisBomb.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UGameplayEffect;
class UParticleSystem;
class AP1Projectile;
struct FHitResult;

// RMB — Stasis Bomb (Dekker).
// "우클릭을 누르고 있으면 조준 포즈 유지, 떼는 순간 발사"하는 홀드 조준 스킬. 튕기는 폭탄을 던져
// 첫 충돌이 적이면 즉시, 지형이면 1회 튕긴 뒤 다음 충돌에서 폭발 — 범위 데미지+스턴(거리 비례로 최대치까지 증가).
//
// AssaultTheGates(RMB, 그레이스톤)와 조준 메커니즘이 다르다 — 그쪽은 지면 장판(WaitTargetData)+LMB로
// 확정하지만, 이건 장판이 없고 "조준을 시작한 스킬 자신(RMB)을 떼는 순간"이 곧 확정이다. 그래서:
//   - 확정은 GAS의 Confirm 델리게이트가 아니라 InputReleased() 오버라이드로 직접 처리.
//   - 취소만 기존 State.TargetingAbility 인프라(F 키)를 그대로 재사용 — ASC->GenericLocalCancelCallbacks에
//     직접 구독해서 받는다(WaitTargetData 없이 이 델리게이트를 쓰려면 직접 구독해야 함).
// 코스트/쿨다운은 조준 단계에서는 커밋하지 않고, 실제 발사(릴리즈) 순간에만 커밋 — 조준만 하다 취소하면
// 완전 무소모(AssaultTheGates의 "확정 전 미커밋"과 동일한 이유).
//
// 폭탄 자체는 AP1Projectile을 그대로 사용(MaxBounces=1로 1회 튕김 활성화) — 실제 폭발 판정은 데미지
// 어빌리티 공통 헬퍼(ApplyDamageToTarget/GetEnemiesInRadius)로 범위 처리, 투사체는 "언제 터질지"만 알려준다.
UCLASS()
class P1_API UP1GameplayAbility_StasisBomb : public UP1DamageGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_StasisBomb();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 우클릭을 떼는 순간 — 조준 중이었다면 이 호출 자체가 발사 확정이다.
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// F(취소) 입력 — ASC->GenericLocalCancelCallbacks에 직접 구독해서 받는다.
	UFUNCTION()
	void OnAimCancelled();

	// 폭탄이 최종적으로 터지는 시점(첫 적 충돌, 또는 1회 튕긴 뒤의 다음 충돌) — 범위 데미지+스턴 판정.
	UFUNCTION()
	void OnBombExplode(AActor* HitActor, const FHitResult& HitResult);

	// 조준 유지 포즈 — 에셋 자체를 Loop로 설정(StunMontage와 동일한 패턴). 릴리즈/취소 시 명시적으로 정지.
	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb")
	TObjectPtr<UAnimMontage> AimMontage;

	// 발사 순간 1회 재생(fire-and-forget, 완료를 기다리지 않음).
	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Bomb")
	TSubclassOf<AP1Projectile> BombProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Bomb")
	float ProjectileSpeed = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Bomb")
	float ProjectileRadius = 30.0f;

	// 첫 충돌이 지형일 때 몇 번까지 튕길지 — 설명상 "1회 튕긴 뒤 다음 충돌에서 폭발"이라 기본값 1.
	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Bomb")
	int32 BombMaxBounces = 1;

	// 폭탄을 스폰할 소켓. 비어있거나 존재하지 않으면 캐릭터 위치로 폴백.
	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Bomb")
	FName MuzzleSocketName;

	// 폭발 시 범위 판정 반경/높이 필터(GetEnemiesInRadius).
	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Explosion")
	float ExplosionRadius = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Explosion")
	float ExplosionHalfHeight = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Explosion")
	TObjectPtr<UParticleSystem> ExplosionEffect;

	// 스턴 디버프 GE — Duration=SetByCaller(Data.StunDuration), State.Stunned 태그 부여(전 캐릭터 공용).
	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Stun")
	TSubclassOf<UGameplayEffect> StunDebuffEffectClass;

	// 근거리(이동거리=0)일 때 스턴 지속시간 — 레벨 무관 고정값(원본 스킬 설명: "0.8s" 고정).
	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Stun")
	float MinStunDuration = 0.8f;

	// MaxStunDistance 이상 날아갔을 때의 스턴 지속시간 — 레벨 1~5: 1.4/1.55/1.7/1.85/2.0초.
	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Stun")
	FScalableFloat MaxStunDuration = FScalableFloat(1.4f);

	// 이 거리 이상 날아가면 스턴이 MaxStunDuration으로 최대치에 도달(그 이상은 더 안 늘어남).
	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Stun")
	float MaxStunDistance = 3500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "StasisBomb|Debug")
	bool bShowDebug = false;

private:
	// 발사 방향 — 논타겟 스킬샷 컨벤션과 동일하게 캐릭터 정면이 아니라 카메라(조준) 방향 기준.
	FVector GetAimDirection() const;

	// 조준 태그 부여/해제 — AssaultTheGates::SetTargetingState()와 동일한 패턴(루즈 태그, 중복 호출 방지 가드).
	void SetTargetingState(bool bEnable);

	// AimMontage 정지 — EndTask만으로는 루프 몽타주가 확실히 안 멎을 수 있어 Montage_Stop도 함께 호출한다.
	void StopAimMontage();

	bool bTargetingActive = false;

	// InputReleased가 조준 중에만 발사를 트리거하도록(예: 발사 후 몽타주 재생 중 다시 릴리즈 이벤트가 와도
	// 중복 발사되지 않도록) 가드.
	bool bHasFired = false;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> AimMontageTask;

	TWeakObjectPtr<AP1Projectile> ActiveBomb;
};
