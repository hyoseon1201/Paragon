// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "P1HeroComponent.generated.h"

class UP1CameraMode;

/**
 * Pawn component responsible for wiring camera mode selection on player-controlled heroes.
 * Attach to AP1HeroCharacter. Does NOT handle input (that stays in AP1PlayerController).
 */
UCLASS(Blueprintable, Meta = (BlueprintSpawnableComponent))
class P1_API UP1HeroComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UP1HeroComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	static UP1HeroComponent* FindHeroComponent(const AActor* Actor);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	TSubclassOf<UP1CameraMode> DetermineCameraMode() const;

	// Set this to a BP subclass of UP1CameraMode in the hero's Blueprint defaults
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UP1CameraMode> DefaultCameraMode;
};
