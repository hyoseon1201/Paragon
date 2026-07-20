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

	// 지면 조준 중: LMB=확정, InputTag.Ability.Cancel(F 등 전용 키)=취소로 리라우팅하고 나머지 어빌리티
	// 입력은 차단(삼킴). 취소를 예전처럼 "조준을 시작한 스킬 키(RMB 등) 재입력"으로 처리하지 않는 이유 —
	// 홀드해서 조준하고 키를 떼야 발사하는 스킬은 "떼는 순간"이 이미 확정 동작이라, 같은 키로는 취소를
	// 표현할 방법이 없다(눌렀다 뗄 때마다 매번 확정돼버림). 전용 취소 키로 분리하면 조준을 어떤 스킬
	// 키(RMB/Q/E 등)로 시작했든, 입력 방식이 탭이든 홀드든 상관없이 항상 하나의 방법으로 취소된다.
	// (WASD 이동은 Enhanced Input Move 액션이라 이 경로를 거치지 않아 영향 없음)
	if (HasMatchingGameplayTag(TAG_State_TargetingAbility))
	{
		if (InputTag == TAG_InputTag_Ability_BasicAttack)
		{
			LocalInputConfirm();
		}
		else if (InputTag == TAG_InputTag_Ability_Cancel)
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

	// 조준 중이어도 release는 삼키지 않고 아래 루프로 그대로 통과시킨다 — 조준을 시작한 스킬 자신의
	// InputTag(예: RMB)에 대한 release는 여전히 그 어빌리티(활성 중인 스펙)에게 정상 전달돼야 한다.
	// Stasis Bomb처럼 "누르고 있으면 조준, 떼는 순간 발사"하는 스킬은 이 release 이벤트 자체가 발사
	// 트리거라서(InputReleased() 오버라이드), 예전처럼 여기서 전부 삼키면 영원히 발사되지 않았다.
	// 아래 루프는 원래도 "활성 중인 스펙과 태그가 일치하는 입력만" 전달하므로, 조준과 무관한 다른
	// 어빌리티가 실수로 활성화될 위험은 없다(그건 AbilityInputTagPressed 쪽에서만 막으면 되는 문제).
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
