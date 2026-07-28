// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "ScalableFloat.h"
#include "P1GameplayAbility_RocketBoots.generated.h"

class UAnimMontage;

// Dekker Passive — Rocket Boots. 공중(낙하 중)에 점프 입력을 다시 누르면 위로 추진한다.
// 데미지/버프 없는 순수 이동 스킬이라 UP1DamageGameplayAbility가 아니라 베이스 UP1GameplayAbility를
// 상속. 입력은 전용 InputTag가 아니라 InputTag.Ability.Passive를 재사용(UI 슬롯 공유 목적, 위 태그
// 정의 주석 참고) — AP1PlayerController::HandleJumpStarted가 캐릭터가 공중(IsFalling())일 때
// Character->Jump() 대신 ASC->AbilityInputTagPressed(InputTag.Ability.Passive)를 호출해 이 어빌리티를
// 발동시킨다.
//
// 쿨다운은 스킬 랭크가 아니라 "캐릭터 레벨"(1~18) 기준으로 평가해야 하므로(StoicismDeflect와 동일한
// 이유), CooldownGameplayEffectClass의 Scalable Float 커브 대신 BaseCooldown을 GetP1CharacterLevel()로
// 평가해 Data.CooldownDuration SetByCaller로 직접 주입한다. "다른 어빌리티 사용 시 쿨다운 3초 감소"는
// ASC->AbilityActivatedCallbacks를 OnGiveAbility에서 구독해 처리 — 이 어빌리티는 입력으로만 짧게
// 활성화됐다 끝나므로(ActivateAbility 수명과 무관하게) 캐릭터 생존 내내 유지되는 구독이 필요하다.
UCLASS()
class P1_API UP1GameplayAbility_RocketBoots : public UP1GameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_RocketBoots();

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// ASC->AbilityActivatedCallbacks 구독 콜백 — 자기 자신을 제외한 다른 어빌리티가 발동될 때마다 호출.
	void OnAnyAbilityActivated(UGameplayAbility* ActivatedAbility);

	// 위로 추진하는 순간 속도(cm/s) — LaunchCharacter의 Z축 값. 원문에 구체적 수치가 없어 임의 기본값,
	// 실제 값은 BP에서 튜닝.
	UPROPERTY(EditDefaultsOnly, Category = "RocketBoots")
	float LaunchVelocityZ = 1200.0f;

	// 레벨 1~18: 22/21.3/20.6/.../10.1초 — 캐릭터 레벨(GetP1CharacterLevel()) 기준으로 평가한다
	// (StoicismDeflect::BaseCooldown과 동일한 패턴). Curve 미지정 시 고정값으로 동작.
	UPROPERTY(EditDefaultsOnly, Category = "RocketBoots")
	FScalableFloat BaseCooldown = FScalableFloat(22.0f);

	// 다른 어빌리티를 사용할 때마다 즉시 감소시킬 쿨다운(초).
	UPROPERTY(EditDefaultsOnly, Category = "RocketBoots")
	float CooldownReductionOnAbilityUse = 3.0f;

	// 발동 순간 1회 재생되는 연출(fire-and-forget). 미설정 시 재생 생략.
	UPROPERTY(EditDefaultsOnly, Category = "RocketBoots")
	TObjectPtr<UAnimMontage> LaunchMontage;

	UPROPERTY(EditDefaultsOnly, Category = "RocketBoots|Debug")
	bool bShowDebug = false;

private:
	// 쿨다운 GE를 InDuration으로 적용(AssaultTheGates::ApplyCooldownWithDuration과 동일 패턴).
	void ApplyCooldownWithDuration(float InDuration);

	// 남은 쿨다운을 InSeconds만큼 앞당겨 감소시킨다 — 이미 쿨다운 중이 아니면 아무 일도 하지 않는다.
	void ReduceCooldownBySeconds(float InSeconds);
};
