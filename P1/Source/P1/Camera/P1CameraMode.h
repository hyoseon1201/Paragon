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

	FP1CameraModeView View;

	float BlendAlpha  = 1.f;
	float BlendWeight = 1.f;

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
