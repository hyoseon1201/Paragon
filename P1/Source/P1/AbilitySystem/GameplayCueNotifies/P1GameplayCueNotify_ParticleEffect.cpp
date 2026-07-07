// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/GameplayCueNotifies/P1GameplayCueNotify_ParticleEffect.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"

bool UP1GameplayCueNotify_ParticleEffect::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!IsValid(MyTarget) || !ParticleTemplate)
	{
		return false;
	}

	if (const ACharacter* Character = Cast<ACharacter>(MyTarget))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (!SocketName.IsNone() && Mesh->DoesSocketExist(SocketName))
			{
				UGameplayStatics::SpawnEmitterAttached(ParticleTemplate, Mesh, SocketName,
					FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
				return false;
			}
		}
	}

	UGameplayStatics::SpawnEmitterAtLocation(MyTarget->GetWorld(), ParticleTemplate, MyTarget->GetActorLocation());
	return false;
}
