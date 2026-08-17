// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "P1GameplayAbility_OnHitItemAbility.generated.h"

// 온-히트 아이템 고유효과(크로노 스트라이크, 매너스, 진실의 일격 등)의 공통 베이스 — Event.Character.
// BasicAttackHitDealt 하나를 공통으로 구독해 "기본 공격이 적중했다"는 신호만 받고, 실제로 무엇을 할지는
// 서브클래스가 결정한다. **`UP1DamageGameplayAbility`를 상속한다(2026-08-17, 진실의 일격 추가 시 변경)** —
// 온-히트 반응이 버프/쿨감뿐 아니라 "추가 데미지"인 경우도 흔해서(주문검류), `ApplyDamageToTarget()`/
// 계수 프로퍼티를 공용으로 물려받게 했다. 크로노 스트라이크/매너스는 이 기능을 안 쓰므로 영향 없음.
//
// 설계 배경: 실제 판정(기본공격인지, 데미지가 적용됐는지)은 UP1AttributeSet::PostGameplayEffectExecute
// 에서만 볼 수 있어 그쪽에 남아야 하지만, "그래서 무엇을 할지"(쿨감, 버프 적용 등)까지 AttributeSet에
// 몰아넣으면 아이템이 늘어날수록 AttributeSet이 계속 커지는 문제가 생긴다(스토이시즘 디플렉트가 이미
// 겪었던 것과 같은 문제 — AttributeSet은 어빌리티 클래스를 참조하면 안 된다는 원칙과도 부딪힘). 그래서
// AttributeSet은 "적중했다"는 이벤트 하나만 가해자에게 쏘고, 실제 반응은 이 이벤트를 구독하는 아이템
// 전용 어빌리티(각각 이 클래스를 상속) 쪽에서 전담한다 — 새 온-히트 아이템이 추가돼도 AttributeSet은
// 전혀 안 바뀌고, 이 어빌리티 클래스 하나만 새로 만들면 된다.
//
// 이 어빌리티는 입력으로 발동하지 않고(InputTag 없음), 아이템 구매 시 ASC->GiveAbility()로 부여되고
// 판매 시 ClearAbility()로 회수된다(AP1PlayerState::ServerBuyItem/ServerSellItem 참고) — 즉 "이 어빌리티를
// 갖고 있다"는 사실 자체가 "이 아이템을 보유 중이다"라는 신호를 겸한다(별도의 "존재 확인용 태그"가 필요 없음).
UCLASS(Abstract)
class P1_API UP1GameplayAbility_OnHitItemAbility : public UP1DamageGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_OnHitItemAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 서브클래스가 구현 — Target=피격자 액터, bWasCritical=이번 적중이 크리티컬이었는지.
	// AP1PlayerState로 캐스트 가능하면 영웅, 실패하면 정글 몬스터(HandleKillRewards 등과 동일한 판별 관례).
	virtual void OnBasicAttackHitDealt(AActor* Target, bool bWasCritical) PURE_VIRTUAL(UP1GameplayAbility_OnHitItemAbility::OnBasicAttackHitDealt, );
};
