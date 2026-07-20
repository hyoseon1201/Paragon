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
	// 없이 InitialLifeSpan(생성자에서 설정된 안전장치)으로만 소멸한다.
	void InitializeProjectile(float Speed, float InMaxRange, float Radius = 15.0f);

	// 지금까지 날아간 거리 — 거리 비례 효과(예: 사거리에 비례해 늘어나는 스턴 지속시간)에 사용.
	float GetDistanceTraveled() const;

	// 피격 시 재생할 임팩트/폭발 이펙트 — 캐스케이드/나이아가라 둘 다 선택적으로 넣을 수 있다(둘 다
	// 비워두면 아무것도 재생 안 함, 둘 다 채우면 둘 다 재생). 어떤 시스템을 쓸지는 BP마다 다를 수 있어
	// 특정 파티클 프레임워크 하나로 강제하지 않는다.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	TObjectPtr<UParticleSystem> HitEffectCascade;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	TObjectPtr<UNiagaraSystem> HitEffectNiagara;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	// HitEffectCascade/HitEffectNiagara를 실제로 스폰 — 서버(OnHit)에서만 호출할 것. 투사체 자신이
	// 곧 Destroy()되므로 자신에게 부착하지 않고 그 순간의 월드 위치/노멀에 독립적으로 스폰한다
	// (AP1CharacterBase의 Multicast 코스메틱 패턴과 동일한 이유 — 서버 단독 SpawnEmitter는 로컬+서버
	// 에서만 보이므로 반드시 NetMulticast를 거쳐야 시뮬레이티드 프록시 화면에도 보인다).
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayHitEffect(FVector Location, FRotator Rotation);

private:
	FVector StartLocation = FVector::ZeroVector;
	float MaxRange = 0.0f;
};
