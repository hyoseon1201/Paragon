// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Dekker/P1GameplayAbility_PhotonDisruptor.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystem/TargetActors/P1TargetActor_DirectionalGroundTargetBase.h"
#include "Characters/P1CharacterBase.h"
#include "GameFramework/PlayerController.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
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

	// State.Rooted는 여기서 걸지 않는다 — 조준 단계는 자유 이동, 확정 후 실제 스윕 구간에서만 SetRootedState(true)로
	// 수동 부여한다(아래 헬퍼 참고).
}

void UP1GameplayAbility_PhotonDisruptor::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// CanActivate(코스트/쿨다운)는 TryActivate에서 이미 통과. 여기서는 커밋하지 않고 조준 단계로 진입.
	if (!TargetActorClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[PhotonDisruptor] TargetActorClass 미설정 — GA BP에서 지정하세요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BeginTargeting();
}

void UP1GameplayAbility_PhotonDisruptor::BeginTargeting()
{
	SetTargetingState(true);

	UAbilityTask_WaitTargetData* Task = UAbilityTask_WaitTargetData::WaitTargetData(
		this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	if (!Task)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	Task->ValidData.AddDynamic(this, &UP1GameplayAbility_PhotonDisruptor::OnTargetDataReady);
	Task->Cancelled.AddDynamic(this, &UP1GameplayAbility_PhotonDisruptor::OnTargetCancelled);

	AGameplayAbilityTargetActor* SpawnedActor = nullptr;
	const bool bSpawned = Task->BeginSpawningActor(this, TargetActorClass, SpawnedActor);
	if (bSpawned)
	{
		if (AP1TargetActor_DirectionalGroundTargetBase* Corridor = Cast<AP1TargetActor_DirectionalGroundTargetBase>(SpawnedActor))
		{
			// 인디케이터 폭을 실제 판정 폭(BlastRadius의 지름)과 맞춰, 보이는 그대로 맞는 정확한 미리보기를 제공한다.
			Corridor->Configure(SweepRange, BlastRadius * 2.0f);
		}
		Task->FinishSpawningActor(this, SpawnedActor);
	}

	Task->ReadyForActivation();
}

void UP1GameplayAbility_PhotonDisruptor::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
	SetTargetingState(false);

	const FVector ConfirmedEnd = UAbilitySystemBlueprintLibrary::GetTargetDataEndPoint(Data, 0);

	// 확정 시 코스트+쿨다운을 함께 커밋 — 스킬샷 계열(짧은 SweepDuration=1.6초짜리 발사체 연출)이라
	// "쓰는 순간 쿨다운이 돈다"는 일반적인 MOBA 컨벤션을 따른다(채널링형인 MakeWay/IonStrike와 다른 점 —
	// 그쪽은 지속시간 내내 스킬을 "쓰는 중"이라 지속시간이 끝나야 쿨다운이 도는 게 자연스러움). 감당
	// 못하면 무취소 종료(조준만 했을 뿐 아무것도 소모 안 됨).
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		UE_LOG(LogP1, Log, TEXT("[PhotonDisruptor] 커밋 실패(코스트/쿨다운) — 취소"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	CurrentTick = 0;
	SetRootedState(true);

	if (CurrentActorInfo->IsNetAuthority())
	{
		AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
		if (IsValid(SourceCharacter))
		{
			// 시작 지점은 서버 기준 현재 위치로 재계산하고, 끝점은 클라이언트가 확정한(조준 인디케이터가
			// 보여준) 지점을 그대로 쓴다(AssaultTheGates가 확정된 착지 위치를 그대로 쓰는 것과 동일한 이유).
			GroundStart = SnapToGround(SourceCharacter->GetActorLocation(), SourceCharacter);
			GroundEnd = ConfirmedEnd;

			SweepStartTime = GetWorld()->GetTimeSeconds();

			// 하늘의 드론/빔 비주얼 — 자체 완결형 캐스케이드 이펙트를 Start~End 사이로 이동시키며 재생한다.
			// Start/End/Duration이 결정적이라 각 클라이언트가 로컬 타이머만으로 독립 재생(위치 리플리케이션 불필요).
			if (SkyDronePodEffect)
			{
				const FVector SkyStart = GroundStart + FVector(0.0f, 0.0f, SkyHeight);
				const FVector SkyEnd = GroundEnd + FVector(0.0f, 0.0f, SkyHeight);
				SourceCharacter->MulticastPlayMovingParticleEffect(SkyDronePodEffect, SkyStart, SkyEnd, SweepDuration);
			}

			// InFirstDelay=0.0f — 기본값(생략 시 InRate와 동일)이면 첫 판정이 BlastTickInterval만큼 밀려서
			// 실행되는 시점엔 이미 Alpha>0(캐릭터 위치를 지나 경로 중간)이 돼버린다(실전 로그로 확인된 버그 —
			// Tick 1부터 Alpha=0.23이었음). 0으로 명시해 첫 블래스트가 캐릭터 위치(Alpha=0)에서 즉시
			// 터지고, 이후 틱부터는 그대로 BlastTickInterval 간격을 유지한다.
			GetWorld()->GetTimerManager().SetTimer(BlastTimerHandle, this,
				&UP1GameplayAbility_PhotonDisruptor::OnBlastTick, BlastTickInterval, true, 0.0f);

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

void UP1GameplayAbility_PhotonDisruptor::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& Data)
{
	UE_LOG(LogP1, Log, TEXT("[PhotonDisruptor] 조준 취소 — 무소모 종료"));
	SetTargetingState(false);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
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

	// 정상 발사든 조준 취소든 강제 중단(스턴 등)이든 항상 이 경로를 거치므로 태그 정리를 일괄 처리.
	// 코스트/쿨다운은 확정 시점에 CommitAbility()로 이미 함께 커밋됐으므로(조준만 하다 취소한 경우는
	// CommitAbility 자체가 안 불려서 애초에 무소모) 여기서 따로 처리할 게 없다.
	SetTargetingState(false);
	SetRootedState(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UP1GameplayAbility_PhotonDisruptor::SetTargetingState(bool bEnable)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	if (bEnable && !bTargetingActive)
	{
		ASC->AddLooseGameplayTag(TAG_State_TargetingAbility);
		bTargetingActive = true;
	}
	else if (!bEnable && bTargetingActive)
	{
		ASC->RemoveLooseGameplayTag(TAG_State_TargetingAbility);
		bTargetingActive = false;
	}
}

void UP1GameplayAbility_PhotonDisruptor::SetRootedState(bool bEnable)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	if (bEnable && !bRootedActive)
	{
		ASC->AddLooseGameplayTag(TAG_State_Rooted);
		bRootedActive = true;
	}
	else if (!bEnable && bRootedActive)
	{
		ASC->RemoveLooseGameplayTag(TAG_State_Rooted);
		bRootedActive = false;
	}
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
