// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/TargetActors/P1TargetActor_GroundDecal_Deferred.h"
#include "P1.h"
#include "Components/DecalComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameFramework/PlayerController.h"

AP1TargetActor_GroundDecal_Deferred::AP1TargetActor_GroundDecal_Deferred()
{
	PrimaryActorTick.bCanEverTick = true;

	bDestroyOnConfirmation = true;

	IndicatorDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("IndicatorDecalComponent"));
	RootComponent = IndicatorDecalComponent;
	// 데칼은 로컬 +X 방향으로 투영된다 — Pitch=-90으로 회전시켜 로컬 +X가 월드 -Z(아래)를 향하게 한다.
	IndicatorDecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	IndicatorDecalComponent->DecalSize = FVector(ProjectionDepth, AOERadius, AOERadius);
}

void AP1TargetActor_GroundDecal_Deferred::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

	OwningAbility = Ability;
	if (Ability && Ability->GetCurrentActorInfo())
	{
		SourceActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
	}
}

void AP1TargetActor_GroundDecal_Deferred::Configure(float InMaxRange, float InAOERadius)
{
	MaxRange = InMaxRange;
	AOERadius = InAOERadius;
	if (IndicatorDecalComponent)
	{
		IndicatorDecalComponent->DecalSize = FVector(ProjectionDepth, AOERadius, AOERadius);
	}
}

APlayerController* AP1TargetActor_GroundDecal_Deferred::ResolvePlayerController() const
{
	if (PrimaryPC)
	{
		return PrimaryPC;
	}
	if (OwningAbility && OwningAbility->GetCurrentActorInfo())
	{
		return Cast<APlayerController>(OwningAbility->GetCurrentActorInfo()->PlayerController.Get());
	}
	return nullptr;
}

bool AP1TargetActor_GroundDecal_Deferred::ComputeTargetLocation(FVector& OutLocation) const
{
	APlayerController* PC = ResolvePlayerController();
	if (!PC || !IsValid(SourceActor))
	{
		return false;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewLoc, ViewRot);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(SourceActor);
	Params.AddIgnoredActor(this);

	// 1) 카메라 시선으로 지면/월드를 트레이스.
	const FVector TraceEnd = ViewLoc + ViewRot.Vector() * 100000.0f;
	FHitResult ViewHit;
	FVector GroundPoint = TraceEnd;
	if (GetWorld()->LineTraceSingleByChannel(ViewHit, ViewLoc, TraceEnd, ECC_Visibility, Params))
	{
		GroundPoint = ViewHit.ImpactPoint;
	}

	// 2) 소스 기준 수평 거리로 클램프 (0 ~ MaxRange).
	const FVector SourceLoc = SourceActor->GetActorLocation();
	FVector DeltaH(GroundPoint.X - SourceLoc.X, GroundPoint.Y - SourceLoc.Y, 0.0f);
	if (DeltaH.SizeSquared() > FMath::Square(MaxRange))
	{
		DeltaH = DeltaH.GetSafeNormal() * MaxRange;
	}
	FVector Clamped = SourceLoc + DeltaH;

	// 3) 클램프된 지점에서 수직 트레이스로 실제 바닥 높이에 안착.
	FHitResult DownHit;
	const FVector DownStart = Clamped + FVector(0.0f, 0.0f, 1000.0f);
	const FVector DownEnd = Clamped - FVector(0.0f, 0.0f, 3000.0f);
	if (GetWorld()->LineTraceSingleByChannel(DownHit, DownStart, DownEnd, ECC_Visibility, Params))
	{
		Clamped.Z = DownHit.ImpactPoint.Z;
	}
	else
	{
		Clamped.Z = SourceLoc.Z;
	}

	OutLocation = Clamped;
	return true;
}

void AP1TargetActor_GroundDecal_Deferred::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector Loc;
	if (ComputeTargetLocation(Loc))
	{
		CurrentTargetLocation = Loc;
		if (IndicatorDecalComponent)
		{
			// 데칼 박스는 컴포넌트 위치에서 로컬 +X(월드 -Z, 아래) 방향으로 ProjectionDepth만큼(±) 뻗어나가는
			// 대칭 박스라, 바닥에서 ProjectionDepth의 절반 정도 위에 중심을 둬야 바닥이 박스 안쪽에 확실히 걸린다.
			IndicatorDecalComponent->SetWorldLocation(Loc + FVector(0.0f, 0.0f, ProjectionDepth * 0.5f));
		}
	}
}

void AP1TargetActor_GroundDecal_Deferred::ConfirmTargetingAndContinue()
{
	FGameplayAbilityTargetData_LocationInfo* Data = new FGameplayAbilityTargetData_LocationInfo();
	Data->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	Data->TargetLocation.LiteralTransform = FTransform(CurrentTargetLocation);

	FGameplayAbilityTargetDataHandle Handle;
	Handle.Add(Data);

	UE_LOG(LogP1, Log, TEXT("[GroundDecalDeferred] ConfirmTargetingAndContinue — 확정 위치=%s"), *CurrentTargetLocation.ToString());
	TargetDataReadyDelegate.Broadcast(Handle);
}
