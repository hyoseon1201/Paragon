// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Hazards/AP1ContainmentFence.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Characters/P1CharacterBase.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "P1.h"

AP1ContainmentFence::AP1ContainmentFence()
{
	PrimaryActorTick.bCanEverTick = true;

	OverlapComponent = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapComponent"));
	OverlapComponent->InitSphereRadius(FenceRadius);
	OverlapComponent->SetCollisionObjectType(ECC_WorldDynamic);
	OverlapComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	OverlapComponent->OnComponentBeginOverlap.AddDynamic(this, &AP1ContainmentFence::OnOverlapBegin);
	SetRootComponent(OverlapComponent);

	FenceEffectComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FenceEffectComponent"));
	FenceEffectComponent->SetupAttachment(OverlapComponent);
	FenceEffectComponent->bAutoActivate = true;

	bReplicates = true;
}

void AP1ContainmentFence::InitializeFence(float InRadius, float InDuration, AP1CharacterBase* InInstigator)
{
	FenceRadius = InRadius;
	FenceCenter = GetActorLocation();
	FenceInstigator = InInstigator;

	if (OverlapComponent)
	{
		OverlapComponent->SetSphereRadius(InRadius);
	}

	if (FenceEffectComponent)
	{
		if (FenceEffectTemplate)
		{
			FenceEffectComponent->SetTemplate(FenceEffectTemplate);
		}
		const float ScaleRatio = BaseEffectRadius > 0.0f ? (InRadius / BaseEffectRadius) : 1.0f;
		FenceEffectComponent->SetWorldScale3D(FVector(ScaleRatio));
	}

	SetLifeSpan(InDuration);

	if (!HasAuthority())
	{
		return;
	}

	// 배치 시점에 이미 원 안에 있던 적 히어로를 스캔 — 이들은 "안에서 시작"이므로 못 나가게 한다.
	FCollisionQueryParams Params;
	TArray<FOverlapResult> Results;
	GetWorld()->OverlapMultiByChannel(Results, FenceCenter, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(InRadius), Params);

	for (const FOverlapResult& Result : Results)
	{
		AActor* OverlappedActor = Result.GetActor();
		if (IsBlockableEnemy(OverlappedActor))
		{
			TrackedEnemies.Add(OverlappedActor, true);
		}
	}

	UE_LOG(LogP1, Log, TEXT("[ContainmentFence] 배치 완료 @ %s | Radius=%.0f Duration=%.2f | 초기 감금 대상=%d"),
		*FenceCenter.ToString(), InRadius, InDuration, TrackedEnemies.Num());
}

bool AP1ContainmentFence::IsBlockableEnemy(const AActor* TargetActor) const
{
	const AP1CharacterBase* Character = Cast<AP1CharacterBase>(TargetActor);
	if (!IsValid(Character))
	{
		return false;
	}

	// 튜닝된 대상은 "적 히어로"만(원 문구 그대로) — 몬스터/보스는 대상이 아니다.
	if (Character->GetCharacterType() != TAG_Character_Type_Hero)
	{
		return false;
	}

	if (!FenceInstigator.IsValid() || AP1CharacterBase::IsSameTeam(FenceInstigator.Get(), Character))
	{
		return false;
	}

	return true;
}

void AP1ContainmentFence::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || TrackedEnemies.Contains(OtherActor) || !IsBlockableEnemy(OtherActor))
	{
		return;
	}

	// 배치 이후 밖에서 접근해온 대상 — "밖에서 시작"이므로 못 들어오게 한다.
	TrackedEnemies.Add(OtherActor, false);
	UE_LOG(LogP1, Log, TEXT("[ContainmentFence] 신규 접근 차단 대상 등록: %s"), *OtherActor->GetName());
}

void AP1ContainmentFence::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	for (auto It = TrackedEnemies.CreateIterator(); It; ++It)
	{
		AActor* Enemy = It->Key.Get();
		if (!IsValid(Enemy))
		{
			It.RemoveCurrent();
			continue;
		}

		const bool bStartedInside = It->Value;
		const FVector EnemyLocation = Enemy->GetActorLocation();
		const FVector2D Offset2D(EnemyLocation.X - FenceCenter.X, EnemyLocation.Y - FenceCenter.Y);
		const float Distance = Offset2D.Size();
		if (Distance < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const bool bCurrentlyInside = Distance <= FenceRadius;
		if (bCurrentlyInside == bStartedInside)
		{
			continue;
		}

		// 시작한 쪽 경계 바로 안쪽으로 clamp — 안에서 시작했으면 반경보다 살짝 작게, 밖에서
		// 시작했으면 반경보다 살짝 크게 되돌려 경계를 실제로 못 넘어가게 한다.
		const float ClampedDistance = bStartedInside ? (FenceRadius - 5.0f) : (FenceRadius + 5.0f);
		const FVector2D ClampedOffset = Offset2D.GetSafeNormal() * ClampedDistance;
		const FVector NewLocation(FenceCenter.X + ClampedOffset.X, FenceCenter.Y + ClampedOffset.Y, EnemyLocation.Z);
		Enemy->SetActorLocation(NewLocation, true);
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		DrawDebugCircle(GetWorld(), FenceCenter, FenceRadius, 32, FColor::Red, false, -1.0f, 0, 2.0f,
			FVector(1, 0, 0), FVector(0, 1, 0), false);
	}
#endif
}
