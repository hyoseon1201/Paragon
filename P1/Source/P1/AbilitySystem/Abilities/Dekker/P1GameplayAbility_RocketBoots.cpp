// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Dekker/P1GameplayAbility_RocketBoots.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Characters/P1CharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UP1GameplayAbility_RocketBoots::UP1GameplayAbility_RocketBoots()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_RocketBoots);
	SetAssetTags(Tags);

	InputTag = TAG_InputTag_Ability_Passive;
}

void UP1GameplayAbility_RocketBoots::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// 서버 쪽 ASC 인스턴스에만 구독 — 쿨다운 감소(GE 재적용)는 서버 권위 로직이므로, 클라이언트 예측
	// 인스턴스에서 같은 델리게이트가 별도로 불려도 여기선 아예 무시하는 게 아니라 애초에 구독하지 않는다.
	if (ActorInfo && ActorInfo->IsNetAuthority() && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->AbilityActivatedCallbacks.AddUObject(
			this, &UP1GameplayAbility_RocketBoots::OnAnyAbilityActivated);
	}
}

void UP1GameplayAbility_RocketBoots::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Character = Cast<ACharacter>(GetP1CharacterFromActorInfo());
	if (!IsValid(Character) || !Character->GetCharacterMovement() || !Character->GetCharacterMovement()->IsFalling())
	{
		// 착지 상태에서 잘못 트리거됐거나 캐릭터를 못 찾음 — 코스트/쿨다운 없이 조용히 종료.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ApplyCooldownWithDuration(BaseCooldown.GetValueAtLevel(GetP1CharacterLevel()));

	// 기존 XY 속도는 유지하고(false) Z만 새 값으로 덮어써(true) 낙하 속도와 무관하게 항상 동일한
	// 추진력을 낸다 — LocalPredicted라 서버/소유 클라 양쪽에서 실행되며, CharacterMovementComponent의
	// 표준 이동 리플리케이션/보정이 나머지를 알아서 처리한다(RMB의 루트모션 도약과 동일한 이유로
	// IsNetAuthority 게이팅 불필요).
	Character->LaunchCharacter(FVector(0.0f, 0.0f, LaunchVelocityZ), false, true);

	if (LaunchMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, LaunchMontage, 1.0f);
		MontageTask->ReadyForActivation();
	}

	UE_LOG(LogP1, Log, TEXT("[RocketBoots] 발동 — %s (CharacterLevel=%d)"), *Character->GetName(), GetP1CharacterLevel());

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UP1GameplayAbility_RocketBoots::OnAnyAbilityActivated(UGameplayAbility* ActivatedAbility)
{
	if (ActivatedAbility == this)
	{
		return;
	}

	ReduceCooldownBySeconds(CooldownReductionOnAbilityUse);
}

void UP1GameplayAbility_RocketBoots::ApplyCooldownWithDuration(float InDuration)
{
	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		UE_LOG(LogP1, Warning, TEXT("[RocketBoots] CooldownGameplayEffectClass 미설정 — 쿨다운 없음"));
		return;
	}

	ApplyEffectToSelf(CooldownGE->GetClass(), TAG_Data_CooldownDuration, InDuration);
}

void UP1GameplayAbility_RocketBoots::ReduceCooldownBySeconds(float InSeconds)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FGameplayTagContainer CooldownTags;
	CooldownTags.AddTag(TAG_Cooldown_Ability_RocketBoots);

	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);
	const TArray<float> Remaining = ASC->GetActiveEffectsTimeRemaining(Query);

	float MaxRemaining = 0.0f;
	for (const float R : Remaining)
	{
		MaxRemaining = FMath::Max(MaxRemaining, R);
	}
	if (MaxRemaining <= 0.0f)
	{
		// 쿨다운 중이 아님(이미 사용 가능) — 감소시킬 게 없다.
		return;
	}

	const float ReducedRemaining = FMath::Max(0.0f, MaxRemaining - InSeconds);
	UE_LOG(LogP1, Log, TEXT("[RocketBoots][Cooldown] 다른 어빌리티 사용으로 감소: 남은시간 %.2f → %.2f"), MaxRemaining, ReducedRemaining);

	ASC->RemoveActiveEffectsWithGrantedTags(CooldownTags);
	if (ReducedRemaining > 0.0f)
	{
		ApplyCooldownWithDuration(ReducedRemaining);
	}
}
