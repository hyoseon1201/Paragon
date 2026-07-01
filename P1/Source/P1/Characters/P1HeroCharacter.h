// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/P1CharacterBase.h"
#include "AbilitySystemComponent.h"
#include "P1HeroCharacter.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UP1CameraComponent;
class UP1HeroComponent;

UCLASS()
class P1_API AP1HeroCharacter : public AP1CharacterBase
{
	GENERATED_BODY()

public:
	AP1HeroCharacter();

	// IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	void InitAbilityActorInfo();
	void AddDefaultAbilities();
	void BindMoveSpeedAttribute();

	FDelegateHandle MoveSpeedChangedDelegateHandle;

	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<UP1CameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<UP1HeroComponent> HeroComponent;
};
