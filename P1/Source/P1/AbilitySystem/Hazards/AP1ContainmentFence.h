// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AP1ContainmentFence.generated.h"

class USphereComponent;
class UParticleSystemComponent;
class UParticleSystem;
class AP1CharacterBase;
struct FHitResult;

// Dekker E — Containment Fence. GAS/데미지를 전혀 모르는 순수 배치형 액터(AP1Projectile과 동일한
// 역할 분담 — 실제 GAS 판정/커밋은 전부 어빌리티가 하고, 이 액터는 "배치된 원형 장판이 적 히어로의
// 이동을 막는다"는 순수 공간적 판정만 담당). 데미지/디버프가 아니라 위치 자체를 매 틱 clamp하는
// 방식이라 GameplayEffect를 전혀 쓰지 않는다 — GE/태그는 "스탯이나 상태"를 표현하는 도구지 "이 지점을
// 넘어갈 수 없다"는 공간적 제약을 표현하는 도구가 아니기 때문.
//
// 콜리전 채널을 새로 만들어 Block으로 막는 대신 Overlap + 매 틱 위치 clamp를 쓴다 — 아군/적군이 같은
// AP1HeroCharacter 클래스를 공유하는 이상 "적만 막고 아군은 통과"는 콜리전 채널(클래스 단위 정적 설정)
// 만으로는 표현 불가능하고, 결국 팀 판별은 항상 런타임에 동적으로 해야 한다. 배치 시점에 이미 원 안에
// 있던 적 히어로는 못 나가게(감금), 밖에 있던 적 히어로는 못 들어오게(차단) — 각자 "시작한 쪽"을 벗어나지
// 못하게 매 틱 clamp한다.
//
// 서버 권위 — bReplicates=true이므로 반드시 HasAuthority()인 실행 경로(어빌리티의 서버 인스턴스)에서만
// 스폰해야 한다. 파티클 컴포넌트는 네이티브 서브오브젝트라 액터 리플리케이션만으로 전 클라이언트에
// 자동으로 보인다(캐릭터 코스메틱과 달리 Multicast RPC가 필요 없음 — 이 액터 자체가 이미 리플리케이트되는
// 독립 액터이기 때문).
UCLASS()
class P1_API AP1ContainmentFence : public AActor
{
	GENERATED_BODY()

public:
	AP1ContainmentFence();

	// 서버 스폰 직후 어빌리티가 호출 — 반경/지속시간/시전자를 주입하고, 배치 시점에 이미 원 안에 있던
	// 적 히어로를 스캔해 감금 대상으로 등록한다.
	void InitializeFence(float InRadius, float InDuration, AP1CharacterBase* InInstigator);

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Fence")
	TObjectPtr<USphereComponent> OverlapComponent;

	UPROPERTY(VisibleAnywhere, Category = "Fence")
	TObjectPtr<UParticleSystemComponent> FenceEffectComponent;

	// FenceEffectComponent에 재생할 캐스케이드 — BP에서 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Fence")
	TObjectPtr<UParticleSystem> FenceEffectTemplate;

	// 이펙트 에셋 자체가 표현하는 기준 반경(월드 유닛, Scale=1일 때) — AP1TargetActor_GroundDecal의
	// BaseMeshSize와 동일한 목적. 실제 배치 반경(InitializeFence의 InRadius)과 이 값의 비율로
	// FenceEffectComponent의 스케일을 계산한다.
	UPROPERTY(EditDefaultsOnly, Category = "Fence")
	float BaseEffectRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Fence|Debug")
	bool bShowDebug = false;

	// 새로 원 경계에 닿은 액터 감지 — 배치 이후 밖에서 접근해오는 적 히어로를 "못 들어오게" 등록한다.
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	float FenceRadius = 300.0f;
	FVector FenceCenter = FVector::ZeroVector;
	TWeakObjectPtr<AP1CharacterBase> FenceInstigator;

	// 값 = 이 액터가 "시작한 쪽"(true=원 안, false=원 밖) — 그 반대쪽으로 못 넘어가게 매 틱 clamp한다.
	TMap<TWeakObjectPtr<AActor>, bool> TrackedEnemies;

	// TargetActor가 이 장판이 막아야 할 대상(적 히어로)인지 판별.
	bool IsBlockableEnemy(const AActor* TargetActor) const;
};
