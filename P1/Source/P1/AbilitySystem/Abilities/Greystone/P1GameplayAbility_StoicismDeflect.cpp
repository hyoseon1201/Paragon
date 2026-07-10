// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Greystone/P1GameplayAbility_StoicismDeflect.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UP1GameplayAbility_StoicismDeflect::UP1GameplayAbility_StoicismDeflect()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_StoicismDeflect);
	SetAssetTags(Tags);

	// 백그라운드 반응형 패시브 — 다른 어빌리티를 쓰는 중이어도(State.Attacking 보유 중이어도) 항상
	// 발동 가능해야 한다. 베이스가 전 어빌리티에 공통으로 거는 상호배타 규칙에서 제외.
	ActivationBlockedTags.RemoveTag(TAG_State_Attacking);
	ActivationOwnedTags.RemoveTag(TAG_State_Attacking);

	// AttributeSet이 보내는 이벤트로만 트리거 — 플레이어 입력으로 직접 발동하지 않음.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = TAG_Event_StoicismDeflect_Consumed;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// 쿨다운 커밋용 순수 서버 로직 — 클라 예측이 필요 없다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UP1GameplayAbility_StoicismDeflect::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// "이 캐릭터는 Stoicism 디플렉트를 갖고 있다"를 AttributeSet이 태그만으로(어빌리티 클래스 참조 없이)
	// 판별할 수 있도록, 자신의 Asset Tag를 루즈 태그로도 ASC에 심어둔다.
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		ASC->AddLooseGameplayTag(TAG_Ability_StoicismDeflect);

		UE_LOG(LogP1, Log, TEXT("[StoicismDeflect] OnGiveAbility — %s에 Ability.StoicismDeflect 루즈 태그 부여됨 (Auth=%d)"),
			ActorInfo->AvatarActor.IsValid() ? *ActorInfo->AvatarActor->GetName() : TEXT("null"),
			ActorInfo->IsNetAuthority() ? 1 : 0);

		// 디버깅용 — 쿨다운 태그가 붙고 떨어지는 순간을 로그로 남겨 "지금 준비 상태인지"를 눈으로 확인할 수 있게 한다.
		ASC->RegisterGameplayTagEvent(TAG_Cooldown_Ability_StoicismDeflect, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UP1GameplayAbility_StoicismDeflect::OnCooldownTagChanged);
	}
}

void UP1GameplayAbility_StoicismDeflect::OnCooldownTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		UE_LOG(LogP1, Log, TEXT("[StoicismDeflect] 쿨다운 시작 (Cooldown.Ability.StoicismDeflect 부여됨) — 사용 불가"));
	}
	else
	{
		UE_LOG(LogP1, Log, TEXT("[StoicismDeflect] 쿨다운 종료 (태그 제거됨) — 다시 사용 가능"));
	}
}

void UP1GameplayAbility_StoicismDeflect::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// "사용 가능한지"는 AttributeSet이 네이티브 쿨다운 태그 부재로 이미 확인하고 나서 이 이벤트를
	// 보낸 것이므로, 여기선 그냥 쿨다운만 적용하면 된다. CommitAbilityCooldown() 대신 ApplyEffectToSelf로
	// 직접 적용하는 이유: 전자는 CooldownGameplayEffectClass의 Duration을 GetAbilityLevel()(스킬 랭크)로
	// 평가하는데, 이 쿨다운은 캐릭터 레벨(GetP1CharacterLevel()) 기준이어야 하기 때문 — BaseCooldown을
	// SetByCaller로 직접 계산해 넣는다(AssaultTheGates::ApplyCooldownWithDuration과 동일 패턴).
	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (CooldownGE)
	{
		const float Duration = BaseCooldown.GetValueAtLevel(GetP1CharacterLevel());
		ApplyEffectToSelf(CooldownGE->GetClass(), TAG_Data_CooldownDuration, Duration);
		UE_LOG(LogP1, Log, TEXT("[StoicismDeflect] 트리거 활성화됨 — 쿨다운 %.2f초 적용 (CharacterLevel=%d)"),
			Duration, GetP1CharacterLevel());
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[StoicismDeflect] CooldownGameplayEffectClass 미설정 — 쿨다운 없음"));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
