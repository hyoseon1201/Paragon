// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1AnimNotifyState_MaterialOverride.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"

void UP1AnimNotifyState_MaterialOverride::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && OverrideMaterial)
	{
		const int32 SlotIndex = MeshComp->GetMaterialIndex(MaterialSlotName);
		if (SlotIndex != INDEX_NONE)
		{
			MeshComp->SetMaterial(SlotIndex, OverrideMaterial);
		}
	}

	Received_NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
}

void UP1AnimNotifyState_MaterialOverride::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp)
	{
		const int32 SlotIndex = MeshComp->GetMaterialIndex(MaterialSlotName);
		if (SlotIndex != INDEX_NONE)
		{
			// nullptr을 넘기면 컴포넌트 오버라이드가 해제되고 스켈레탈메시 에셋의 기본 머티리얼로 되돌아간다.
			MeshComp->SetMaterial(SlotIndex, nullptr);
		}
	}

	Received_NotifyEnd(MeshComp, Animation, EventReference);
}

FString UP1AnimNotifyState_MaterialOverride::GetNotifyName_Implementation() const
{
	return OverrideMaterial ? FString::Printf(TEXT("MaterialOverride: %s"), *OverrideMaterial->GetName())
		: TEXT("MaterialOverride");
}
