// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "P1GameInstance.generated.h"

UCLASS()
class P1_API UP1GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
};
