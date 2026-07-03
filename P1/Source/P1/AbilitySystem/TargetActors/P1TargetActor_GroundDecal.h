// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "P1TargetActor_GroundDecal.generated.h"

class UStaticMeshComponent;

// 지면 조준(ground-targeted) 액터.
// 소유 클라이언트에서 카메라 방향으로 지면 위치를 계산하고(사거리 클램프),
// 그 자리에 원형 인디케이터 메시를 표시한다. 확정 시 위치를 타겟 데이터로 반환해 서버에 복제한다.
// UAbilityTask_WaitTargetData(ConfirmationType=UserConfirmed)와 함께 사용.
// (Deferred Decal 대신 StaticMesh + 일반 Translucent Surface 머티리얼 사용 — Substrate 데칼 렌더링 이슈 회피)
UCLASS(Blueprintable)
class P1_API AP1TargetActor_GroundDecal : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:
	AP1TargetActor_GroundDecal();

	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ConfirmTargetingAndContinue() override;
	virtual void Destroyed() override;

	// 어빌리티가 스폰 직후 사거리/AOE 반경을 주입한다. 인디케이터 크기도 반경에 맞춰 조정된다.
	void Configure(float InMaxRange, float InAOERadius);

protected:
	// 지면 위에 표시되는 원형 인디케이터 메시. 메시/머티리얼은 BP 서브클래스에서 지정.
	// 기준 메시는 XY 평면에 평평하게 놓인 정사각형이라고 가정 (엔진 기본 Plane 메시 권장, 기본 100x100 유닛).
	UPROPERTY(VisibleAnywhere, Category = "Targeting")
	TObjectPtr<UStaticMeshComponent> IndicatorMeshComponent;

	// 기준 메시의 한 변 크기(유닛). 엔진 기본 Plane = 100. AOERadius에 맞춰 스케일 계산할 때 사용.
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float BaseMeshSize = 100.0f;

	// 지면과의 Z-fighting 방지용으로 살짝 띄우는 높이.
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float GroundOffset = 2.0f;

	// 소스(시전자) 위치 기준 최대 수평 사거리.
	float MaxRange = 1200.0f;

	// AOE 반경 — 데칼 크기 및 데미지 판정 반경과 일치.
	float AOERadius = 300.0f;

	// 이번 틱에 계산된 지면 조준 위치.
	FVector CurrentTargetLocation = FVector::ZeroVector;

	// 디버그 로그 스로틀 및 상태 변화 감지용.
	float DebugLogAccum = 0.0f;
	bool bLastComputeOk = false;

	// 카메라 방향 트레이스로 지면 위치를 구하고 사거리로 클램프한다. 실패 시 false.
	bool ComputeTargetLocation(FVector& OutLocation) const;

	// PrimaryPC가 아직 없을 때 어빌리티 액터 정보에서 PC를 얻는다.
	APlayerController* ResolvePlayerController() const;
};
