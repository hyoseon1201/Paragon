// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "P1GameplayAbility.generated.h"

class AP1CharacterBase;
class AP1PlayerController;

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
};
