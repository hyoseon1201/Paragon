// Copyright Epic Games, Inc. All Rights Reserved.

#include "VFX/P1MovingParticleEffectActor.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

AP1MovingParticleEffectActor::AP1MovingParticleEffectActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	ParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleComponent"));
	RootComponent = ParticleComponent;
	ParticleComponent->bAutoDestroy = false;
	ParticleComponent->bAutoActivate = false;
}

void AP1MovingParticleEffectActor::InitializeMovingEffect(UParticleSystem* ParticleTemplate, const FVector& InStartLocation, const FVector& InEndLocation, float InDuration)
{
	StartLocation = InStartLocation;
	EndLocation = InEndLocation;
	Duration = FMath::Max(InDuration, 0.01f);
	StartTime = GetWorld()->GetTimeSeconds();

	SetActorLocation(StartLocation);
	SetActorRotation((EndLocation - StartLocation).Rotation());

	if (ParticleComponent)
	{
		ParticleComponent->SetTemplate(ParticleTemplate);
		ParticleComponent->ActivateSystem(true);
	}

	SetLifeSpan(Duration);
}

void AP1MovingParticleEffectActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Alpha = FMath::Clamp(static_cast<float>((GetWorld()->GetTimeSeconds() - StartTime) / Duration), 0.0f, 1.0f);
	SetActorLocation(FMath::Lerp(StartLocation, EndLocation, Alpha));
}
