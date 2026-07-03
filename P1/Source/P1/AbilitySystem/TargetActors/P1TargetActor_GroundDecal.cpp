// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/TargetActors/P1TargetActor_GroundDecal.h"
#include "P1.h"
#include "Components/StaticMeshComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"

AP1TargetActor_GroundDecal::AP1TargetActor_GroundDecal()
{
	PrimaryActorTick.bCanEverTick = true;

	// 확정 후 자동 소멸. 타겟 데이터는 클라에서 생성해 서버로 복제(bShouldProduceTargetDataOnServer=false 기본값).
	bDestroyOnConfirmation = true;

	IndicatorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IndicatorMeshComponent"));
	RootComponent = IndicatorMeshComponent;
	IndicatorMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IndicatorMeshComponent->SetCastShadow(false);
	IndicatorMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
	const float InitialScale = (AOERadius * 2.0f) / BaseMeshSize;
	IndicatorMeshComponent->SetRelativeScale3D(FVector(InitialScale, InitialScale, 1.0f));
}

void AP1TargetActor_GroundDecal::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

	OwningAbility = Ability;
	if (Ability && Ability->GetCurrentActorInfo())
	{
		SourceActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
	}

	UE_LOG(LogP1, Warning, TEXT("[GroundDecal] StartTargeting — SourceActor=%s PrimaryPC=%s TickEnabled=%d NetMode=%d"),
		SourceActor ? *SourceActor->GetName() : TEXT("NULL"),
		PrimaryPC ? *PrimaryPC->GetName() : TEXT("NULL"),
		IsActorTickEnabled() ? 1 : 0, (int32)GetNetMode());
}

void AP1TargetActor_GroundDecal::Destroyed()
{
	UE_LOG(LogP1, Warning, TEXT("[GroundDecal] Destroyed — 인디케이터 소멸"));
	Super::Destroyed();
}

void AP1TargetActor_GroundDecal::Configure(float InMaxRange, float InAOERadius)
{
	MaxRange = InMaxRange;
	AOERadius = InAOERadius;
	if (IndicatorMeshComponent)
	{
		const float Scale = (AOERadius * 2.0f) / BaseMeshSize;
		IndicatorMeshComponent->SetRelativeScale3D(FVector(Scale, Scale, 1.0f));
	}
}

APlayerController* AP1TargetActor_GroundDecal::ResolvePlayerController() const
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

bool AP1TargetActor_GroundDecal::ComputeTargetLocation(FVector& OutLocation) const
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

void AP1TargetActor_GroundDecal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 조준 위치 계산은 로컬(소유) 컨트롤러에서만 의미가 있다.
	FVector Loc;
	const bool bOk = ComputeTargetLocation(Loc);
	if (bOk)
	{
		CurrentTargetLocation = Loc;
		if (IndicatorMeshComponent)
		{
			IndicatorMeshComponent->SetWorldLocation(Loc + FVector(0.0f, 0.0f, GroundOffset));
		}

#if ENABLE_DRAW_DEBUG
		// 메시가 안 보여도 조준 위치를 확인할 수 있게 디버그 원을 그린다 (문제 없으면 나중에 제거해도 무방).
		DrawDebugCircle(GetWorld(), Loc + FVector(0.0f, 0.0f, 3.0f), AOERadius, 48,
			FColor::Green, false, -1.0f, 0, 4.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
#endif
	}

	// 계산 성공/실패 전환 시 즉시 로그, 그 외엔 1초마다 스로틀 로그.
	DebugLogAccum += DeltaSeconds;
	if (bOk != bLastComputeOk || DebugLogAccum >= 1.0f)
	{
		UE_LOG(LogP1, Warning, TEXT("[GroundDecal] Tick — ComputeOk=%d Loc=%s PC=%s Source=%s"),
			bOk ? 1 : 0, *Loc.ToString(),
			ResolvePlayerController() ? TEXT("valid") : TEXT("NULL"),
			IsValid(SourceActor) ? TEXT("valid") : TEXT("NULL"));
		DebugLogAccum = 0.0f;
		bLastComputeOk = bOk;
	}
}

void AP1TargetActor_GroundDecal::ConfirmTargetingAndContinue()
{
	// 확정된 지면 위치를 LocationInfo 타겟 데이터로 반환. 어빌리티는 GetEndPoint()로 읽는다.
	FGameplayAbilityTargetData_LocationInfo* Data = new FGameplayAbilityTargetData_LocationInfo();
	Data->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	Data->TargetLocation.LiteralTransform = FTransform(CurrentTargetLocation);

	FGameplayAbilityTargetDataHandle Handle;
	Handle.Add(Data);

	UE_LOG(LogP1, Warning, TEXT("[GroundDecal] ConfirmTargetingAndContinue — 확정 위치=%s"), *CurrentTargetLocation.ToString());
	TargetDataReadyDelegate.Broadcast(Handle);
}
