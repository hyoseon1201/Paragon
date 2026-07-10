// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/WidgetController/P1OverlayWidgetController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "Engine/Texture2D.h"
#include "P1.h"

void UP1OverlayWidgetController::BroadcastInitialValues()
{
	bool bFound = false;
	OnHealthChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetHealthAttribute(), bFound));
	OnMaxHealthChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetMaxHealthAttribute(), bFound));
	OnHealthRegenChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetHealthRegenAttribute(), bFound));
	OnManaChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetManaAttribute(), bFound));
	OnMaxManaChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetMaxManaAttribute(), bFound));
	OnManaRegenChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetManaRegenAttribute(), bFound));

	// AbilitiesGiven 여부에 따라 즉시 브로드캐스트할지는 여기서(위젯이 이미 OnWidgetControllerSet에서
	// 리스너를 건 뒤 호출되는 지점) 판단해야 한다. BindCallbacksToDependencies는 위젯 생성보다 먼저
	// 실행되므로, 그 시점에 이미 AreAbilitiesGiven()==true라면 여기서가 아니라 거기서 즉시 브로드캐스트했을 때
	// 리스너가 아직 없어 브로드캐스트가 허공에 날아간다 — 실제로 겪은 버그. AreAbilitiesGiven() 체크와
	// 즉시 브로드캐스트는 반드시 이 함수(위젯 구독 이후 호출됨)에서만 한다.
	if (const UP1AbilitySystemComponent* P1ASC = Cast<UP1AbilitySystemComponent>(AbilitySystemComponent))
	{
		if (P1ASC->AreAbilitiesGiven())
		{
			BroadcastAbilityInfo();
		}
	}
}

void UP1OverlayWidgetController::BindCallbacksToDependencies()
{
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetHealthAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnHealthAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnMaxHealthAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetHealthRegenAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnHealthRegenAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetManaAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnManaAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetMaxManaAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnMaxManaAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetManaRegenAttribute())
		.AddUObject(this, &UP1OverlayWidgetController::OnManaRegenAttributeChanged);

	if (UP1AbilitySystemComponent* P1ASC = Cast<UP1AbilitySystemComponent>(AbilitySystemComponent))
	{
		UE_LOG(LogP1, Log, TEXT("[OverlayWC][AbilityIcon] BindCallbacksToDependencies — AreAbilitiesGiven=%d (여기서 이미 true여도 즉시 브로드캐스트하지 않음 — BroadcastInitialValues에서 처리)"),
			P1ASC->AreAbilitiesGiven() ? 1 : 0);
		// 항상 구독해둔다 — 아직 안 줬으면 나중에 델리게이트가 발화될 때 브로드캐스트된다.
		// 이미 줬다면(델리게이트가 과거에 이미 한 번 발화된 상태) 이 구독 자체는 조용히 무의미하고,
		// 대신 BroadcastInitialValues()(위젯이 리스너를 건 뒤 호출됨)에서 명시적으로 한 번 브로드캐스트한다.
		P1ASC->AbilitiesGivenDelegate.AddUObject(this, &UP1OverlayWidgetController::BroadcastAbilityInfo);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[OverlayWC][AbilityIcon] AbilitySystemComponent가 UP1AbilitySystemComponent가 아님 — 아이콘/쿨다운 브로드캐스트 불가"));
	}
}

void UP1OverlayWidgetController::BroadcastAbilityInfo()
{
	const TArray<FGameplayAbilitySpec>& Specs = AbilitySystemComponent->GetActivatableAbilities();
	UE_LOG(LogP1, Log, TEXT("[OverlayWC][AbilityIcon] BroadcastAbilityInfo 실행 — ActivatableAbilities count=%d"), Specs.Num());

	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		const UP1GameplayAbility* Ability = Cast<UP1GameplayAbility>(Spec.Ability);
		if (!Ability)
		{
			UE_LOG(LogP1, Warning, TEXT("[OverlayWC][AbilityIcon]   Spec.Ability가 UP1GameplayAbility가 아니거나 null — 스킵"));
			continue;
		}
		if (!Ability->InputTag.IsValid())
		{
			UE_LOG(LogP1, Log, TEXT("[OverlayWC][AbilityIcon]   %s | InputTag 없음 — 스킵"), *Ability->GetClass()->GetName());
			continue;
		}

		UE_LOG(LogP1, Log, TEXT("[OverlayWC][AbilityIcon]   %s | InputTag=%s | AbilityIcon=%s"),
			*Ability->GetClass()->GetName(), *Ability->InputTag.ToString(),
			Ability->AbilityIcon ? *Ability->AbilityIcon->GetName() : TEXT("NULL(미설정)"));

		OnAbilityIconAssigned.Broadcast(Ability->InputTag, Ability->AbilityIcon);

		// GetUICooldownTag(): 기본값은 진짜 GAS 쿨다운(CooldownGameplayEffectClass), 기본공격처럼
		// 진짜 쿨다운이 없는 어빌리티는 오버라이드해서 UI 전용 타이머 태그를 대신 반환한다.
		const FGameplayTag CooldownTag = Ability->GetUICooldownTag();
		if (!CooldownTag.IsValid())
		{
			continue;
		}

		// AnyCountChange 사용 — NewOrRemoved(기본값)는 카운트가 0↔양수로 넘어갈 때만 발동돼서,
		// 콤보처럼 이전 인스턴스가 만료되기 전에 새 인스턴스가 겹쳐 적용되는 경우(카운트 1→2→1처럼
		// 중간값끼리만 오갈 때) 콜백이 아예 안 터진다 — 기본공격 콤보 중 UI가 멈춰있던 원인.
		AbilitySystemComponent->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::AnyCountChange)
			.AddUObject(this, &UP1OverlayWidgetController::OnAbilityCooldownTagChanged,
				Ability->InputTag, TWeakObjectPtr<UGameplayAbility>(Spec.Ability), Spec.Handle);
	}
}

void UP1OverlayWidgetController::OnAbilityCooldownTagChanged(const FGameplayTag CooldownTag, int32 NewCount,
	FGameplayTag InputTag, TWeakObjectPtr<UGameplayAbility> AbilityWeak, FGameplayAbilitySpecHandle Handle)
{
	UE_LOG(LogP1, Log, TEXT("[OverlayWC][Cooldown] OnAbilityCooldownTagChanged — CooldownTag=%s NewCount=%d InputTag=%s"),
		*CooldownTag.ToString(), NewCount, *InputTag.ToString());

	if (NewCount > 0)
	{
		// Ability->GetCooldownTimeRemainingAndDuration()은 내부적으로 그 어빌리티의 네이티브
		// GetCooldownTags()(CooldownGameplayEffectClass)만 조회한다 — 기본공격처럼 CooldownGameplayEffectClass
		// 없이 GetUICooldownTag()로 별도 태그만 쓰는 경우 항상 0을 반환해버린다. 대신 지금 실제로 바뀐
		// CooldownTag를 직접 쿼리해 남은 시간을 구한다 — 네이티브/UI 전용 태그 양쪽 다 정확하게 동작.
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(CooldownTag));
		const TArray<float> TimesRemaining = AbilitySystemComponent->GetActiveEffectsTimeRemaining(Query);
		float TimeRemaining = 0.f;
		for (const float T : TimesRemaining)
		{
			TimeRemaining = FMath::Max(TimeRemaining, T);
		}
		UE_LOG(LogP1, Log, TEXT("[OverlayWC][Cooldown]   → OnCooldownStart(InputTag=%s, Duration=%.2f, ActiveEffectCount=%d)"),
			*InputTag.ToString(), TimeRemaining, TimesRemaining.Num());
		OnCooldownStart.Broadcast(InputTag, TimeRemaining);
	}
	else
	{
		UE_LOG(LogP1, Log, TEXT("[OverlayWC][Cooldown]   → OnCooldownClear(InputTag=%s)"), *InputTag.ToString());
		OnCooldownClear.Broadcast(InputTag);
	}
}

void UP1OverlayWidgetController::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)    { OnHealthChanged.Broadcast(Data.NewValue); }
void UP1OverlayWidgetController::OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data)  { OnMaxHealthChanged.Broadcast(Data.NewValue); }
void UP1OverlayWidgetController::OnHealthRegenAttributeChanged(const FOnAttributeChangeData& Data){ OnHealthRegenChanged.Broadcast(Data.NewValue); }
void UP1OverlayWidgetController::OnManaAttributeChanged(const FOnAttributeChangeData& Data)       { OnManaChanged.Broadcast(Data.NewValue); }
void UP1OverlayWidgetController::OnMaxManaAttributeChanged(const FOnAttributeChangeData& Data)    { OnMaxManaChanged.Broadcast(Data.NewValue); }
void UP1OverlayWidgetController::OnManaRegenAttributeChanged(const FOnAttributeChangeData& Data)  { OnManaRegenChanged.Broadcast(Data.NewValue); }
