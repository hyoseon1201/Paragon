// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "P1PlayerState.generated.h"

class UP1AbilitySystemComponent;
class UP1AttributeSet;

UCLASS()
class P1_API AP1PlayerState : public APlayerState, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AP1PlayerState();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UP1AttributeSet* GetAttributeSet() const { return AttributeSet; }
	UP1AbilitySystemComponent* GetP1AbilitySystemComponent() const { return AbilitySystemComponent; }

	// IGenericTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

protected:
	UPROPERTY()
	TObjectPtr<UP1AbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UP1AttributeSet> AttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamId)
	FGenericTeamId MyTeamId;

	UFUNCTION()
	void OnRep_MyTeamId();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
