// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Dekker/P1GameplayAbility_PhotonDisruptor.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Characters/P1CharacterBase.h"
#include "GameFramework/PlayerController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

UP1GameplayAbility_PhotonDisruptor::UP1GameplayAbility_PhotonDisruptor()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_PhotonDisruptor);
	SetAssetTags(Tags);

	InputTag = TAG_InputTag_Ability_Q;
}

void UP1GameplayAbility_PhotonDisruptor::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentTick = 0;
	bCooldownApplied = false;

	if (ActorInfo->IsNetAuthority())
	{
		AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
		if (IsValid(SourceCharacter))
		{
			// 시작~끝 지점(지표면)을 캐스트 시점에 한 번 계산해두고, 매 블래스트 틱마다 이 사이를 선형보간한다.
			const FVector CharacterLocation = SourceCharacter->GetActorLocation();
			const FVector AimDirXY = GetAimDirectionXY();

			GroundStart = SnapToGround(CharacterLocation, SourceCharacter);
			GroundEnd = SnapToGround(CharacterLocation + AimDirXY * SweepRange, SourceCharacter);

			SweepStartTime = GetWorld()->GetTimeSeconds();

			// 하늘의 드론/빔 비주얼 — 자체 완결형 캐스케이드 이펙트를 Start~End 사이로 이동시키며 재생한다.
			// Start/End/Duration이 결정적이라 각 클라이언트가 로컬 타이머만으로 독립 재생(위치 리플리케이션 불필요).
			if (SkyDronePodEffect)
			{
				const FVector SkyStart = GroundStart + FVector(0.0f, 0.0f, SkyHeight);
				const FVector SkyEnd = GroundEnd + FVector(0.0f, 0.0f, SkyHeight);
				SourceCharacter->MulticastPlayMovingParticleEffect(SkyDronePodEffect, SkyStart, SkyEnd, SweepDuration);
			}

			GetWorld()->GetTimerManager().SetTimer(BlastTimerHandle, this,
				&UP1GameplayAbility_PhotonDisruptor::OnBlastTick, BlastTickInterval, true);

			UE_LOG(LogP1, Log, TEXT("[PhotonDisruptor] 발동 — Start=%s End=%s SweepDuration=%.2f초 %.1f초 간격"),
				*GroundStart.ToString(), *GroundEnd.ToString(), SweepDuration, BlastTickInterval);
		}
	}

	if (CastMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, CastMontage, 1.0f);
		// 스윕 지속시간과 별개로 재생만 하고 끝 — 어빌리티 종료는 OnBlastTick이 전 틱 소진 시점에 담당한다.
		MontageTask->ReadyForActivation();
	}
}

void UP1GameplayAbility_PhotonDisruptor::OnBlastTick()
{
	++CurrentTick;

	if (CurrentActorInfo->IsNetAuthority())
	{
		AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
		if (IsValid(SourceCharacter))
		{
			const float Alpha = FMath::Clamp(static_cast<float>((GetWorld()->GetTimeSeconds() - SweepStartTime) / SweepDuration), 0.0f, 1.0f);
			const FVector CurrentGroundPoint = FMath::Lerp(GroundStart, GroundEnd, Alpha);

			const TArray<AActor*> Enemies = GetEnemiesInRadius(CurrentGroundPoint, BlastRadius, BlastHalfHeight);

			UE_LOG(LogP1, Log, TEXT("[PhotonDisruptor] Tick %d (Alpha=%.2f) — Point=%s EnemyCount=%d"),
				CurrentTick, Alpha, *CurrentGroundPoint.ToString(), Enemies.Num());

			for (AActor* Enemy : Enemies)
			{
				AP1CharacterBase* TargetCharacter = Cast<AP1CharacterBase>(Enemy);
				if (!IsValid(TargetCharacter))
				{
					continue;
				}

				ApplyDamageToTarget(TargetCharacter, 1.0f);

				if (SlowDebuffEffectClass)
				{
					if (IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetCharacter))
					{
						if (UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent())
						{
							if (UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo())
							{
								FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
								Ctx.AddSourceObject(SourceCharacter);
								const FGameplayEffectSpecHandle DebuffSpec = SourceASC->MakeOutgoingSpec(
									SlowDebuffEffectClass, GetAbilityLevel(), Ctx);
								if (DebuffSpec.IsValid())
								{
									// GE의 MovementSpeed 모디파이어는 Modifier Op=Multiply로 이 값을 그대로 곱한다 —
									// 즉 "얼마나 느려지는지"가 아니라 "원래 속도의 몇 %가 남는지"를 넣어야 한다
									// (30% 슬로우 = 70%가 남음 = 0.7을 곱함). MMC 없이 GE 에셋만으로 구성하기 위한 선택.
									const float RemainingSpeedFraction = 1.0f - SlowPercent.GetValueAtLevel(GetAbilityLevel());
									DebuffSpec.Data->SetSetByCallerMagnitude(TAG_Data_DebuffMagnitude, RemainingSpeedFraction);
									SourceASC->ApplyGameplayEffectSpecToTarget(*DebuffSpec.Data.Get(), TargetASC);
								}
							}
						}
					}
				}
			}

			if (GroundBlastEffect)
			{
				SourceCharacter->MulticastPlayParticleEffectAtLocation(GroundBlastEffect, CurrentGroundPoint, FRotator::ZeroRotator, GroundBlastEffectScale);
			}

#if ENABLE_DRAW_DEBUG
			if (bShowDebug)
			{
				DrawDebugSphere(GetWorld(), CurrentGroundPoint, BlastRadius, 16, FColor::Purple, false, BlastTickInterval * 0.9f, 0, 2.0f);
			}
#endif
		}
	}

	if (GetWorld()->GetTimeSeconds() - SweepStartTime >= SweepDuration)
	{
		GetWorld()->GetTimerManager().ClearTimer(BlastTimerHandle);

		if (!bCooldownApplied)
		{
			const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
			if (CooldownGE)
			{
				ApplyEffectToSelf(CooldownGE->GetClass());
			}
			bCooldownApplied = true;
		}

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UP1GameplayAbility_PhotonDisruptor::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BlastTimerHandle);
	}

	// 중간에 취소/중단되어 전 틱을 다 못 채웠어도 코스트는 이미 소모됐으므로 쿨다운은 적용한다(MakeWay와 동일 이유).
	if (!bCooldownApplied)
	{
		const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
		if (CooldownGE)
		{
			ApplyEffectToSelf(CooldownGE->GetClass());
		}
		bCooldownApplied = true;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FVector UP1GameplayAbility_PhotonDisruptor::GetAimDirectionXY() const
{
	const AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
	if (!IsValid(SourceCharacter))
	{
		return FVector::ForwardVector;
	}

	FVector RawDirection = SourceCharacter->GetActorForwardVector();
	if (const APlayerController* PC = Cast<APlayerController>(SourceCharacter->GetController()))
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
		RawDirection = ViewRotation.Vector();
	}

	// 지표면 스윕이라 수직 성분은 버리고 수평 방향만 사용한다.
	return FVector(RawDirection.X, RawDirection.Y, 0.0f).GetSafeNormal();
}

FVector UP1GameplayAbility_PhotonDisruptor::SnapToGround(const FVector& XYSource, const AActor* IgnoreActor) const
{
	FCollisionQueryParams Params;
	if (IgnoreActor)
	{
		Params.AddIgnoredActor(IgnoreActor);
	}

	const FVector TraceStart = XYSource + FVector(0.0f, 0.0f, 1000.0f);
	const FVector TraceEnd = XYSource - FVector(0.0f, 0.0f, 3000.0f);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		return Hit.ImpactPoint;
	}

	return XYSource;
}
