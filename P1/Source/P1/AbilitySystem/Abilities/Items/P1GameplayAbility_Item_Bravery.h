// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "P1GameplayAbility_Item_Bravery.generated.h"

class UGameplayEffect;
class UNiagaraSystem;

// 사면(아이템) 고유효과 "용기" — Event.Character.Immobilized(이동 불가류 CC에 걸리는 순간, AP1HeroCharacter::
// OnImmobilizedTagChanged 발신)로 트리거되는 반응형 패시브. 즉시 자신에게 BuffEffectClass(짧은 지속시간
// 피해 감소+이동속도 버프)를 적용하고 자기 쿨다운을 커밋한다 — 그 외 아무 판정도 하지 않는다("이동 불가에
// 걸렸는지"는 이미 발신자가 확인하고 보낸 이벤트이므로).
//
// StoicismDeflect와 두 가지가 다르다: (1) 발신자(AP1HeroCharacter)가 쿨다운을 미리 체크하지 않고 무조건
// 이벤트를 보내므로 — "지금 사용 가능한지"는 이 어빌리티 자신의 CooldownGameplayEffectClass를 통해 엔진의
// 표준 CanActivateAbility() 쿨다운 체크가 자동으로 걸러준다(Event.Character.BasicAttackHitDealt와 동일한
// "발신자는 아무것도 모른다" 원칙). (2) 쿨다운 지속시간이 캐릭터 레벨에 따라 스케일링될 필요가 없어(고정
// 45초) SetByCaller 없이 CooldownGameplayEffectClass를 그대로 재사용.
UCLASS()
class P1_API UP1GameplayAbility_Item_Bravery : public UP1GameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_Item_Bravery();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 3초 지속시간, DamageReduction(+0.25)/MovementSpeed(Multiply×1.40) 버프 GE.
	UPROPERTY(EditDefaultsOnly, Category = "Bravery")
	TSubclassOf<UGameplayEffect> BuffEffectClass;

	// 발동 순간(버프 적용 시점) 1회 재생되는 연출(fire-and-forget, AP1CharacterBase::MulticastPlayNiagaraEffect
	// 경유) — 미설정 시 재생 생략. 소켓 미지정(NAME_None)이면 캐릭터 위치에 재생.
	UPROPERTY(EditDefaultsOnly, Category = "Bravery|VFX")
	TObjectPtr<UNiagaraSystem> ShieldEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Bravery|VFX")
	FName ShieldEffectSocketName = NAME_None;
};
