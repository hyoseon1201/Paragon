// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/P1CameraComponent.h"
#include "Camera/P1CameraMode.h"

UP1CameraComponent::UP1CameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), CameraModeStack(nullptr)
{
}

// static
UP1CameraComponent* UP1CameraComponent::FindCameraComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UP1CameraComponent>() : nullptr;
}

void UP1CameraComponent::OnRegister()
{
	Super::OnRegister();

	if (!CameraModeStack)
	{
		CameraModeStack = NewObject<UP1CameraModeStack>(this);
	}
}

void UP1CameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	check(CameraModeStack);

	UpdateCameraModes();

	FP1CameraModeView ModeView;
	if (CameraModeStack->EvaluateStack(DeltaTime, ModeView))
	{
		// Snap component world transform so audio listener / editor gizmo follow the camera
		SetWorldLocationAndRotation(ModeView.Location, ModeView.Rotation);

		DesiredView.Location             = ModeView.Location;
		DesiredView.Rotation             = ModeView.Rotation;
		DesiredView.FOV                  = ModeView.FieldOfView;
		DesiredView.OrthoWidth           = 512.f;
		DesiredView.AspectRatio          = 16.f / 9.f;
		DesiredView.bConstrainAspectRatio = false;
		DesiredView.ProjectionMode       = ECameraProjectionMode::Perspective;
		DesiredView.PostProcessBlendWeight = 1.f;
	}
	else
	{
		Super::GetCameraView(DeltaTime, DesiredView);
	}
}

void UP1CameraComponent::UpdateCameraModes()
{
	if (DetermineCameraModeDelegate.IsBound())
	{
		if (TSubclassOf<UP1CameraMode> CameraMode = DetermineCameraModeDelegate.Execute())
		{
			CameraModeStack->PushCameraMode(CameraMode);
		}
	}
}
