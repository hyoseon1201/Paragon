// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Projectiles/P1Projectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "P1.h"

AP1Projectile::AP1Projectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(15.0f);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Pawn은 Block이 아니라 Overlap — 관통 투사체가 첫 대상과 물리적으로 충돌해 멈추거나 튕기면 안 되기
	// 때문에 애초에 막히지 않도록 한다(단발 투사체는 어차피 OnPawnOverlap에서 바로 Destroy()하므로 동작 동일).
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	// 팀 판별(아군 무시)은 여기서 안 한다 — OnProjectileHit을 받는 어빌리티 쪽에서 IsSameTeam()으로 판단.
	CollisionComponent->OnComponentHit.AddDynamic(this, &AP1Projectile::OnHit);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AP1Projectile::OnPawnOverlap);
	SetRootComponent(CollisionComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	// 논타겟 스킬샷 기본값 = 직선 비행. 포물선/튕기는 투사체(Stasis Bomb 등)가 필요하면 그 어빌리티가
	// 스폰 직후 ProjectileMovement 프로퍼티를 직접 조정(bShouldBounce=true 등)하면 된다.
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	bReplicates = true;
	SetReplicateMovement(true);

	// 안전장치 — MaxRange를 설정 안 하거나 아무것도 안 맞고 허공으로 날아가는 경우에도 영원히 안 남게.
	InitialLifeSpan = 5.0f;
}

void AP1Projectile::BeginPlay()
{
	Super::BeginPlay();
	StartLocation = GetActorLocation();

	// 총구 소켓이 시전자 캡슐과 겹치는 위치일 수 있어, 물리적 블록 충돌(OnComponentHit 이전 단계)로
	// 스폰 직후 자기 자신에게 막혀버리는 것을 방지 — OnHit의 OtherActor==GetOwner() 체크만으로는
	// "판정 무시"는 되지만 "물리적으로 부딪혀 멈추는 것"까지는 막지 못한다.
	if (AActor* MyOwner = GetOwner())
	{
		CollisionComponent->IgnoreActorWhenMoving(MyOwner, true);
	}

	// 튕기는 투사체(Stasis Bomb 등)는 MaxBounces>0로 설정하는 것만으로 물리 반사까지 자동으로 켜지게 한다 —
	// BP에서 bShouldBounce를 따로 켜는 걸 잊어 "MaxBounces는 설정했는데 실제로는 안 튕기는" 실수를 방지.
	if (MaxBounces > 0 && ProjectileMovement)
	{
		ProjectileMovement->bShouldBounce = true;
	}
}

void AP1Projectile::InitializeProjectile(float Speed, float InMaxRange, float Radius)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;
		// 이미 BeginPlay가 지난 시점(스폰 직후 호출)이면 새 속도로 다시 초기화해줘야 실제로 반영된다.
		ProjectileMovement->Velocity = GetActorForwardVector() * Speed;
	}
	if (CollisionComponent)
	{
		CollisionComponent->SetSphereRadius(Radius);
	}
	MaxRange = InMaxRange;
	StartLocation = GetActorLocation();
}

float AP1Projectile::GetDistanceTraveled() const
{
	return FVector::Dist(StartLocation, GetActorLocation());
}

void AP1Projectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 사거리 초과 시 소멸 — 서버에서만 판단(권위), 클라는 Destroy 리플리케이션으로 자연히 따라온다.
	if (HasAuthority() && MaxRange > 0.0f && GetDistanceTraveled() >= MaxRange)
	{
		UE_LOG(LogP1, Log, TEXT("[Projectile] 사거리 초과로 소멸 (%s, 이동거리=%.0f)"), *GetName(), GetDistanceTraveled());
		Destroy();
	}
}

void AP1Projectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!OtherActor || OtherActor == this || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	// 아직 튕길 여유가 남아있으면 소멸하지 않고 그대로 둔다 — 실제 반사(속도 반전)는
	// ProjectileMovement->bShouldBounce가 이미 처리했으므로 여기서는 카운트만 올리고 끝.
	if (CurrentBounceCount < MaxBounces)
	{
		++CurrentBounceCount;
		UE_LOG(LogP1, Log, TEXT("[Projectile] 튕김 %d/%d — %s (이동거리=%.0f)"),
			CurrentBounceCount, MaxBounces, *OtherActor->GetName(), GetDistanceTraveled());
		return;
	}

	UE_LOG(LogP1, Log, TEXT("[Projectile] OnHit — %s (이동거리=%.0f)"), *OtherActor->GetName(), GetDistanceTraveled());

	MulticastPlayHitEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	OnProjectileHit.Broadcast(OtherActor, Hit);
	Destroy();
}

void AP1Projectile::OnPawnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!OtherActor || OtherActor == this || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	if (HitActors.Contains(OtherActor))
	{
		return;
	}
	HitActors.Add(OtherActor);

	UE_LOG(LogP1, Log, TEXT("[Projectile] OnPawnOverlap — %s (이동거리=%.0f, 관통=%d)"),
		*OtherActor->GetName(), GetDistanceTraveled(), bPierceThroughTargets ? 1 : 0);

	const FVector EffectLocation = SweepResult.bBlockingHit ? FVector(SweepResult.ImpactPoint) : OtherActor->GetActorLocation();
	MulticastPlayHitEffect(EffectLocation, SweepResult.ImpactNormal.Rotation());
	OnProjectileHit.Broadcast(OtherActor, SweepResult);

	if (!bPierceThroughTargets)
	{
		Destroy();
	}
}

void AP1Projectile::MulticastPlayHitEffect_Implementation(FVector Location, FRotator Rotation)
{
	if (HitEffectCascade)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, HitEffectCascade, Location, Rotation);
	}
	if (HitEffectNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitEffectNiagara, Location, Rotation);
	}
}
