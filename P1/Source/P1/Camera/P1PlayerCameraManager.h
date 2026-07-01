// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "P1PlayerCameraManager.generated.h"

#define MAX_CAMERA_DEFAULT_FOV (80.f)
#define MAX_CAMERA_DEFAULT_PITCH_MIN (-89.f)
#define MAX_CAMERA_DEFAULT_PITCH_MAX (89.f)

/**
 * 
 */
UCLASS()
class P1_API AP1PlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
public:
	AP1PlayerCameraManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
