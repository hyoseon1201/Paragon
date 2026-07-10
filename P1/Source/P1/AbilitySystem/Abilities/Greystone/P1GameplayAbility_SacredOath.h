// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "P1GameplayAbility_SacredOath.generated.h"

class UGameplayEffect;
class UAnimMontage;

// E — Sacred Oath.
// 데미지를 직접 입히지 않는 순수 버프 어빌리티. 자신에게 지속시간 있는 버프 GE를 적용하고
// 캐스팅 몽타주를 재생한 뒤, 몽타주가 끝나면 종료한다 (버프 자체는 즉시 적용 — 반응성 우선).
// 버프 GE가 하는 일:
//   - Buff.SacredOath.Active 태그 부여 → UP1GameplayAbility_MeleeAttack이 이 태그를 보고
//     "강화된 다음 기본공격"인지 판단, 발동 즉시 태그/GE를 제거해 1회만 소모되게 한다.
//   - AttackRange 어트리뷰트에 보너스 모디파이어 → 사거리 증가는 별도 코드 없이 어트리뷰트로 자연스럽게 처리.
// 검 발광 이펙트는 이 어빌리티가 전혀 관여하지 않는다 — 그레이스톤 BP에 붙은
// UP1BuffCosmeticEffectComponent가 Buff.SacredOath.Active 태그 자체를 구독해서 부여/소멸에 맞춰
// 알아서 켜고 끈다. 이 어빌리티는 그저 버프 GE를 적용할 뿐이고, 그게 무슨 코스메틱 반응을
// 일으키는지 전혀 몰라도 된다. 무기 궤적은 "매 스윙마다" 재생돼야 해서(태그 상태 자체가 아니라
// 스윙 이벤트에 반응) 여기가 아니라 MeleeAttack_SacredOath가 관리한다.
UCLASS()
class P1_API UP1GameplayAbility_SacredOath : public UP1GameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_SacredOath();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnCastMontageFinished();

	// 버프 GE (Duration=5초 등). Buff.SacredOath.Active 태그 부여 + AttackRange 보너스 모디파이어를 GE 자체에서 설정.
	UPROPERTY(EditDefaultsOnly, Category = "SacredOath")
	TSubclassOf<UGameplayEffect> BuffEffectClass;

	// 캐스팅 연출용 몽타주. 미설정 시 몽타주 없이 즉시 종료(기존 동작 유지).
	UPROPERTY(EditDefaultsOnly, Category = "SacredOath")
	TObjectPtr<UAnimMontage> CastMontage;
};
