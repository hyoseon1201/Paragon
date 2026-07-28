// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "P1CameraMode.generated.h"

class UP1CameraComponent;

// Single frame view output produced by a camera mode
USTRUCT()
struct FP1CameraModeView
{
	GENERATED_BODY()

	FP1CameraModeView();

	// Blends this view toward Other by OtherWeight [0,1]
	void Blend(const FP1CameraModeView& Other, float OtherWeight);

	FVector   Location;
	FRotator  Rotation;
	FRotator  ControlRotation;
	float     FieldOfView;
};

UENUM(BlueprintType)
enum class EP1CameraModeBlendFunction : uint8
{
	Linear,
	EaseIn,
	EaseOut,
	EaseInOut,
};

/**
 * Abstract base for a single camera mode (e.g. default third-person, skill-cast overhead).
 * Subclass and override UpdateView() to define camera position per mode.
 * Instances are owned and pooled by UP1CameraModeStack.
 */
UCLASS(Abstract, Blueprintable, NotBlueprintType)
class P1_API UP1CameraMode : public UObject
{
	GENERATED_BODY()

public:
	UP1CameraMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UP1CameraComponent* GetP1CameraComponent() const;
	AActor* GetTargetActor() const;

	virtual void OnActivation() {}
	virtual void OnDeactivation() {}

	virtual void UpdateView(float DeltaTime);
	void UpdateBlending(float DeltaTime);

	float GetBlendWeight() const { return BlendWeight; }
	const FP1CameraModeView& GetCameraView() const { return View; }

protected:
	FVector  GetPivotLocation() const;
	FRotator GetPivotRotation() const;

	UPROPERTY(EditDefaultsOnly, Category = "CameraMode", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float FieldOfView = 80.f;

	// Camera position relative to pivot in local (pivot-rotated) space
	UPROPERTY(EditDefaultsOnly, Category = "CameraMode")
	FVector ViewOffset = FVector(-300.f, 0.f, 75.f);

	UPROPERTY(EditDefaultsOnly, Category = "CameraMode", meta = (ClampMin = "0.0"))
	float BlendTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "CameraMode")
	EP1CameraModeBlendFunction BlendFunction = EP1CameraModeBlendFunction::EaseOut;

	UPROPERTY(EditDefaultsOnly, Category = "CameraMode", meta = (ClampMin = "1.0"))
	float BlendExponent = 4.f;

	// 카메라가 지형/벽에 파고들 때 피벗 쪽으로 당겨오는 충돌 회피 — 꺼두면 SpringArm 없이 순수 오프셋
	// 카메라라 벽 뒤/땅 밑으로 그대로 박혀 들어간다.
	UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Collision")
	bool bPreventPenetration = true;

	// 카메라를 점이 아니라 이 반경의 구체로 취급해 스윕 — 값이 클수록 여유를 두고 더 일찍 당겨온다
	// (카메라의 니어클립 평면 크기와 비슷하게 잡는 게 보통 무난하다).
	UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Collision", meta = (ClampMin = "0.0", EditCondition = "bPreventPenetration"))
	float PenetrationProbeSize = 12.f;

	// 충돌 판정에 쓸 트레이스 채널 — 게임플레이 트레이스(ECC_Visibility)와 분리해서, 레벨 쪽에서
	// "카메라만 막고 싶은" 오브젝트(예: 얇은 잎사귀)를 이 채널로만 개별 대응할 수 있게 한다.
	UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Collision", meta = (EditCondition = "bPreventPenetration"))
	TEnumAsByte<ECollisionChannel> PenetrationCollisionChannel = ECC_Camera;

	// 막혀서 당겨질 때의 초당 이동 속도 — 순간적으로 벽 안에 파고드는 프레임이 남지 않도록 충분히 빠르게.
	UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Collision", meta = (ClampMin = "0.0", EditCondition = "bPreventPenetration"))
	float PenetrationPullInSpeed = 5000.f;

	// 막힘이 풀려 원래 거리로 되돌아갈 때의 초당 이동 속도 — 당겨질 때보다 훨씬 느리게 둬서, 기둥을
	// 살짝 스치기만 해도 카메라가 확 튀어나왔다 들어가는 팝핑을 방지한다.
	UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Collision", meta = (ClampMin = "0.0", EditCondition = "bPreventPenetration"))
	float PenetrationPushOutSpeed = 500.f;

	FP1CameraModeView View;

	float BlendAlpha  = 1.f;
	float BlendWeight = 1.f;

	// 지난 프레임에 실제로 당겨졌던 거리 — 이번 프레임 목표치와 보간해 스냅 없이 부드럽게 수렴시킨다.
	float CurrentPenetrationPullback = 0.f;

	// PivotLocation에서 InOutCameraLocation까지 스윕해 막히면 InOutCameraLocation을 피벗 쪽으로 당긴다.
	void PreventCameraPenetration(const FVector& PivotLocation, FVector& InOutCameraLocation, float DeltaTime);

	friend class UP1CameraModeStack;
};


/**
 * Manages a stack of camera modes and evaluates their blended view each frame.
 * Index 0 = oldest (lowest priority), Last = newest (highest priority).
 */
UCLASS()
class P1_API UP1CameraModeStack : public UObject
{
	GENERATED_BODY()

public:
	UP1CameraModeStack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void PushCameraMode(TSubclassOf<UP1CameraMode> CameraModeClass);
	bool EvaluateStack(float DeltaTime, FP1CameraModeView& OutView);

private:
	UP1CameraMode* GetCameraModeInstance(TSubclassOf<UP1CameraMode> CameraModeClass);

	// Pool of all instantiated modes (never destroyed mid-session)
	UPROPERTY()
	TArray<TObjectPtr<UP1CameraMode>> CameraModeInstances;

	// Subset currently on the active blend stack
	UPROPERTY()
	TArray<TObjectPtr<UP1CameraMode>> ActiveModes;
};
