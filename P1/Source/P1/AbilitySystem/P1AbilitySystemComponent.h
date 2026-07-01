// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "P1AbilitySystemComponent.generated.h"

// 어빌리티 Spec의 DynamicAbilityTags에 입력 태그(예: InputTag.Ability.Q)를 달아두고,
// 입력 press/release가 들어오면 즉시 해당 태그를 가진 Spec을 찾아 활성화/이벤트 전달한다.
UCLASS()
class P1_API UP1AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
};
