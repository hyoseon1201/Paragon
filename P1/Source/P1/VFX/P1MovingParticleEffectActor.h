// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1MovingParticleEffectActor.generated.h"

class UParticleSystemComponent;
class UParticleSystem;

// 두 지점 사이를 정해진 시간 동안 이동하다 자동 파괴되는 1회성 파티클 이펙트 액터.
// AP1CharacterBase::MulticastPlayMovingParticleEffect()가 순수 로컬 코스메틱으로 스폰한다
// (bReplicates=false — 서버가 존재조차 모름, AP1DamageNumberActor와 동일한 이유).
//
// 원래는 이 클래스 없이 SpawnActor<AActor>()+NewObject<UParticleSystemComponent>()+커스텀
// TWeakObjectPtr+FTimerHandle+수동 Destroy() 조합으로 직접 구현했었는데, 세 차례 시도에도
// EXCEPTION_ACCESS_VIOLATION(오너 액터가 유효성 체크는 통과했는데 몇 줄 뒤 Destroy() 호출
// 시점엔 죽어있는 패턴)이 계속 재현됐다 — 정확한 근본 원인은 끝내 특정 못함. 대신 이 프로젝트에서
// 이미 검증된 패턴(AP1DamageNumberActor가 쓰는 Tick()+SetLifeSpan() 조합 — 커스텀 위크포인터/
// 타이머 관리가 전혀 없는, 엔진이 전적으로 수명을 관리하는 방식)으로 갈아엎어 문제 자체를 없앴다.
UCLASS()
class P1_API AP1MovingParticleEffectActor : public AActor
{
	GENERATED_BODY()

public:
	AP1MovingParticleEffectActor();

	// 스폰 직후 1회 호출 — 시작/끝 지점, 지속시간, 파티클 템플릿을 세팅하고 SetLifeSpan()으로
	// 지속시간 종료 시 엔진이 자동으로 파괴하게 한다.
	void InitializeMovingEffect(UParticleSystem* ParticleTemplate, const FVector& InStartLocation, const FVector& InEndLocation, float InDuration);

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "MovingParticleEffect")
	TObjectPtr<UParticleSystemComponent> ParticleComponent;

private:
	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	float Duration = 0.0f;
	double StartTime = 0.0;
};
