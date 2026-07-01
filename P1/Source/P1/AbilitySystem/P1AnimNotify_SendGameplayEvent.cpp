// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1AnimNotify_SendGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"

void UP1AnimNotify_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	FGameplayEventData Payload;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, Payload);
}

FString UP1AnimNotify_SendGameplayEvent::GetNotifyName_Implementation() const
{
	return EventTag.IsValid() ? EventTag.ToString() : TEXT("SendGameplayEvent");
}
