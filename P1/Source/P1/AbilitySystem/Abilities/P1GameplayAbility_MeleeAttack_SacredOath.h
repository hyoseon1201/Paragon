// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1GameplayAbility_MeleeAttack.h"
#include "ScalableFloat.h"
#include "P1GameplayAbility_MeleeAttack_SacredOath.generated.h"

class UGameplayEffect;
class UParticleSystem;

// Sacred Oath(E) 시너지가 있는 영웅(그레이스톤)의 BasicAttack 어빌리티.
// 콤보/몽타주 로직은 UP1GameplayAbility_MeleeAttack을 그대로 상속받아 재사용하고,
// ApplyComboHitDamage()만 오버라이드해 데미지 계산 방식을 바꾼다:
//   - Cleave 무시, 범위 내 전원 100% 데미지
//   - 추가 데미지(Flat + PhysicalPower% + TargetMaxHealth%)
//   - 범위 전원에게 이동속도 감소 디버프
//   - 판정 후 Sacred Oath 버프 소모(제거) — 검 발광 원복은 AP1HeroCharacter가 태그 변경 이벤트로 처리
//
// 무기 궤적(TrailParticleTemplate)은 OnHitEventReceived를 오버라이드해 재생한다 — 검 발광과 달리
// "버프가 살아있는 동안 계속 붙어있는 지속 이펙트"가 아니라 "적중 여부와 무관하게 매 스윙마다
// 다시 재생"되어야 하기 때문이다 (허공에 헛스윙해도 버프는 소모되지 않으므로 트레일도 계속 나와야 함).
// OnHitEventReceived는 적 유무를 확인하기 전에(=스윙이 발생했다는 시점 자체에) 호출되므로 이 지점에서
// 버프 태그 유무만 확인해 트레일을 재생하고, 실제 데미지/버프 소모 판정은 그대로 Super에 위임한다.
//
// 콤보가 여러 스텝 이어지는 동안(홀드/재입력) 같은 어빌리티 인스턴스가 계속 재사용되므로,
// 매 히트마다 버프가 "아직도" 활성 상태인지 재확인한다 — 첫 스윙에서 버프를 소모한 뒤
// 이어지는 콤보 스텝은 평소 로직(Super)으로 자동 폴백해 "다음 1회만" 강화를 보장한다.
//
// 입력 라우팅: 이 시너지가 있는 영웅은 평소 UP1GameplayAbility_MeleeAttack 대신 이 클래스를
// 자신의 유일한 BasicAttack 어빌리티로 등록한다 — 같은 InputTag를 두고 두 어빌리티가
// Required/Blocked 태그로 경쟁하던 이전 방식은 액티베이션 타이밍(예측/복제, State.Attacking
// 공유 등)에 취약해 폐기했다. 버프 유무 판정은 활성화 시점이 아니라 실제 히트 판정 시점에
// 이 클래스 내부에서 하므로, 어떤 어빌리티가 나갈지를 둘러싼 경쟁 자체가 없다.
UCLASS()
class P1_API UP1GameplayAbility_MeleeAttack_SacredOath : public UP1GameplayAbility_MeleeAttack
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_MeleeAttack_SacredOath();

protected:
	virtual void ApplyComboHitDamage(const TArray<AActor*>& EnemyTargets, AActor* PrimaryTarget, float CleavePct) override;
	virtual void OnHitEventReceived(FGameplayEventData Payload) override;

private:
	void ConsumeSacredOathBuff() const;

	// 추가 데미지 채널 (클래스 기본 계수 위에 얹는 보너스로 ApplyDamageToTarget에 전달됨).
	// 어빌리티 레벨에 따라 값이 달라질 수 있어 FScalableFloat로 관리 (Curve 미지정 시 고정값).
	UPROPERTY(EditDefaultsOnly, Category = "SacredOath")
	FScalableFloat BonusFlatDamage = FScalableFloat(10.0f);

	UPROPERTY(EditDefaultsOnly, Category = "SacredOath")
	float BonusPhysicalPowerCoefficient = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "SacredOath")
	float BonusTargetMaxHealthPctCoefficient = 0.02f;

	// 슬로우 비율 (0~1). 지속시간은 GE 자체에 고정(1.25초)돼 있고 여기서는 크기만 전달.
	UPROPERTY(EditDefaultsOnly, Category = "SacredOath", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlowPercent = 0.22f;

	UPROPERTY(EditDefaultsOnly, Category = "SacredOath")
	TSubclassOf<UGameplayEffect> SlowDebuffEffectClass;

	// 매 스윙(적중 여부 무관)마다 재생할 무기 궤적 파티클 — 버프 태그가 살아있는 동안만 재생된다.
	UPROPERTY(EditDefaultsOnly, Category = "SacredOath|Trail")
	TObjectPtr<UParticleSystem> TrailParticleTemplate;

	UPROPERTY(EditDefaultsOnly, Category = "SacredOath|Trail")
	FName TrailSocketName;

	// true면 PIE에서 강화 판정 구체(금색)와 슬로우 적용 대상을 화면에 그린다 (Shipping 빌드에서는 자동 제거).
	UPROPERTY(EditDefaultsOnly, Category = "SacredOath|Debug")
	bool bShowDebugSacredOath = false;
};
