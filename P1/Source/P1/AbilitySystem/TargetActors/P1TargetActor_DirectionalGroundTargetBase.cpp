// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/TargetActors/P1TargetActor_DirectionalGroundTargetBase.h"
#include "P1.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"

void AP1TargetActor_DirectionalGroundTargetBase::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

	OwningAbility = Ability;
	if (Ability && Ability->GetCurrentActorInfo())
	{
		SourceActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
	}
}

void AP1TargetActor_DirectionalGroundTargetBase::Configure(float InLength, float InWidth)
{
	Length = InLength;
	Width = InWidth;
}

APlayerController* AP1TargetActor_DirectionalGroundTargetBase::ResolvePlayerController() const
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

FVector AP1TargetActor_DirectionalGroundTargetBase::SnapToGround(const FVector& XYSource) const
{
	FCollisionQueryParams Params;
	if (IsValid(SourceActor))
	{
		Params.AddIgnoredActor(SourceActor);
	}
	Params.AddIgnoredActor(this);

	const FVector TraceStart = XYSource + FVector(0.0f, 0.0f, 1000.0f);
	const FVector TraceEnd = XYSource - FVector(0.0f, 0.0f, 3000.0f);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		return Hit.ImpactPoint;
	}

	return XYSource;
}

bool AP1TargetActor_DirectionalGroundTargetBase::ComputeSourceAndDirection(FVector& OutGroundLocation, FVector& OutDirection) const
{
	APlayerController* PC = ResolvePlayerController();
	if (!PC || !IsValid(SourceActor))
	{
		return false;
	}

	OutGroundLocation = SnapToGround(SourceActor->GetActorLocation());

	FVector ViewLoc;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewLoc, ViewRot);

	// 지표면 위 직사각형이라 수직 성분은 버리고 수평 방향만 사용한다(PhotonDisruptor의
	// GetAimDirectionXY와 동일한 계산 — 조준 표시와 실제 스윕 방향이 항상 일치해야 함).
	OutDirection = FVector(ViewRot.Vector().X, ViewRot.Vector().Y, 0.0f).GetSafeNormal();
	return true;
}

void AP1TargetActor_DirectionalGroundTargetBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector GroundLoc, Dir;
	if (!ComputeSourceAndDirection(GroundLoc, Dir))
	{
		return;
	}

	CurrentSourceGroundLocation = GroundLoc;
	CurrentDirection = Dir;
	CurrentFarEndpoint = SnapToGround(GroundLoc + Dir * Length);

	UpdateIndicatorTransform();

#if ENABLE_DRAW_DEBUG
	DrawDebugLine(GetWorld(), GroundLoc + FVector(0, 0, 3.0f), CurrentFarEndpoint + FVector(0, 0, 3.0f),
		FColor::Cyan, false, -1.0f, 0, 4.0f);
#endif
}

void AP1TargetActor_DirectionalGroundTargetBase::ConfirmTargetingAndContinue()
{
	// 캐릭터 위치가 아니라 "먼 쪽 끝점"을 반환 — 어빌리티는 Source(자기 위치)→이 지점을 방향으로
	// 재해석한다(AP1TargetActor_GroundDecal이 확정 지점 자체를 반환하는 것과 대비되는 지점).
	FGameplayAbilityTargetData_LocationInfo* Data = new FGameplayAbilityTargetData_LocationInfo();
	Data->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	Data->TargetLocation.LiteralTransform = FTransform(CurrentFarEndpoint);

	FGameplayAbilityTargetDataHandle Handle;
	Handle.Add(Data);

	UE_LOG(LogP1, Log, TEXT("[DirectionalGroundTarget] ConfirmTargetingAndContinue — 끝점=%s"), *CurrentFarEndpoint.ToString());
	TargetDataReadyDelegate.Broadcast(Handle);
}
