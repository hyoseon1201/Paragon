// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "P1GameplayAbility_RangedAttack.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UGameplayEffect;
class AP1Projectile;
struct FGameplayEventData;
struct FHitResult;

// 원거리 기본공격 — MeleeAttack과 대칭 구조(같은 AttackTimer UI 쿨다운 패턴, 같은 InputTag/AssetTag를
// 공유하는 "영웅별로 이 클래스 아니면 MeleeAttack 둘 중 하나만 등록" 방식)이지만 콤보는 없다: 발사
// 프레임마다 AP1Projectile을 하나 스폰하고, 버튼을 누르고 있으면(bInputHeld) 몽타주가 끝날 때마다
// 같은 몽타주를 계속 반복 재생한다(연사).
UCLASS()
class P1_API UP1GameplayAbility_RangedAttack : public UP1DamageGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_RangedAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	// UP1GameplayAbility 참고 — 기본공격은 진짜 GAS 쿨다운이 없어(연사를 막으면 안 되므로)
	// CooldownGameplayEffectClass 대신 이 태그를 스킬 아이콘 UI가 구독하도록 오버라이드한다.
	// MeleeAttack과 동일한 이유/패턴.
	virtual FGameplayTag GetUICooldownTag() const override;

	// AnimNotify 발사 프레임마다 호출 — 서버에서만 실제로 투사체를 스폰한다(클라 예측 인스턴스도 동일
	// 노티파이를 받으므로 IsNetAuthority 체크 필수, MeleeAttack::OnHitEventReceived와 동일한 이유).
	UFUNCTION()
	void OnFireEventReceived(FGameplayEventData Payload);

	// 스폰된 투사체가 뭔가에 맞았을 때 — 투사체 자체가 서버에서만 스폰되므로 이 델리게이트도 서버에서만 발화한다.
	UFUNCTION()
	void OnProjectileHit(AActor* HitActor, const FHitResult& HitResult);

	// 매 발사 시작 시(PlayAttackMontage) Data.CooldownDuration SetByCaller로 "다음 공격까지 남은
	// 시간"을 실어 적용 — Cooldown.Ability.BasicAttack 태그만 부여하는 순수 UI용 GE.
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> AttackTimerEffectClass;

	// 콤보가 없으므로 배열이 아니라 몽타주 하나 — 매 발사마다 처음부터 반복 재생된다.
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	TSubclassOf<AP1Projectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	float ProjectileSpeed = 4000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	float ProjectileRadius = 15.0f;

	// 캐릭터 메시에서 투사체를 스폰할 소켓(예: 총구). 비어있거나 존재하지 않으면 캐릭터 위치로 폴백.
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	FName MuzzleSocketName;

private:
	void PlayAttackMontage();

	UFUNCTION()
	void OnMontageBlendingOut();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	void EndAttack();
	float GetComputedMontagePlayRate() const;

	// 발사 방향 — 논타겟 스킬샷 컨벤션과 동일하게 캐릭터 정면이 아니라 카메라(조준) 방향 기준.
	FVector GetAimDirection() const;

	// 이전 발사분의 히트 이벤트 태스크 — 다음 발사 시작 시 명시적으로 종료해 중복 수신을 방지한다.
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveFireEventTask;

	// 현재 버튼이 눌린 상태(MeleeAttack::bInputHeld와 동일한 이유로 Spec->InputPressed 대신 이 플래그 사용).
	bool bInputHeld = false;

	// OnBlendOut에서 다음 발사를 이미 시작했을 때 OnCompleted의 EndAttack 호출을 막는 플래그.
	bool bContinuingFire = false;
};
