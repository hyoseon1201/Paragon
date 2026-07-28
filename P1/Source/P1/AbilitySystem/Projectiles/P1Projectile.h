// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1Projectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileHitSignature, AActor*, HitActor, const FHitResult&, HitResult);

// GAS/데미지/CC를 전혀 모르는 순수 "날아가다가 뭔가에 부딪히면 알려주는" 액터. 데미지를 넣고 싶은
// 어빌리티(UP1DamageGameplayAbility 상속)든, CC만 있는 어빌리티든, 둘 다인 어빌리티든 전부 이 액터를
// 그대로 스폰해서 OnProjectileHit만 구독하면 된다 — 실제로 뭘 적용할지는 항상 그 어빌리티 자신의
// 기존 헬퍼(ApplyDamageToTarget 등)가 처리한다. 투사체 자신이 GameplayEffect를 직접 적용하지 않는
// 이유: 그러면 어빌리티마다 다른 데미지 계수 계산(FlatDamage/PhysicalPowerCoefficient 등)을 우회하게 됨.
//
// 서버 권위 — bReplicates=true이므로 반드시 HasAuthority()인 실행 경로(어빌리티의 서버 인스턴스)에서만
// 스폰해야 한다. 클라이언트는 리플리케이트된 인스턴스를 그대로 보기만 한다(클라이언트 예측은 이
// 프로젝트 스코프에서 하지 않음 — RTT만큼 늦게 나타나는 건 허용).
UCLASS()
class P1_API AP1Projectile : public AActor
{
	GENERATED_BODY()

public:
	AP1Projectile();

	UPROPERTY(BlueprintAssignable, Category = "Projectile")
	FOnProjectileHitSignature OnProjectileHit;

	// 스폰 직후 어빌리티가 호출 — 속도/충돌 반경/최대 사거리를 주입한다. MaxRange<=0이면 사거리 제한
	// 없이 InitialLifeSpan(생성자에서 설정된 안전장치)으로만 소멸한다. InGravityScale은 기본값 0(직선
	// 논타겟 스킬샷, RangedAttack/Ionizer 등)을 유지 — 곡사 투사체(Stasis Bomb 등)만 호출부에서 양수
	// 값을 넘겨 포물선을 만든다(조준 각도 자체는 스폰 시 SpawnRotation/Velocity 방향으로 이미 반영됨).
	// bInDrawDebugTrajectory=true면 매 틱 이동 경로를 디버그 라인으로 그린다(튕기는 경로 확인용).
	void InitializeProjectile(float Speed, float InMaxRange, float Radius = 15.0f, float InGravityScale = 0.0f,
		bool bInDrawDebugTrajectory = false);

	// 지금까지 날아간 거리 — 거리 비례 효과(예: 사거리에 비례해 늘어나는 스턴 지속시간)에 사용.
	float GetDistanceTraveled() const;

	// 피격 시 재생할 임팩트/폭발 이펙트 — 캐스케이드/나이아가라 둘 다 선택적으로 넣을 수 있다(둘 다
	// 비워두면 아무것도 재생 안 함, 둘 다 채우면 둘 다 재생). 어떤 시스템을 쓸지는 BP마다 다를 수 있어
	// 특정 파티클 프레임워크 하나로 강제하지 않는다.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	TObjectPtr<UParticleSystem> HitEffectCascade;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	TObjectPtr<UNiagaraSystem> HitEffectNiagara;

	// true면 대상을 맞혀도 소멸하지 않고 계속 날아가며 경로상의 모든 적을 관통한다(예: Photon Disruptor의
	// 관통 빔). false(기본값)면 첫 유효 타격 직후 스스로 Destroy()된다 — 지금까지의 단발 투사체(Ionizer 등)와
	// 동일한 동작을 그대로 유지한다. 이 값과 무관하게 벽 등 WorldStatic/WorldDynamic에 막히면 항상 소멸한다.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	bool bPierceThroughTargets = false;

	// 0(기본값)이면 벽 등에 처음 부딪히는 즉시 소멸(Ionizer 등 기존 동작 그대로). N>0이면 WorldStatic/
	// WorldDynamic과의 충돌을 N번까지 튕겨내며 계속 날아가고, N+1번째 충돌에서야 소멸한다(예: Stasis
	// Bomb의 "1회 튕긴 뒤 다음 충돌에서 폭발"). Pawn(Overlap)과의 충돌은 이 값과 무관하게 항상 즉시
	// 소멸(=적을 맞히면 튕기지 않고 바로 터짐) — bPierceThroughTargets가 true가 아닌 이상 그대로다.
	// true로 설정 시 BeginPlay에서 자동으로 ProjectileMovement->bShouldBounce를 켜준다.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	int32 MaxBounces = 0;

	// 튕길 때 충돌 직전 속도 대비 반사 속도 비율(0~1) — MaxBounces>0일 때만 의미가 있다. 엔진 기본
	// 바운스(Bounciness/Friction, 바닥 PhysicalMaterial과 결합)에 그대로 맡기면 각도/마찰에 따라 반사
	// 속도가 정지 임계값 밑으로 떨어져 그 자리에 멈춰버릴 수 있어(="튕겨야 하는데 안 튕김"), 충돌
	// 이벤트에서 우리가 직접 반사 속도를 계산해 덮어써 지형과 무관하게 항상 확실한 다음 아크를 보장한다.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BounceRestitution = 0.6f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// WorldStatic/WorldDynamic(Block)에 물리적으로 부딪혔을 때 — 관통 여부와 무관하게 항상 소멸한다.
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	// Pawn(Overlap)과 겹쳤을 때 — 물리적으로 막히지 않고 그대로 통과하며 이 콜백만 받는다. bPierceThroughTargets에
	// 따라 계속 날아갈지(관통) 여기서 Destroy()할지(단발) 결정한다. Overlap을 쓰는 이유: Block으로 두면 단발
	// 투사체는 이미 Destroy()로 즉시 사라지니 상관없지만, 관통 투사체는 첫 대상과 물리적으로 충돌해 멈추거나
	// 튕기면 안 되기 때문 — 애초에 Pawn 채널 자체를 Overlap으로 둬서 두 경우 모두 정상 동작하게 한다.
	UFUNCTION()
	void OnPawnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// ProjectileMovementComponent::OnProjectileBounce 구독 — 생성자에서 무조건 바인딩(MaxBounces와 무관,
	// bShouldBounce가 꺼져 있으면 애초에 콜백이 안 불려서 무해함 — InitializeProjectile()로 미뤘다가
	// 점블랭크 발사 시 pending-kill 액터에 바인딩을 시도해 크래시 나던 버그가 있었음). 엔진이 자체
	// Bounciness/Friction으로 계산한 반사 속도를 우리가 직접 덮어써서, 정지 임계값 밑으로 떨어져 그
	// 자리에 멈추는 것을 방지하고 항상 확실하게 다음 아크로 이어지게 한다. 엔진 주석 "Event may modify
	// velocity or threshold, so check velocity threshold now"가 명시하는 정식 확장 지점.
	UFUNCTION()
	void OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	// HitEffectCascade/HitEffectNiagara를 실제로 스폰 — 서버(OnHit)에서만 호출할 것. 투사체 자신이
	// 곧 Destroy()되므로 자신에게 부착하지 않고 그 순간의 월드 위치/노멀에 독립적으로 스폰한다
	// (AP1CharacterBase의 Multicast 코스메틱 패턴과 동일한 이유 — 서버 단독 SpawnEmitter는 로컬+서버
	// 에서만 보이므로 반드시 NetMulticast를 거쳐야 시뮬레이티드 프록시 화면에도 보인다).
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayHitEffect(FVector Location, FRotator Rotation);

private:
	FVector StartLocation = FVector::ZeroVector;
	float MaxRange = 0.0f;

	// 관통 투사체가 같은 대상을 두 번 타격하지 않도록 — 오버랩은 대상 콜리전 형상에 따라 이론상 여러 번
	// 발화할 수 있어 방어적으로 기록해둔다.
	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActors;

	// 지금까지 WorldStatic/WorldDynamic에 튕긴 횟수.
	int32 CurrentBounceCount = 0;

	// 최종 폭발/소멸 브로드캐스트가 두 번 이상 나가지 않도록 하는 가드 — 콤플렉스 콜리전(스태틱메시가
	// 여러 프리미티브로 구성된 경우 등)에 부딪히면 같은 물리 틱 안에서 OnComponentHit이 두 번 이상
	// 발화할 수 있는데, Destroy()가 그 프레임 즉시 액터를 제거하지 못해 두 번째 호출이 Destroy() 이전에
	// 끼어들면 OnProjectileHit이 중복 브로드캐스트된다(=폭발/데미지가 여러 번 발동하는 것처럼 보임).
	bool bHasExploded = false;

	// 디버그 궤적 라인용 — true면 Tick마다 직전 위치→현재 위치를 그린다.
	bool bDrawDebugTrajectory = false;
	FVector LastDebugTrajectoryLocation = FVector::ZeroVector;
};
