// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "P1.h"

void UP1AbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
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
				AbilitySpecInputPressed(AbilitySpec);
			}
			else
			{
				const bool bActivated = TryActivateAbility(AbilitySpec.Handle);
				UE_LOG(LogP1, Log, TEXT("[ASC]   TryActivateAbility result: %d"), bActivated ? 1 : 0);
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

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySpec.InputPressed = false;

			if (AbilitySpec.IsActive())
			{
				AbilitySpecInputReleased(AbilitySpec);
			}
		}
	}
}
