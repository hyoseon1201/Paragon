// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Projectiles/P1Projectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
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
	// bShouldBounce 자체는 MaxBounces가 정해지는 InitializeProjectile()에서 켜지만(아래 주석 참고),
	// 이 델리게이트 바인딩은 여기(생성자)에서 무조건 해둔다 — bShouldBounce가 꺼져 있으면 콜백이
	// 애초에 안 불리니 미리 바인딩해도 무해하다. 대신 이 바인딩을 InitializeProjectile()로 미루면,
	// 점블랭크(스폰 위치가 이미 적과 겹친 경우) 발사 시 OnPawnOverlap이 SpawnActor() 도중 즉시 발화해
	// Destroy()를 호출할 수 있어, 그 직후 호출자가 실행하는 InitializeProjectile()이 이미 pending-kill인
	// 액터에 바인딩을 시도하다 "Unable to bind delegate... object may be pending kill" 크래시가 났었다.
	ProjectileMovement->OnProjectileBounce.AddDynamic(this, &AP1Projectile::OnProjectileBounce);
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

	// bShouldBounce 활성화는 여기(BeginPlay)가 아니라 InitializeProjectile()에서 한다 — SpawnActor()는
	// (Deferred가 아닌 이상) BeginPlay()를 반환 전에 동기 호출하므로, 어빌리티가 스폰 직후에 대입하는
	// "Bomb->MaxBounces = N;" 은 항상 이 BeginPlay보다 나중에 실행된다. 즉 여기서 MaxBounces를 읽으면
	// 항상 기본값 0으로 보여서 bShouldBounce가 절대 안 켜지는 버그가 있었다(실제로 이 프로젝트에서 겪음 —
	// OnHit의 자체 CurrentBounceCount 카운터는 MaxBounces와 무관하게 정상 동작해 "튕김 1/1" 로그는
	// 찍히지만, 물리적으로는 bShouldBounce가 꺼져 있어 그 즉시 StopSimulating되어 실제로는 안 튕겼다).
}

void AP1Projectile::InitializeProjectile(float Speed, float InMaxRange, float Radius, float InGravityScale,
	bool bInDrawDebugTrajectory)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;
		ProjectileMovement->ProjectileGravityScale = InGravityScale;
		// 이미 BeginPlay가 지난 시점(스폰 직후 호출)이면 새 속도로 다시 초기화해줘야 실제로 반영된다.
		ProjectileMovement->Velocity = GetActorForwardVector() * Speed;
	}
	if (CollisionComponent)
	{
		CollisionComponent->SetSphereRadius(Radius);
	}
	MaxRange = InMaxRange;
	StartLocation = GetActorLocation();
	bDrawDebugTrajectory = bInDrawDebugTrajectory;
	LastDebugTrajectoryLocation = StartLocation;

	// 튕기는 투사체(Stasis Bomb 등)는 MaxBounces>0로 설정하는 것만으로 물리 반사까지 자동으로 켜지게 한다 —
	// BP에서 bShouldBounce를 따로 켜는 걸 잊어 "MaxBounces는 설정했는데 실제로는 안 튕기는" 실수를 방지.
	// MaxBounces는 어빌리티가 SpawnActor 직후(=BeginPlay보다 나중, InitializeProjectile 호출 직전)에
	// 대입하므로 여기서 읽어야 실제로 설정된 값을 본다(BeginPlay에서 읽으면 항상 0). 델리게이트 바인딩
	// 자체는 생성자에서 이미 끝났으므로(위 생성자 주석 참고) 여기서는 bShouldBounce만 켠다.
	if (MaxBounces > 0 && ProjectileMovement)
	{
		ProjectileMovement->bShouldBounce = true;
	}
}

float AP1Projectile::GetDistanceTraveled() const
{
	return FVector::Dist(StartLocation, GetActorLocation());
}

void AP1Projectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if ENABLE_DRAW_DEBUG
	// 서버/클라 각자 자기가 보고 있는 위치(리플리케이트된 위치 포함) 기준으로 그린다 — 권위 체크 없이
	// 틱을 도는 쪽 전부(호스트+원격 클라 PIE 창 등) 각자 화면에서 궤적을 확인할 수 있게.
	if (bDrawDebugTrajectory)
	{
		DrawDebugLine(GetWorld(), LastDebugTrajectoryLocation, GetActorLocation(), FColor::Orange, false, 8.0f, 0, 2.5f);
		LastDebugTrajectoryLocation = GetActorLocation();
	}
#endif

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

	// 콤플렉스 콜리전(스태틱메시가 여러 프리미티브로 구성된 경우 등)은 같은 물리 틱에서 OnComponentHit이
	// 두 번 이상 발화할 수 있다 — Destroy()가 그 프레임 즉시 액터를 없애지 못해 두 번째 호출이 끼어들면
	// OnProjectileHit이 중복 브로드캐스트(=폭발/데미지가 여러 번 발동)된다. bHasExploded로 한 번만 통과.
	if (bHasExploded)
	{
		return;
	}
	bHasExploded = true;

	UE_LOG(LogP1, Log, TEXT("[Projectile] OnHit — %s (이동거리=%.0f)"), *OtherActor->GetName(), GetDistanceTraveled());

	MulticastPlayHitEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	OnProjectileHit.Broadcast(OtherActor, Hit);
	Destroy();
}

void AP1Projectile::OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (!ProjectileMovement)
	{
		return;
	}

	// ImpactVelocity = 엔진이 자체 바운스 계산을 하기 "직전"의 충돌 속도(OldVelocity) — 이걸 직접
	// 반사시켜 덮어쓰면, 엔진이 방금 계산해 넣어둔 Bounciness/Friction 기반 결과를 완전히 대체한다.
	// 이 콜백이 끝난 직후 엔진이 이어서 정지 임계값(BounceVelocityStopSimulatingThreshold) 체크를
	// 하므로, 여기서 반사 후 속도가 그 임계값을 넉넉히 넘도록 보장하는 게 핵심이다.
	const FVector ReflectedVelocity = FMath::GetReflectionVector(ImpactVelocity, ImpactResult.ImpactNormal);
	ProjectileMovement->Velocity = ReflectedVelocity * BounceRestitution;

	UE_LOG(LogP1, Log, TEXT("[Projectile] 바운스 반사 — %s | Speed %.0f → %.0f"),
		*GetName(), ImpactVelocity.Size(), ProjectileMovement->Velocity.Size());
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

	// 관통이 아닌(단발 폭발) 투사체는 OnHit과 동일한 "한 번만 터진다" 가드를 공유한다 — 같은 물리 틱에
	// Pawn 오버랩과 WorldStatic 충돌이 동시에 잡히는 경우까지 방어. 관통 투사체는 대상마다 계속
	// 브로드캐스트돼야 하는 게 정상 동작이라 이 가드를 적용하지 않는다(HitActors 중복 방지로 충분).
	if (!bPierceThroughTargets)
	{
		if (bHasExploded)
		{
			return;
		}
		bHasExploded = true;
	}

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
