// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "P1TargetActor_GroundDecal_Deferred.generated.h"

class UDecalComponent;

// 원형 지면 조준 타겟액터. 카메라 트레이스+사거리 클램프, 확정 시 타겟데이터 반환. 표시는
// UDecalComponent(Deferred Decal)로 렌더링한다 — StaticMesh 기반 구버전은 제거됨(전부 이 클래스로 통일).
UCLASS(Blueprintable)
class P1_API AP1TargetActor_GroundDecal_Deferred : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:
	AP1TargetActor_GroundDecal_Deferred();

	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ConfirmTargetingAndContinue() override;

	// 어빌리티가 스폰 직후 사거리/AOE 반경을 주입한다. 데칼 크기도 반경에 맞춰 조정된다.
	void Configure(float InMaxRange, float InAOERadius);

protected:
	// 지면 위에 투영되는 원형 데칼. Decal Material은 BP 서브클래스(또는 이 컴포넌트를 선택해)에서 지정.
	UPROPERTY(VisibleAnywhere, Category = "Targeting")
	TObjectPtr<UDecalComponent> IndicatorDecalComponent;

	// 데칼 박스의 투영 깊이(반값, cm) — 지면 위아래로 이 정도 두께의 박스가 있어야 실제 바닥 지오메트리와
	// 확실히 겹친다(너무 얇으면 바닥 높이가 살짝만 달라져도 투영이 안 먹힘).
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ProjectionDepth = 200.0f;

	float MaxRange = 1200.0f;
	float AOERadius = 300.0f;

	// 이번 틱에 계산된 지면 조준 위치.
	FVector CurrentTargetLocation = FVector::ZeroVector;

	// 카메라 방향 트레이스로 지면 위치를 구하고 사거리로 클램프한다. 실패 시 false.
	bool ComputeTargetLocation(FVector& OutLocation) const;

	// PrimaryPC가 아직 없을 때 어빌리티 액터 정보에서 PC를 얻는다.
	APlayerController* ResolvePlayerController() const;
};
