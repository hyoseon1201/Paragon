// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Dekker/P1GameplayAbility_ContainmentFence.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystem/TargetActors/P1TargetActor_GroundDecal_Deferred.h"
#include "AbilitySystem/Hazards/AP1ContainmentFence.h"
#include "Characters/P1CharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UP1GameplayAbility_ContainmentFence::UP1GameplayAbility_ContainmentFence()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_ContainmentFence);
	SetAssetTags(Tags);

	InputTag = TAG_InputTag_Ability_E;
}

void UP1GameplayAbility_ContainmentFence::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// CanActivate(코스트/쿨다운)는 TryActivate에서 이미 통과. 조준 확정 전까지는 커밋하지 않는다.
	if (!TargetActorClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[ContainmentFence] TargetActorClass 미설정 — GA BP에서 지정하세요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BeginTargeting();
}

void UP1GameplayAbility_ContainmentFence::BeginTargeting()
{
	SetTargetingState(true);

	UAbilityTask_WaitTargetData* Task = UAbilityTask_WaitTargetData::WaitTargetData(
		this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	if (!Task)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	Task->ValidData.AddDynamic(this, &UP1GameplayAbility_ContainmentFence::OnTargetDataReady);
	Task->Cancelled.AddDynamic(this, &UP1GameplayAbility_ContainmentFence::OnTargetCancelled);

	AGameplayAbilityTargetActor* SpawnedActor = nullptr;
	const bool bSpawned = Task->BeginSpawningActor(this, TargetActorClass, SpawnedActor);
	if (bSpawned)
	{
		if (AP1TargetActor_GroundDecal_Deferred* DeferredDecal = Cast<AP1TargetActor_GroundDecal_Deferred>(SpawnedActor))
		{
			DeferredDecal->Configure(MaxRange, FenceRadius);
		}
		Task->FinishSpawningActor(this, SpawnedActor);
	}

	Task->ReadyForActivation();
}

void UP1GameplayAbility_ContainmentFence::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
	SetTargetingState(false);

	const FVector ConfirmedLocation = UAbilitySystemBlueprintLibrary::GetTargetDataEndPoint(Data, 0);

	// 확정 시 코스트+쿨다운을 함께 커밋. 감당 못하면 무취소 종료(조준만 했을 뿐 아무것도 소모 안 됨).
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		UE_LOG(LogP1, Log, TEXT("[ContainmentFence] 커밋 실패(코스트/쿨다운) — 취소"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (CastMontage)
	{
		UAbilityTask_PlayMontageAndWait* CastTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, CastMontage, 1.0f);
		CastTask->ReadyForActivation();
	}

	if (CurrentActorInfo->IsNetAuthority())
	{
		AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
		if (IsValid(SourceCharacter) && FenceActorClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = SourceCharacter;
			SpawnParams.Instigator = SourceCharacter;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AP1ContainmentFence* Fence = GetWorld()->SpawnActor<AP1ContainmentFence>(
				FenceActorClass, ConfirmedLocation, FRotator::ZeroRotator, SpawnParams);
			if (Fence)
			{
				Fence->InitializeFence(FenceRadius, FenceDuration.GetValueAtLevel(GetAbilityLevel()), SourceCharacter);
				UE_LOG(LogP1, Log, TEXT("[ContainmentFence] 배치: %s @ %s"), *Fence->GetName(), *ConfirmedLocation.ToString());
			}
			else
			{
				UE_LOG(LogP1, Warning, TEXT("[ContainmentFence] 배치 실패"));
			}
		}
		else if (!FenceActorClass)
		{
			UE_LOG(LogP1, Warning, TEXT("[ContainmentFence] FenceActorClass가 설정되지 않았습니다 — GA BP에서 지정해주세요."));
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UP1GameplayAbility_ContainmentFence::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& Data)
{
	UE_LOG(LogP1, Log, TEXT("[ContainmentFence] 조준 취소 — 무소모 종료"));
	SetTargetingState(false);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UP1GameplayAbility_ContainmentFence::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 정상 배치든 조준 취소든 강제 중단(스턴 등)이든 항상 이 경로를 거치므로 태그 정리를 일괄 처리.
	SetTargetingState(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UP1GameplayAbility_ContainmentFence::SetTargetingState(bool bEnable)
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
