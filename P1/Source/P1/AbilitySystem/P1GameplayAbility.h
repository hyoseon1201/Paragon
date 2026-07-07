// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "P1GameplayAbility.generated.h"

class AP1CharacterBase;
class AP1PlayerController;
class UGameplayEffect;

// 프로젝트의 모든 GameplayAbility가 상속하는 베이스 클래스.
UCLASS(Abstract)
class P1_API UP1GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility();

	// BP 서브클래스에서 설정. GiveAbility 시 이 태그를 Spec.DynamicAbilityTags에 추가해
	// AbilityInputTagPressed/Released 라우팅에 사용한다.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTag InputTag;

protected:
	AP1CharacterBase* GetP1CharacterFromActorInfo() const;
	AP1PlayerController* GetP1PlayerControllerFromActorInfo() const;

	// 자신(어빌리티 소유자)에게 GE를 적용하는 공통 헬퍼 — 버프/디버프/쿨다운 등
	// "어빌리티가 자기 자신에게 이펙트를 건다" 패턴 전반에서 재사용한다.
	// SetByCallerTag가 유효하면 SetByCallerMagnitude를 함께 실어 보낸다 (지속시간, 세기 등).
	// MakeOutgoingGameplayEffectSpec/ApplyGameplayEffectSpecToOwner를 사용해 어빌리티의
	// 예측 키(prediction key) 컨텍스트를 올바르게 태운다 (raw ASC->ApplyGameplayEffectSpecToSelf보다 정석적).
	FActiveGameplayEffectHandle ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass,
		FGameplayTag SetByCallerTag = FGameplayTag(), float SetByCallerMagnitude = 0.0f) const;
};
