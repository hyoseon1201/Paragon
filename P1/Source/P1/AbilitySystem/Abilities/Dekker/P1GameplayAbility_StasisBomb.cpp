// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/Dekker/P1GameplayAbility_StasisBomb.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystem/Projectiles/P1Projectile.h"
#include "Characters/P1CharacterBase.h"
#include "GameFramework/PlayerController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayEffect.h"
#include "DrawDebugHelpers.h"

UP1GameplayAbility_StasisBomb::UP1GameplayAbility_StasisBomb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_StasisBomb);
	SetAssetTags(Tags);

	InputTag = TAG_InputTag_Ability_RMB;
}

void UP1GameplayAbility_StasisBomb::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// CanActivate(코스트/쿨다운)는 TryActivate에서 이미 통과. 코스트/쿨다운은 여기서 커밋하지 않고
	// 조준 단계로 진입 — 실제 발사(릴리즈) 시점에만 커밋한다(AssaultTheGates와 동일한 이유).
	bHasFired = false;
	SetTargetingState(true);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->GenericLocalCancelCallbacks.AddDynamic(this, &UP1GameplayAbility_StasisBomb::OnAimCancelled);
	}

	if (AimMontage)
	{
		AimMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AimMontage, 1.0f);
		AimMontageTask->ReadyForActivation();
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[StasisBomb] AimMontage 미설정 — 조준 포즈 없이 진행"));
	}

	UE_LOG(LogP1, Log, TEXT("[StasisBomb] 조준 시작"));
}

void UP1GameplayAbility_StasisBomb::InputReleased(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	if (!IsActive() || bHasFired)
	{
		return;
	}
	bHasFired = true;

	StopAimMontage();
	SetTargetingState(false);

	// 확정 시 코스트+쿨다운 커밋. 감당 못하면 무취소 종료(이미 조준만 했을 뿐 아무것도 소모 안 됨).
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogP1, Log, TEXT("[StasisBomb] 발사 커밋 실패(코스트 부족 등) — 취소"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (FireMontage)
	{
		UAbilityTask_PlayMontageAndWait* FireTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, FireMontage, 1.0f);
		FireTask->ReadyForActivation();
	}

	if (ActorInfo->IsNetAuthority())
	{
		AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
		if (IsValid(SourceCharacter) && BombProjectileClass)
		{
			FVector SpawnLocation = SourceCharacter->GetActorLocation();
			if (!MuzzleSocketName.IsNone())
			{
				if (const USkeletalMeshComponent* Mesh = SourceCharacter->GetMesh())
				{
					if (Mesh->DoesSocketExist(MuzzleSocketName))
					{
						SpawnLocation = Mesh->GetSocketLocation(MuzzleSocketName);
					}
				}
			}

			const FRotator SpawnRotation = GetAimDirection().Rotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = SourceCharacter;
			SpawnParams.Instigator = SourceCharacter;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AP1Projectile* Bomb = GetWorld()->SpawnActor<AP1Projectile>(BombProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
			if (Bomb)
			{
				// bPierceThroughTargets는 기본값(false) 그대로 — 적을 맞히면 튕기지 않고 즉시 폭발.
				Bomb->MaxBounces = BombMaxBounces;
				Bomb->InitializeProjectile(ProjectileSpeed, 0.0f, ProjectileRadius);
				Bomb->OnProjectileHit.AddDynamic(this, &UP1GameplayAbility_StasisBomb::OnBombExplode);
				ActiveBomb = Bomb;

				UE_LOG(LogP1, Log, TEXT("[StasisBomb] 발사: %s (MaxBounces=%d)"), *Bomb->GetName(), BombMaxBounces);
			}
			else
			{
				UE_LOG(LogP1, Warning, TEXT("[StasisBomb] 폭탄 스폰 실패"));
			}
		}
		else if (!BombProjectileClass)
		{
			UE_LOG(LogP1, Warning, TEXT("[StasisBomb] BombProjectileClass가 설정되지 않았습니다 — GA BP에서 지정해주세요."));
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UP1GameplayAbility_StasisBomb::OnAimCancelled()
{
	if (!IsActive() || bHasFired)
	{
		return;
	}

	UE_LOG(LogP1, Log, TEXT("[StasisBomb] 조준 취소(F) — 무소모 종료"));
	StopAimMontage();
	SetTargetingState(false);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UP1GameplayAbility_StasisBomb::OnBombExplode(AActor* HitActor, const FHitResult& HitResult)
{
	if (!CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
	if (!IsValid(SourceCharacter))
	{
		return;
	}

	const FVector ExplosionPoint = FVector(HitResult.ImpactPoint);
	const float DistanceTraveled = ActiveBomb.IsValid() ? ActiveBomb->GetDistanceTraveled() : 0.0f;
	const float Alpha = MaxStunDistance > 0.0f ? FMath::Clamp(DistanceTraveled / MaxStunDistance, 0.0f, 1.0f) : 1.0f;
	const float StunDuration = FMath::Lerp(MinStunDuration, MaxStunDuration.GetValueAtLevel(GetAbilityLevel()), Alpha);

	UE_LOG(LogP1, Log, TEXT("[StasisBomb] 폭발 @ %s | 이동거리=%.0f Alpha=%.2f StunDuration=%.2f"),
		*ExplosionPoint.ToString(), DistanceTraveled, Alpha, StunDuration);

	const TArray<AActor*> Enemies = GetEnemiesInRadius(ExplosionPoint, ExplosionRadius, ExplosionHalfHeight);
	for (AActor* Enemy : Enemies)
	{
		AP1CharacterBase* TargetCharacter = Cast<AP1CharacterBase>(Enemy);
		if (!IsValid(TargetCharacter))
		{
			continue;
		}

		ApplyDamageToTarget(TargetCharacter, 1.0f);

		if (StunDebuffEffectClass)
		{
			if (IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetCharacter))
			{
				if (UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent())
				{
					if (UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo())
					{
						FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
						Ctx.AddSourceObject(SourceCharacter);
						const FGameplayEffectSpecHandle StunSpec = SourceASC->MakeOutgoingSpec(
							StunDebuffEffectClass, GetAbilityLevel(), Ctx);
						if (StunSpec.IsValid())
						{
							StunSpec.Data->SetSetByCallerMagnitude(TAG_Data_StunDuration, StunDuration);
							SourceASC->ApplyGameplayEffectSpecToTarget(*StunSpec.Data.Get(), TargetASC);
						}
					}
				}
			}
		}
	}

	if (ExplosionEffect)
	{
		SourceCharacter->MulticastPlayParticleEffectAtLocation(ExplosionEffect, ExplosionPoint, FRotator::ZeroRotator, FVector(1.0f));
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		DrawDebugSphere(GetWorld(), ExplosionPoint, ExplosionRadius, 24, FColor::Cyan, false, 2.0f, 0, 2.0f);
	}
#endif
}

void UP1GameplayAbility_StasisBomb::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 정상 발사든 조준 취소든 강제 중단(스턴 등)이든, 이 경로를 항상 거치므로 몽타주/태그 정리를 일괄 처리.
	StopAimMontage();
	SetTargetingState(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UP1GameplayAbility_StasisBomb::SetTargetingState(bool bEnable)
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

void UP1GameplayAbility_StasisBomb::StopAimMontage()
{
	if (AimMontageTask)
	{
		AimMontageTask->EndTask();
		AimMontageTask = nullptr;
	}

	// EndTask만으로는 루프 몽타주(에셋 자체 Loop 설정)가 확실히 멎는다는 보장이 없어 명시적으로도 정지시킨다.
	if (AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo())
	{
		if (USkeletalMeshComponent* Mesh = SourceCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				if (AimMontage && AnimInstance->Montage_IsPlaying(AimMontage))
				{
					AnimInstance->Montage_Stop(0.25f, AimMontage);
				}
			}
		}
	}
}

FVector UP1GameplayAbility_StasisBomb::GetAimDirection() const
{
	const AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();
	if (!IsValid(SourceCharacter))
	{
		return FVector::ForwardVector;
	}

	if (const APlayerController* PC = Cast<APlayerController>(SourceCharacter->GetController()))
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
		return ViewRotation.Vector();
	}

	return SourceCharacter->GetActorForwardVector();
}
