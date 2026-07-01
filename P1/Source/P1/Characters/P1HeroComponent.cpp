// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/P1HeroComponent.h"
#include "Camera/P1CameraComponent.h"
#include "Camera/P1CameraMode.h"
#include "P1.h"

UP1HeroComponent::UP1HeroComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
}

// static
UP1HeroComponent* UP1HeroComponent::FindHeroComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UP1HeroComponent>() : nullptr;
}

void UP1HeroComponent::BeginPlay()
{
	Super::BeginPlay();

	const APawn* Pawn = GetPawn<APawn>();
	if (!IsValid(Pawn)) return;

	if (UP1CameraComponent* CameraComponent = UP1CameraComponent::FindCameraComponent(Pawn))
	{
		CameraComponent->DetermineCameraModeDelegate.BindUObject(this, &UP1HeroComponent::DetermineCameraMode);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("UP1HeroComponent::BeginPlay: No UP1CameraComponent found on %s"), *GetNameSafe(Pawn));
	}
}

void UP1HeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (UP1CameraComponent* CameraComponent = UP1CameraComponent::FindCameraComponent(Pawn))
		{
			CameraComponent->DetermineCameraModeDelegate.Unbind();
		}
	}

	Super::EndPlay(EndPlayReason);
}

TSubclassOf<UP1CameraMode> UP1HeroComponent::DetermineCameraMode() const
{
	return DefaultCameraMode;
}
