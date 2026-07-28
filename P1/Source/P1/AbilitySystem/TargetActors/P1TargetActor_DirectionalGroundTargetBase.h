// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "P1TargetActor_DirectionalGroundTargetBase.generated.h"

// "캐릭터 위치에서 조준 방향으로 고정 길이만큼 뻗어나가는 복도"를 조준하는 액터들의 공통 베이스.
// 조준 방향 계산/지면 스냅/확정 처리는 비주얼 컴포넌트 종류와 완전히 무관한 순수 로직이라 여기로
// 뽑아뒀다 — 실제 비주얼 컴포넌트를 만들고 매 틱 위치/회전을 갱신하는 부분만 서브클래스
// (AP1TargetActor_GroundDecal_Rectangle_Deferred 등)가 구현한다.
// 원형 타겟액터(AP1TargetActor_GroundDecal_Deferred, 확정 지점 "중심")와 달리 이쪽은 확정 시
// "먼 쪽 끝점"을 반환한다 — 어빌리티가 Source(자기 위치)→그 지점을 방향으로 재해석해서 쓴다.
UCLASS(Abstract)
class P1_API AP1TargetActor_DirectionalGroundTargetBase : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:
	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ConfirmTargetingAndContinue() override;

	// 길이(사거리)/폭 주입 — 서브클래스가 오버라이드해서 실제 비주얼 컴포넌트 크기를 갱신한다.
	virtual void Configure(float InLength, float InWidth);

protected:
	float Length = 1200.0f;
	float Width = 300.0f;

	// 매 틱 갱신되는 값 — 서브클래스의 UpdateIndicatorTransform()이 그대로 참조해서 쓴다.
	FVector CurrentSourceGroundLocation = FVector::ZeroVector;
	FVector CurrentDirection = FVector::ForwardVector;
	FVector CurrentFarEndpoint = FVector::ZeroVector;

	APlayerController* ResolvePlayerController() const;

	// 캐릭터 위치를 지면에 스냅하고, 카메라 수평 조준 방향을 계산한다. 실패 시(PC/Source 없음) false.
	bool ComputeSourceAndDirection(FVector& OutGroundLocation, FVector& OutDirection) const;

	// 지정 XY 지점에서 아래로 트레이스해 실제 지표면 Z를 구한다 — 실패 시 XYSource를 그대로 반환.
	FVector SnapToGround(const FVector& XYSource) const;

	// Tick()이 CurrentSourceGroundLocation/CurrentDirection/CurrentFarEndpoint를 갱신한 직후 호출된다.
	// 서브클래스가 오버라이드해서 실제 비주얼 컴포넌트(StaticMesh/Decal 등)의 위치·회전을 반영한다.
	virtual void UpdateIndicatorTransform() {}
};
