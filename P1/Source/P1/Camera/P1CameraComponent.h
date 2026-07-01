// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "Camera/P1CameraMode.h"
#include "P1CameraComponent.generated.h"

class UP1CameraModeStack;

template <class TClass> class TSubclassOf;

DECLARE_DELEGATE_RetVal(TSubclassOf<UP1CameraMode>, FP1CameraModeDelegate);

UCLASS()
class P1_API UP1CameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	UP1CameraComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	static UP1CameraComponent* FindCameraComponent(const AActor* Actor);

	virtual void OnRegister() override;
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

	FP1CameraModeDelegate DetermineCameraModeDelegate;

private:
	void UpdateCameraModes();

	UPROPERTY()
	TObjectPtr<UP1CameraModeStack> CameraModeStack;
};
