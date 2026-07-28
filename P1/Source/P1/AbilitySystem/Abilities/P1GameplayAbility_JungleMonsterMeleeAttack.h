// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "P1GameplayAbility_JungleMonsterMeleeAttack.generated.h"

class UAnimMontage;

// 정글 몬스터 근접 공격 — Event.Monster.MeleeAttack GameplayEvent로만 트리거된다(입력 없음,
// AP1JungleMonsterCharacter::RequestMeleeAttack()을 BTTask_P1MeleeAttack이 매 틱 호출해서 이벤트를
// 보냄). 몽타주/콤보 없이 즉시 판정 — AttackRadius 반경 내 적 전원에게 데미지(반경을 좁게 잡으면
// 사실상 단일 대상).
//
// 재사용 대기시간은 새 태그를 만들지 않고 베이스 UGameplayAbility::CooldownGameplayEffectClass(에디터
// 설정)로 표준 GAS 커밋 방식 그대로 처리한다 — 쿨다운 중엔 이벤트가 들어와도 TryActivateAbility 내부의
// CanActivateAbility가 자동으로 막아준다(StoicismDeflect와 동일한 이벤트 트리거+쿨다운 조합 패턴).
UCLASS()
class P1_API UP1GameplayAbility_JungleMonsterMeleeAttack : public UP1DamageGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_JungleMonsterMeleeAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 구 오버랩 판정 반경(cm). 근접 몬스터답게 짧게 — GetEnemiesInRadius가 팀/높이 필터까지 처리해준다.
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackRadius = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "50.0"))
	float AttackHalfHeight = 100.0f;

	// 판정과 동시에(선딜 없이) 재생하는 코스메틱 공격 몽타주 — 풀바디 슬롯 하나면 충분하다(BT가 이동을
	// 멈추고 공격하는 구조라 히어로처럼 하체 로코모션과 블렌드할 필요가 없음). 미설정 시 애니메이션
	// 없이 판정만 적용된다. ASC->PlayMontage()로 재생해야 전 클라이언트에 정상 복제된다(NetExecutionPolicy
	// =ServerOnly라 raw AnimInstance->Montage_Play는 서버 화면에만 보이고 리모트 클라에 전파 안 됨).
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;
};
