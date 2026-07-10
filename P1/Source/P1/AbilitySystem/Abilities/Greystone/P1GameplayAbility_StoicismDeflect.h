// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "ScalableFloat.h"
#include "P1GameplayAbility_StoicismDeflect.generated.h"

// Stoicism(패시브) — 디플렉트 절반. 다음 기본공격을 무효화하고, 사용 후 일정 시간 쿨다운.
//
// 실제 "기본공격인지 판별 + 데미지 무효화" 판정 자체는 이 어빌리티가 아니라
// UP1AttributeSet::PostGameplayEffectExecute에서 처리한다(무적 태그 체크와 같은 자리) — 데미지를
// 실제로 취소할 수 있는 유일한 지점이 거기이기 때문. 이 어빌리티의 역할은 딱 두 가지:
//   1. OnGiveAbility에서 자신의 Asset Tag(Ability.StoicismDeflect)를 루즈 태그로 ASC에 심어서,
//      AttributeSet이 "이 캐릭터가 애초에 디플렉트를 갖고 있는지"를 태그만으로(어빌리티 클래스
//      참조 없이) 판별할 수 있게 한다.
//   2. AttributeSet이 디플렉트 발동을 감지하면 보내는 GameplayEvent(Event.StoicismDeflect.Consumed)를
//      받아 자기 쿨다운 GE만 커밋한다 — "사용 가능한지" 자체는 네이티브 쿨다운 태그
//      (Cooldown.Ability.StoicismDeflect)의 부재로 이미 판별되므로, 이 어빌리티가 별도의
//      "준비 완료" 상태를 관리할 필요가 없다(새로 부여된 어빌리티는 쿨다운 태그가 없는 채로
//      시작하므로 "처음엔 사용 가능"이 자연스럽게 성립).
UCLASS()
class P1_API UP1GameplayAbility_StoicismDeflect : public UP1GameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_StoicismDeflect();

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 디버깅용 — 쿨다운 태그 카운트 변경(0→1=쿨다운 시작, 1→0=사용 가능해짐)을 로그로 남긴다.
	void OnCooldownTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 15/14.4/.../4.8초 등 레벨별 쿨다운 — 스킬 랭크(GetAbilityLevel())가 아니라 "캐릭터 레벨"(1~18)
	// 기준으로 평가해야 하므로, CooldownGameplayEffectClass 자체의 Scalable Float 커브(그건
	// GetAbilityLevel()로 평가됨, AssaultTheGates 이전의 다른 어빌리티들과 동일한 방식이라 여기선 안 맞음)
	// 대신 이 값을 GetP1CharacterLevel()로 평가해 Data.CooldownDuration SetByCaller로 직접 주입한다
	// (AssaultTheGates::BaseCooldown과 같은 패턴). Curve 미지정 시 고정값으로 동작(IsStatic()).
	UPROPERTY(EditDefaultsOnly, Category = "Stoicism|Cooldown")
	FScalableFloat BaseCooldown = FScalableFloat(15.0f);
};
