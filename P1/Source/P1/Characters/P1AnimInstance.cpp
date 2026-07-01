// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/P1AnimInstance.h"
#include "Characters/P1CharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"

void UP1AnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CachedCharacter = Cast<AP1CharacterBase>(TryGetPawnOwner());
}

void UP1AnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(CachedCharacter))
	{
		CachedCharacter = Cast<AP1CharacterBase>(TryGetPawnOwner());
	}

	if (!IsValid(CachedCharacter))
	{
		return;
	}

	const UCharacterMovementComponent* MovementComponent = CachedCharacter->GetCharacterMovement();
	if (!IsValid(MovementComponent))
	{
		return;
	}

	const FVector Velocity = CachedCharacter->GetVelocity();
	const float MaxSpeed = MovementComponent->MaxWalkSpeed;

	Speed = (MaxSpeed > 0.0f) ? (Velocity.Size2D() / MaxSpeed) * 100.0f : 0.0f;
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, CachedCharacter->GetActorRotation());
	bIsFalling = MovementComponent->IsFalling();
}
