// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1GameplayAbility_OnHitItemAbility.h"
#include "P1GameplayAbility_Item_TrueStrike.generated.h"

class UGameplayEffect;
class UNiagaraSystem;

// 증강(아이템) 고유효과 "진실의 일격" — 롤의 "주문검"류(Sheen/Spellblade)와 동일한 두 단계 메커니즘을
// 한 어빌리티 클래스 안에 담는다. 두 부분이 완전히 다른 트리거 방식을 쓰므로 별도 클래스로 쪼개지 않고,
// 각자 독립된 콜백/훅으로 명확히 분리해서 서로 섞이지 않게 한다:
//
//   1. "부여" — Q/E/RMB/R 중 하나를 사용하면(기본공격/패시브/다른 아이템 반응형 어빌리티는 제외) 자신에게
//      Buff.TrueStrike.Empowered(4초)를 걸고 자기 쿨다운(1.5초)을 커밋한다. RocketBoots의 "다른 스킬
//      사용 시 쿨감" 패시브와 정확히 같은 패턴 — OnGiveAbility에서 ASC->AbilityActivatedCallbacks를
//      구독해 처리하고, 이 경로는 ActivateAbility를 전혀 거치지 않는 순수 사이드이펙트다(RocketBoots가
//      쿨다운 감소를 ActivateAbility 밖에서 처리하는 것과 동일한 이유 — "다른 무언가가 발동했다"는 신호에
//      반응하는 것뿐이라 이 어빌리티 자신의 정식 활성화 생명주기가 필요 없음).
//   2. "소모" — 베이스 UP1GameplayAbility_OnHitItemAbility가 이미 제공하는 Event.Character.
//      BasicAttackHitDealt 트리거(정식 ActivateAbility 경로)를 그대로 사용 — OnBasicAttackHitDealt()만
//      오버라이드해서, Empowered 태그가 있으면 고정 피해(bIsTrueDamage=true, PhysicalPowerCoefficient로
//      스탯 계수화)를 추가로 입히고 태그를 제거한다.
//
// UP1DamageGameplayAbility를 상속(베이스가 이미 바뀜)해서 ApplyDamageToTarget()을 그대로 재사용한다 —
// 새 데미지 적용 경로를 따로 만들 필요가 없다.
UCLASS()
class P1_API UP1GameplayAbility_Item_TrueStrike : public UP1GameplayAbility_OnHitItemAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_Item_TrueStrike();

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	// "소모" — Empowered 태그가 있을 때만 고정 피해를 추가로 입히고 태그를 제거한다.
	virtual void OnBasicAttackHitDealt(AActor* Target, bool bWasCritical) override;

	// ASC->AbilityActivatedCallbacks 구독 콜백("부여") — Q/E/RMB/R 활성화만 필터링하고, 그 외(기본공격/
	// 패시브/InputTag 없는 아이템 반응형 어빌리티)는 무시한다.
	void OnAnyAbilityActivated(UGameplayAbility* ActivatedAbility);

	// Buff.TrueStrike.Empowered를 부여하는 4초짜리 마커 버프(스탯 모디파이어 없음 — 태그만).
	UPROPERTY(EditDefaultsOnly, Category = "TrueStrike")
	TSubclassOf<UGameplayEffect> EmpowerBuffEffectClass;

	// "부여" 순간(스킬 사용 감지, 강화 버프 시작) 1회 재생 — 미설정 시 재생 생략. "소모" 순간(강화된
	// 기본공격 적중, 고정 피해 발동)과는 별개 이펙트라 각자 독립적으로 설정/생략 가능하다.
	UPROPERTY(EditDefaultsOnly, Category = "TrueStrike|VFX")
	TObjectPtr<UNiagaraSystem> EmpowerEffect;

	UPROPERTY(EditDefaultsOnly, Category = "TrueStrike|VFX")
	FName EmpowerEffectSocketName = NAME_None;

	// "소모" 순간(강화된 기본공격이 적중해 고정 피해가 들어가는 순간) 1회 재생 — 미설정 시 재생 생략.
	UPROPERTY(EditDefaultsOnly, Category = "TrueStrike|VFX")
	TObjectPtr<UNiagaraSystem> ConsumeEffect;

	UPROPERTY(EditDefaultsOnly, Category = "TrueStrike|VFX")
	FName ConsumeEffectSocketName = NAME_None;
};
