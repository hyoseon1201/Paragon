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

	UE_LOG(LogP1, Log, TEXT("[StasisBomb] InputReleased 진입 — IsActive=%d bHasFired=%d IsNetAuthority=%d"),
		IsActive() ? 1 : 0, bHasFired ? 1 : 0, ActorInfo->IsNetAuthority() ? 1 : 0);

	if (!IsActive() || bHasFired)
	{
		UE_LOG(LogP1, Log, TEXT("[StasisBomb] InputReleased 무시(이미 비활성이거나 이미 발사됨)"));
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
				Bomb->InitializeProjectile(ProjectileSpeed, 0.0f, ProjectileRadius, ProjectileGravityScale, bShowDebug);
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

	const TArray<AActor*> Enemies = GetEnemiesInRadius(ExplosionPoint, ExplosionRadius, ExplosionHalfHeight);

	// Warning 레벨로 — OnBombExplode는 서버(IsNetAuthority) 코드 경로에서만 도는데, PIE에서 서버 창을
	// 안 보고 있으면 이 로그 자체가 "폭발이 실제로 일어났는지" 확인할 수 있는 유일한 신호일 수 있다.
	UE_LOG(LogP1, Warning, TEXT("[StasisBomb] 폭발 @ %s | 이동거리=%.0f Alpha=%.2f StunDuration=%.2f 적중대상=%d"),
		*ExplosionPoint.ToString(), DistanceTraveled, Alpha, StunDuration, Enemies.Num());
	for (AActor* Enemy : Enemies)
	{
		AP1CharacterBase* TargetCharacter = Cast<AP1CharacterBase>(Enemy);
		if (!IsValid(TargetCharacter))
		{
			continue;
		}

		ApplyDamageToTarget(TargetCharacter, 1.0f);

		if (!StunDebuffEffectClass)
		{
			UE_LOG(LogP1, Warning, TEXT("[StasisBomb] StunDebuffEffectClass가 설정되지 않았습니다 — GA BP에서 지정해주세요. 스턴 미적용."));
		}
		else if (IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetCharacter))
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
						const FActiveGameplayEffectHandle ActiveHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*StunSpec.Data.Get(), TargetASC);
						// Duration형 GE라 (Damage와 달리) 성공 시 ActiveHandle이 유효해야 정상 — Invalid면 GE
						// 자체가(예: Instant로 잘못 설정, Application 조건 미충족 등) 실제로는 안 걸린 것.
						UE_LOG(LogP1, Log, TEXT("[StasisBomb] 스턴 GE 적용 — Target=%s Duration=%.2f ActiveHandle valid=%d TargetHasStunnedTag=%d"),
							*TargetCharacter->GetName(), StunDuration, ActiveHandle.IsValid() ? 1 : 0,
							TargetASC->HasMatchingGameplayTag(TAG_State_Stunned) ? 1 : 0);
					}
					else
					{
						UE_LOG(LogP1, Warning, TEXT("[StasisBomb] 스턴 스펙 생성 실패 — MakeOutgoingSpec 반환값이 Invalid"));
					}
				}
			}
			else
			{
				UE_LOG(LogP1, Warning, TEXT("[StasisBomb] 대상의 TargetASC를 찾을 수 없음 — %s"), *TargetCharacter->GetName());
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
		// 서버 전용 경로라 raw DrawDebugSphere는 서버 프로세스 화면에서만 보인다(리플리케이트 안 됨) —
		// 실제로 확인하려는 클라이언트 화면에도 뜨도록 Multicast를 거친다.
		SourceCharacter->MulticastDrawDebugSphere(ExplosionPoint, ExplosionRadius, FColor::Cyan, 2.0f);
	}
#endif
}

void UP1GameplayAbility_StasisBomb::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 정상 발사든 조준 취소든 강제 중단(스턴/네트워크 활성화 거부 등)이든, 이 경로를 항상 거치므로
	// 몽타주/태그 정리를 일괄 처리한다. "홀드 중인데 갑자기 끝남" 재현 시 bWasCancelled=1인데 직전에
	// InputReleased/OnAimCancelled 로그가 하나도 안 찍혔다면 GAS 바깥(예: 서버가 클라 예측 활성화를
	// 거부해 강제로 걸어온 EndAbility)에서 걸려온 것 — 그 경우가 진짜 원인일 가능성이 높다.
	UE_LOG(LogP1, Log, TEXT("[StasisBomb] EndAbility — bWasCancelled=%d bReplicateEndAbility=%d IsNetAuthority=%d"),
		bWasCancelled ? 1 : 0, bReplicateEndAbility ? 1 : 0, ActorInfo->IsNetAuthority() ? 1 : 0);

	StopAimMontage();
	SetTargetingState(false);

	// ActivateAbility에서 매번 AddDynamic으로 구독한 걸 여기서 짝 맞춰 해제한다. InstancedPerActor라
	// 같은 인스턴스가 재활성화마다 재사용되는데, 구독 해제 없이 재활성화하면 델리게이트에 같은
	// (오브젝트, 함수) 조합이 중복으로 쌓여 다음 AddDynamic 시점에 "same function isn't already bound"
	// ensure가 터진다(정확히 두 번째 시전부터 재현된 크래시의 원인 — 서버/클라 각자의 ASC 인스턴스에서
	// 독립적으로 누적).
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->GenericLocalCancelCallbacks.RemoveDynamic(this, &UP1GameplayAbility_StasisBomb::OnAimCancelled);
	}

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
