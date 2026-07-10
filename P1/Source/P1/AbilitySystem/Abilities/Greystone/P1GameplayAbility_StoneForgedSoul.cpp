// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Greystone/P1GameplayAbility_StoneForgedSoul.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "Characters/P1CharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

UP1GameplayAbility_StoneForgedSoul::UP1GameplayAbility_StoneForgedSoul()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_StoneForgedSoul);
	SetAssetTags(Tags);

	InputTag = TAG_InputTag_Ability_R;
}

void UP1GameplayAbility_StoneForgedSoul::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CastMontage)
	{
		UE_LOG(LogP1, Warning, TEXT("[StoneForgedSoul] CastMontage 미설정 — GA BP에서 지정하세요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// --- 회복: 잃은 체력 비율에 따라 최대 MaxHealAmplification배까지 증폭 (단순 계수 곱셈으로 표현
	// 안 되는 공식이라 ExecCalc 채널 대신 C++에서 최종값을 계산해 SetByCaller로 그대로 넘긴다) ---
	if (HealEffectClass)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			if (const UP1AttributeSet* AttrSet = ASC->GetSet<UP1AttributeSet>())
			{
				const float MaxHealth = AttrSet->GetMaxHealth();
				const float CurrentHealth = AttrSet->GetHealth();
				const float MissingRatio = MaxHealth > 0.0f
					? FMath::Clamp((MaxHealth - CurrentHealth) / MaxHealth, 0.0f, 1.0f)
					: 0.0f;
				const float AmplificationMultiplier = FMath::Lerp(1.0f, MaxHealAmplification, MissingRatio);
				const float HealAmount = HealPercent.GetValueAtLevel(GetAbilityLevel()) * MaxHealth * AmplificationMultiplier;

				ApplyEffectToSelf(HealEffectClass, TAG_Data_Heal_Flat, HealAmount);
				UE_LOG(LogP1, Log, TEXT("[StoneForgedSoul] 회복 적용 — MissingRatio=%.2f Multiplier=%.2f HealAmount=%.1f"),
					MissingRatio, AmplificationMultiplier, HealAmount);
			}
		}
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[StoneForgedSoul] HealEffectClass 미설정 — 회복 스킵"));
	}

	// --- 무적(스테이시스) — 지속시간은 GE 자체에 고정, State.Invulnerable은 AttributeSet이 공용으로 체크 ---
	if (InvulnerabilityEffectClass)
	{
		ApplyEffectToSelf(InvulnerabilityEffectClass);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[StoneForgedSoul] InvulnerabilityEffectClass 미설정 — 무적 스킵"));
	}

	// --- 비행 모드 전환: 몽타주 루트모션(상승)이 중력과 싸우지 않도록 ---
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Flying);
		}
	}

	// --- 근접도 비례 슬로우 재스캔 타이머 시작 ---
	GetWorld()->GetTimerManager().SetTimer(SlowTickTimerHandle, this,
		&UP1GameplayAbility_StoneForgedSoul::OnSlowTick, SlowTickPeriod, true);

	// --- 착지 프레임 이벤트 대기 ---
	CrashEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, TAG_Event_Montage_StoneForgedSoul_Crash, nullptr, true);
	CrashEventTask->EventReceived.AddDynamic(this, &UP1GameplayAbility_StoneForgedSoul::OnCrashEventReceived);
	CrashEventTask->ReadyForActivation();

	// --- 몽타주 재생 (캐스팅+상승+공중대기+하강+착지 전부 포함) ---
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, CastMontage, 1.0f);
	MontageTask->OnCompleted.AddDynamic(this, &UP1GameplayAbility_StoneForgedSoul::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UP1GameplayAbility_StoneForgedSoul::OnMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &UP1GameplayAbility_StoneForgedSoul::OnMontageFinished);
	MontageTask->ReadyForActivation();

	UE_LOG(LogP1, Log, TEXT("[StoneForgedSoul] 발동 — 스테이시스 시작"));
}

void UP1GameplayAbility_StoneForgedSoul::OnSlowTick()
{
	// 데미지/디버프 판정은 서버에서만 수행 (LocalPredicted 클라 예측 인스턴스에서도 이 타이머가 돌기 때문).
	if (!CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceCharacter) || !ASC)
	{
		return;
	}

	const FVector Center = SourceCharacter->GetActorLocation();
	const TArray<AActor*> Enemies = GetEnemiesInRadius(Center, SlowRadius, SlowHalfHeight);

	for (AActor* Enemy : Enemies)
	{
		const float Distance = FVector::Dist(Center, Enemy->GetActorLocation());
		const float Alpha = FMath::Clamp(1.0f - (Distance / SlowRadius), 0.0f, 1.0f);
		const float SlowPct = Alpha * MaxSlowPercent;
		if (SlowPct <= 0.0f || !SlowDebuffEffectClass)
		{
			continue;
		}

		if (IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(Enemy))
		{
			if (UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent())
			{
				FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
				Ctx.AddSourceObject(SourceCharacter);
				const FGameplayEffectSpecHandle DebuffSpec = ASC->MakeOutgoingSpec(SlowDebuffEffectClass, GetAbilityLevel(), Ctx);
				if (DebuffSpec.IsValid())
				{
					// GE의 MovementSpeed Modifier는 Operation=Multiply라 "곱할 배수"를 기대한다.
					// SlowPct는 "감소율"(0.7=70% 감소)이므로 배수(1-SlowPct)로 변환해서 보낸다.
					DebuffSpec.Data->SetSetByCallerMagnitude(TAG_Data_DebuffMagnitude, 1.0f - SlowPct);
					ASC->ApplyGameplayEffectSpecToTarget(*DebuffSpec.Data.Get(), TargetASC);
				}
			}
		}
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		DrawDebugSphere(GetWorld(), Center, SlowRadius, 24, FColor::Cyan, false, SlowTickPeriod * 0.9f, 0, 2.0f);
	}
#endif
}

void UP1GameplayAbility_StoneForgedSoul::OnCrashEventReceived(FGameplayEventData Payload)
{
	UE_LOG(LogP1, Log, TEXT("[StoneForgedSoul] Crash 이벤트 수신 — Auth=%d"), CurrentActorInfo->IsNetAuthority() ? 1 : 0);

	// 비행 모드 해제 — 무브먼트 모드 전환은 GAS 데미지처럼 권위 게이팅 대상이 아니라 클라/서버 양쪽에서 실행.
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			if (Movement->MovementMode == MOVE_Flying)
			{
				Movement->SetMovementMode(MOVE_Walking);
			}
		}
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SlowTickTimerHandle);
	}

	// 범위 데미지 판정은 서버에서만.
	if (!CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
	if (!IsValid(SourceCharacter))
	{
		return;
	}

	const FVector Center = SourceCharacter->GetActorLocation();
	const TArray<AActor*> Enemies = GetEnemiesInRadius(Center, CrashRadius, CrashHalfHeight);

	UE_LOG(LogP1, Log, TEXT("[StoneForgedSoul] 착지 데미지 — EnemyCount=%d"), Enemies.Num());

	for (AActor* Enemy : Enemies)
	{
		ApplyDamageToTarget(Enemy, 1.0f);
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		DrawDebugSphere(GetWorld(), Center, CrashRadius, 24, FColor::Red, false, 1.5f, 0, 3.0f);
	}
#endif
}

void UP1GameplayAbility_StoneForgedSoul::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UP1GameplayAbility_StoneForgedSoul::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SlowTickTimerHandle);
	}

	if (CrashEventTask)
	{
		CrashEventTask->EndTask();
		CrashEventTask = nullptr;
	}

	// 몽타주가 Crash 노티파이 없이 중간에 끊긴 경우를 대비한 안전장치 — Flying 모드에 갇히지 않게.
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			if (Movement->MovementMode == MOVE_Flying)
			{
				Movement->SetMovementMode(MOVE_Walking);
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
