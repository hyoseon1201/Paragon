// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "P1.h"

void UP1AbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	// 지면 조준 중: LMB=확정, RMB=취소로 리라우팅하고 나머지 어빌리티 입력은 차단(삼킴).
	// (WASD 이동은 Enhanced Input Move 액션이라 이 경로를 거치지 않아 영향 없음)
	if (HasMatchingGameplayTag(TAG_State_TargetingAbility))
	{
		if (InputTag == TAG_InputTag_Ability_BasicAttack)
		{
			LocalInputConfirm();
		}
		else if (InputTag == TAG_InputTag_Ability_RMB)
		{
			LocalInputCancel();
		}
		// 그 외 입력은 조준 중 차단.
		return;
	}

	const TArray<FGameplayAbilitySpec>& Specs = GetActivatableAbilities();
	UE_LOG(LogP1, Log, TEXT("[ASC] AbilityInputTagPressed: %s | ActivatableAbilities count=%d"),
		*InputTag.ToString(), Specs.Num());

	bool bFoundMatch = false;
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		const FGameplayTagContainer& DynTags = AbilitySpec.GetDynamicSpecSourceTags();
		UE_LOG(LogP1, Log, TEXT("[ASC]   Spec: %s | DynamicTags=%s | IsActive=%d"),
			AbilitySpec.Ability ? *AbilitySpec.Ability->GetName() : TEXT("null"),
			*DynTags.ToString(),
			AbilitySpec.IsActive() ? 1 : 0);

		if (AbilitySpec.Ability && DynTags.HasTagExact(InputTag))
		{
			bFoundMatch = true;
			AbilitySpec.InputPressed = true;

			if (AbilitySpec.IsActive())
			{
				// 이미 활성 중인 어빌리티: 서버에도 press를 전달해야 서버 인스턴스의
				// InputPressed()가 호출된다 (표준 AbilityLocalInputPressed와 동일한 처리).
				if (AbilitySpec.Ability->bReplicateInputDirectly && !IsOwnerActorAuthoritative())
				{
					ServerSetInputPressed(AbilitySpec.Handle);
				}
				AbilitySpecInputPressed(AbilitySpec);
			}
			else
			{
				const bool bActivated = TryActivateAbility(AbilitySpec.Handle);
				UE_LOG(LogP1, Log, TEXT("[ASC]   TryActivateAbility result: %d (Ability=%s)"),
					bActivated ? 1 : 0, *AbilitySpec.Ability->GetName());
			}
		}
	}

	if (!bFoundMatch)
	{
		UE_LOG(LogP1, Warning, TEXT("[ASC] No spec found matching InputTag: %s"), *InputTag.ToString());
	}
}

void UP1AbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	// 조준 중에는 release도 삼킨다 (확정/취소는 press로만 처리).
	if (HasMatchingGameplayTag(TAG_State_TargetingAbility))
	{
		return;
	}

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySpec.InputPressed = false;

			if (AbilitySpec.IsActive())
			{
				// 서버에도 release를 전달해야 서버 인스턴스의 InputReleased()가 호출된다.
				// 이게 없으면 서버는 버튼이 계속 눌린 상태로 오인해 콤보가 한 타 더 나간다.
				if (AbilitySpec.Ability->bReplicateInputDirectly && !IsOwnerActorAuthoritative())
				{
					ServerSetInputReleased(AbilitySpec.Handle);
				}
				AbilitySpecInputReleased(AbilitySpec);
			}
		}
	}
}
