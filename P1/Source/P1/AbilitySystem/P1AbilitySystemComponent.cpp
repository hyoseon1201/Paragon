// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "Player/P1PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "P1.h"

void UP1AbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UP1AbilitySystemComponent, bAbilitiesGiven);
}

void UP1AbilitySystemComponent::SetAbilitiesGiven()
{
	bAbilitiesGiven = true;
	UE_LOG(LogP1, Log, TEXT("[ASC][AbilitiesGiven] SetAbilitiesGiven() 호출(서버/로컬) — Owner=%s | AbilitiesGivenDelegate 브로드캐스트"),
		GetOwnerActor() ? *GetOwnerActor()->GetName() : TEXT("null"));
	AbilitiesGivenDelegate.Broadcast();
}

void UP1AbilitySystemComponent::OnRep_AbilitiesGiven()
{
	UE_LOG(LogP1, Log, TEXT("[ASC][AbilitiesGiven] OnRep_AbilitiesGiven() 호출(리플리케이트 수신) — Owner=%s | AbilitiesGivenDelegate 브로드캐스트"),
		GetOwnerActor() ? *GetOwnerActor()->GetName() : TEXT("null"));
	AbilitiesGivenDelegate.Broadcast();
}

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

bool UP1AbilitySystemComponent::ServerInvestSkillPoint_Validate(FGameplayTag AbilityInputTag)
{
	return AbilityInputTag.IsValid();
}

void UP1AbilitySystemComponent::ServerInvestSkillPoint_Implementation(FGameplayTag AbilityInputTag)
{
	AP1PlayerState* PS = Cast<AP1PlayerState>(GetOwnerActor());
	if (!IsValid(PS) || PS->GetSkillPoints() <= 0)
	{
		UE_LOG(LogP1, Warning, TEXT("[ASC][Invest] ServerInvestSkillPoint: 남은 스킬 포인트 없음 — %s"), *AbilityInputTag.ToString());
		return;
	}

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.Ability || !Spec.GetDynamicSpecSourceTags().HasTagExact(AbilityInputTag))
		{
			continue;
		}

		const UP1GameplayAbility* AbilityCDO = Cast<UP1GameplayAbility>(Spec.Ability);
		const int32 MaxLevel = AbilityCDO ? AbilityCDO->MaxAbilityLevel : 1;
		if (Spec.Level >= MaxLevel)
		{
			UE_LOG(LogP1, Warning, TEXT("[ASC][Invest] ServerInvestSkillPoint: 이미 최대 레벨(%d) — %s"), MaxLevel, *AbilityInputTag.ToString());
			return;
		}

		// R처럼 특정 캐릭터 레벨(6/11/15 등)에 도달해야 다음 랭크가 풀리는 어빌리티 — 포인트가 있고
		// 아직 최대 레벨이 아니어도 이 조건을 통과 못하면 투자할 수 없다.
		const int32 RequiredCharacterLevel = AbilityCDO ? AbilityCDO->GetRequiredCharacterLevelForNextRank(Spec.Level) : 1;
		if (PS->GetCharacterLevel() < RequiredCharacterLevel)
		{
			UE_LOG(LogP1, Warning, TEXT("[ASC][Invest] ServerInvestSkillPoint: 캐릭터 레벨 부족(필요=%d, 현재=%d) — %s"),
				RequiredCharacterLevel, PS->GetCharacterLevel(), *AbilityInputTag.ToString());
			return;
		}

		Spec.Level += 1;
		MarkAbilitySpecDirty(Spec);
		PS->SpendSkillPoint();

		UE_LOG(LogP1, Log, TEXT("[ASC][Invest] 포인트 투자 — %s → Level %d (남은 SkillPoints=%d)"),
			*AbilityInputTag.ToString(), Spec.Level, PS->GetSkillPoints());
		return;
	}

	UE_LOG(LogP1, Warning, TEXT("[ASC][Invest] ServerInvestSkillPoint: 매칭되는 어빌리티 스펙 없음 — %s"), *AbilityInputTag.ToString());
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
