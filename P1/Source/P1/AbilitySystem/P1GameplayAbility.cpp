// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1GameplayAbility.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "Characters/P1CharacterBase.h"
#include "Player/P1PlayerController.h"
#include "Player/P1PlayerState.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"

UP1GameplayAbility::UP1GameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 이미 활성화된 어빌리티에 대한 press/release 입력을 클라 → 서버로 직접 복제한다.
	// 이게 false면 커스텀 입력 라우팅에서 서버 어빌리티의 InputPressed/InputReleased가
	// 절대 호출되지 않아, 홀드 콤보가 서버에서 종료되지 않고 한 타 더 나간다.
	bReplicateInputDirectly = true;

	// 게임 전역 규칙: 어빌리티 사용 중에는 다른 어빌리티를 발동할 수 없다 (WASD 이동은 어빌리티가 아니라
	// 영향 없음). State.Attacking을 모든 어빌리티가 공유해 상호 배타적으로 만든다 — 개별 어빌리티마다
	// 반복 설정할 필요 없이 베이스에서 한 번에 처리.
	ActivationOwnedTags.AddTag(TAG_State_Attacking);
	ActivationBlockedTags.AddTag(TAG_State_Attacking);

	// 사망 중엔 어떤 어빌리티도 발동 불가 (State.Dead GE가 자연 만료되면 곧 리스폰).
	ActivationBlockedTags.AddTag(TAG_State_Dead);

	// 기절 중엔 대부분의 어빌리티 발동 불가 — 기절 해제용 스킬은 Stoicism이 State.Attacking을 뺐던 것과
	// 동일한 방식으로 자기 생성자에서 ActivationBlockedTags.RemoveTag(TAG_State_Stunned)로 예외 처리한다.
	ActivationBlockedTags.AddTag(TAG_State_Stunned);
}

AP1CharacterBase* UP1GameplayAbility::GetP1CharacterFromActorInfo() const
{
	return Cast<AP1CharacterBase>(GetAvatarActorFromActorInfo());
}

AP1PlayerController* UP1GameplayAbility::GetP1PlayerControllerFromActorInfo() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	return ActorInfo ? Cast<AP1PlayerController>(ActorInfo->PlayerController.Get()) : nullptr;
}

int32 UP1GameplayAbility::GetP1CharacterLevel() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const AP1PlayerState* P1PS = ActorInfo ? Cast<AP1PlayerState>(ActorInfo->OwnerActor.Get()) : nullptr;
	return P1PS ? P1PS->GetCharacterLevel() : 1;
}

FGameplayTag UP1GameplayAbility::GetUICooldownTag() const
{
	if (const FGameplayTagContainer* CooldownTags = GetCooldownTags())
	{
		for (const FGameplayTag& Tag : *CooldownTags)
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

int32 UP1GameplayAbility::GetRequiredCharacterLevelForNextRank(int32 CurrentSpecLevel) const
{
	if (RequiredCharacterLevelPerRank.IsValidIndex(CurrentSpecLevel))
	{
		return RequiredCharacterLevelPerRank[CurrentSpecLevel];
	}
	return 1;
}

bool UP1GameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (MaxAbilityLevel > 1 && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
		{
			if (Spec->Level <= 0)
			{
				return false;
			}
		}
	}

	return true;
}

void UP1GameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
	ApplyAbilityHasteToCommittedCooldown();
}

void UP1GameplayAbility::ApplyAbilityHasteToCommittedCooldown() const
{
	// InputTag가 없는 어빌리티(아이템 반응형 어빌리티 등)는 ReduceCooldownByInputTag가 애초에 찾을 수
	// 없으므로 자연히 스킵된다 — 별도 조기 리턴 불필요, 그냥 진행해도 안전.
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UP1AbilitySystemComponent* P1ASC = ActorInfo ? Cast<UP1AbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	const UP1AttributeSet* AttrSet = P1ASC ? P1ASC->GetSet<UP1AttributeSet>() : nullptr;
	if (!P1ASC || !AttrSet || !InputTag.IsValid())
	{
		return;
	}

	// 궁극기(R)면 AbilityHaste에 UltimateHaste를 더해서 계산 — 나머지 어빌리티는 AbilityHaste만.
	float Haste = AttrSet->GetAbilityHaste();
	if (InputTag.MatchesTagExact(TAG_InputTag_Ability_R))
	{
		Haste += AttrSet->GetUltimateHaste();
	}

	if (Haste <= 0.0f)
	{
		return;
	}

	// LoL식 공식: 감소율 = Haste/(Haste+100) — 예: Haste 25 → 20% 감소.
	const float Percent = Haste / (Haste + 100.0f);
	P1ASC->ReduceCooldownByInputTag(InputTag, Percent);
}

FActiveGameplayEffectHandle UP1GameplayAbility::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass,
	FGameplayTag SetByCallerTag, float SetByCallerMagnitude) const
{
	if (!EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(EffectClass, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	if (SetByCallerTag.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, SetByCallerMagnitude);
	}

	const FActiveGameplayEffectHandle Handle = ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);

	// 이 프로젝트 대부분의 실제 스킬(MakeWay/IonStrike/PhotonDisruptor/AssaultTheGates/StoicismDeflect/
	// RocketBoots 등)은 "코스트/쿨다운 분리" 컨벤션 때문에 표준 CommitAbilityCooldown()이 아니라
	// ApplyEffectToSelf(CooldownGE->GetClass())로 쿨다운을 직접 적용한다 — 그래서 위 ApplyCooldown()
	// 오버라이드가 걸리지 않는다. 지금 적용한 EffectClass가 이 어빌리티 자신의 쿨다운 GE와 같으면
	// (모든 호출부가 GetCooldownGameplayEffect()에서 얻은 클래스를 그대로 넘기므로 이 비교만으로 충분히
	// 식별 가능 — 호출부를 일일이 고칠 필요 없음) 방금 건 쿨다운에도 동일하게 헤이스트를 적용한다.
	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (CooldownGE && EffectClass == CooldownGE->GetClass())
	{
		ApplyAbilityHasteToCommittedCooldown();
	}

	return Handle;
}
