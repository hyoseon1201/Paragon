// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/P1PlayerCameraManager.h"

AP1PlayerCameraManager::AP1PlayerCameraManager(const FObjectInitializer& ObjectInitializer)
{
	DefaultFOV = MAX_CAMERA_DEFAULT_FOV;
	ViewPitchMin = MAX_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = MAX_CAMERA_DEFAULT_PITCH_MAX;
}
