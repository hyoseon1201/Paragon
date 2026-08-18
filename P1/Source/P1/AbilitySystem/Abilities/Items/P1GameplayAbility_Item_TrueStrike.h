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

	// Buff.TrueStrike.Empowered 태그 0<->양수 전이 콜백 — 손 발광 이펙트의 시작/중지를 여기 한 곳에서만
	// 처리한다("소모" 경로의 RemoveActiveEffectsWithGrantedTags든, 버프 GE가 4초 후 자연 만료되든 둘 다
	// 결국 이 태그가 사라지는 것으로 귀결되므로, 원인과 무관하게 정확히 한 번만 반응할 수 있음 — Sacred
	// Oath 검 발광에서 이미 검증된 패턴). MulticastSetAttachedNiagaraEffect/Stop을 쓰는 이유: 이 이펙트는
	// 보통 Looping 에셋이라(지속 버프 연출), 1회성 MulticastPlayNiagaraEffect로 재생하면 "완료" 상태에
	// 영영 도달하지 못해 버프가 끝나도 영구히 남아있는 버그가 생긴다.
	void OnEmpoweredTagChanged(const FGameplayTag Tag, int32 NewCount);

	// Buff.TrueStrike.Empowered를 부여하는 4초짜리 마커 버프(스탯 모디파이어 없음 — 태그만).
	UPROPERTY(EditDefaultsOnly, Category = "TrueStrike")
	TSubclassOf<UGameplayEffect> EmpowerBuffEffectClass;

	// Buff.TrueStrike.Empowered 태그가 붙어있는 동안(부여~소모 또는 4초 자연만료까지) 지속 재생 —
	// 미설정 시 재생 생략. "소모" 순간(강화된 기본공격 적중, 고정 피해 발동)과는 별개 이펙트라 각자
	// 독립적으로 설정/생략 가능하다. **양손에 동시 재생**(주문검류 연출 — 양손이 빛나는 느낌) —
	// OnEmpoweredTagChanged가 소켓별로 MulticastSetAttachedNiagaraEffect/Stop을 호출한다(1회성
	// MulticastPlayNiagaraEffect가 아님 — Looping 에셋을 1회성으로 재생하면 버프가 끝나도 이펙트가
	// 영구히 안 사라지는 버그가 생기므로 반드시 지속형 함수를 사용). 소켓 이름은 스켈레톤마다 다르므로
	// BP에서 지정 — 둘 중 하나만 채워도 그쪽만 재생되고, 둘 다 비우면(기본값 NAME_None) 재생 생략.
	UPROPERTY(EditDefaultsOnly, Category = "TrueStrike|VFX")
	TObjectPtr<UNiagaraSystem> EmpowerEffect;

	UPROPERTY(EditDefaultsOnly, Category = "TrueStrike|VFX")
	FName EmpowerEffectSocketNameLeftHand = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "TrueStrike|VFX")
	FName EmpowerEffectSocketNameRightHand = NAME_None;

	// "소모" 순간(강화된 기본공격이 적중해 고정 피해가 들어가는 순간) 1회 재생 — 미설정 시 재생 생략.
	UPROPERTY(EditDefaultsOnly, Category = "TrueStrike|VFX")
	TObjectPtr<UNiagaraSystem> ConsumeEffect;

	UPROPERTY(EditDefaultsOnly, Category = "TrueStrike|VFX")
	FName ConsumeEffectSocketName = NAME_None;
};
