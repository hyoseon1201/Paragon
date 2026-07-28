// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/TargetActors/P1TargetActor_DirectionalGroundTargetBase.h"
#include "P1TargetActor_GroundDecal_Rectangle_Deferred.generated.h"

class UDecalComponent;

// AP1TargetActor_DirectionalGroundTargetBase의 Deferred Decal 표시 버전(직사각형 인디케이터).
// 조준 방향/지면 스냅/확정 로직은 베이스가 담당하고, 여기서는 데칼 박스의 크기·위치·회전만 갱신한다.
UCLASS(Blueprintable)
class P1_API AP1TargetActor_GroundDecal_Rectangle_Deferred : public AP1TargetActor_DirectionalGroundTargetBase
{
	GENERATED_BODY()

public:
	AP1TargetActor_GroundDecal_Rectangle_Deferred();

	virtual void Configure(float InLength, float InWidth) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Targeting")
	TObjectPtr<UDecalComponent> IndicatorDecalComponent;

	// 데칼 박스의 투영 깊이(반값, cm) — AP1TargetActor_GroundDecal_Deferred와 동일한 목적.
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ProjectionDepth = 200.0f;

	virtual void UpdateIndicatorTransform() override;
};
