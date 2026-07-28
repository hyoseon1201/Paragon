// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/TargetActors/P1TargetActor_GroundDecal_Rectangle_Deferred.h"
#include "P1.h"
#include "Components/DecalComponent.h"

AP1TargetActor_GroundDecal_Rectangle_Deferred::AP1TargetActor_GroundDecal_Rectangle_Deferred()
{
	PrimaryActorTick.bCanEverTick = true;

	bDestroyOnConfirmation = true;

	IndicatorDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("IndicatorDecalComponent"));
	RootComponent = IndicatorDecalComponent;
	// 초기값(Length/Width는 베이스 기본값) — 실제 값은 Configure()가 다시 계산해 덮어쓴다.
	IndicatorDecalComponent->DecalSize = FVector(ProjectionDepth, Width * 0.5f, Length * 0.5f);
}

void AP1TargetActor_GroundDecal_Rectangle_Deferred::Configure(float InLength, float InWidth)
{
	Super::Configure(InLength, InWidth);

	if (IndicatorDecalComponent)
	{
		// Y=폭(반값), Z=길이(반값) — UpdateIndicatorTransform()의 회전 구성 방식(MakeFromXZ)과 짝을 맞춰야 한다.
		IndicatorDecalComponent->DecalSize = FVector(ProjectionDepth, Width * 0.5f, Length * 0.5f);
	}
}

void AP1TargetActor_GroundDecal_Rectangle_Deferred::UpdateIndicatorTransform()
{
	if (!IndicatorDecalComponent)
	{
		return;
	}

	// 데칼 박스 중심을 캐릭터~끝점의 중점에 둔다(±Length/2가 DecalSize.Z와 맞아떨어져 정확히
	// 캐릭터 위치~끝점 사이를 덮는다). ProjectionDepth의 절반만큼 띄워서 바닥이 박스 안쪽에 확실히 걸리게 한다.
	const FVector Midpoint = FMath::Lerp(CurrentSourceGroundLocation, CurrentFarEndpoint, 0.5f);
	IndicatorDecalComponent->SetWorldLocation(Midpoint + FVector(0.0f, 0.0f, ProjectionDepth * 0.5f));

	// 로컬 X(투영축)는 항상 월드 -Z(아래), 로컬 Z(DecalSize.Z=길이축)는 조준 방향을 향해야 한다 —
	// 단순 Pitch/Yaw 조합으로는 "투영은 항상 수직 아래이면서 그 축을 중심으로 한 회전만 조준 방향을
	// 따라가야 함"을 표현하기 까다로워서, 두 축을 직접 지정해 회전을 구성한다(MakeFromXZ가 첫 번째
	// 축은 그대로 유지하고 두 번째 축을 그 축에 직교하도록 보정해준다).
	const FRotator Rot = FRotationMatrix::MakeFromXZ(FVector(0.0f, 0.0f, -1.0f), CurrentDirection).Rotator();
	IndicatorDecalComponent->SetWorldRotation(Rot);
}
