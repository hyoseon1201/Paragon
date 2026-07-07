// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/GameplayCueNotifies/P1GameplayCueNotify_MaterialOverride.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"

bool UP1GameplayCueNotify_MaterialOverride::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (const ACharacter* Character = Cast<ACharacter>(MyTarget))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			const int32 SlotIndex = Mesh->GetMaterialIndex(MaterialSlotName);
			if (SlotIndex != INDEX_NONE && OverrideMaterial)
			{
				Mesh->SetMaterial(SlotIndex, OverrideMaterial);
			}
		}
	}
	return false;
}

bool UP1GameplayCueNotify_MaterialOverride::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (const ACharacter* Character = Cast<ACharacter>(MyTarget))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			const int32 SlotIndex = Mesh->GetMaterialIndex(MaterialSlotName);
			if (SlotIndex != INDEX_NONE)
			{
				// nullptr을 넘기면 오버라이드가 해제되고 스켈레탈메시 에셋의 기본 머티리얼로 되돌아간다.
				Mesh->SetMaterial(SlotIndex, nullptr);
			}
		}
	}
	return false;
}
