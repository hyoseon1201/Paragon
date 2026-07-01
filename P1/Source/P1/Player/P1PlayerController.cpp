// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/P1PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/P1PlayerCameraManager.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "Characters/P1CharacterBase.h"
#include "UI/HUD/P1HUD.h"
#include "Player/P1PlayerState.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "P1.h"

AP1PlayerController::AP1PlayerController()
{
	PlayerCameraManagerClass = AP1PlayerCameraManager::StaticClass();
}

void AP1PlayerController::CreateHUDForASC(UAbilitySystemComponent* InASC)
{
	if (!IsLocalController() || !IsValid(InASC))
	{
		return;
	}

	AP1PlayerState* PS = GetPlayerState<AP1PlayerState>();
	if (!IsValid(PS))
	{
		return;
	}

	if (AP1HUD* P1HUD = GetHUD<AP1HUD>())
	{
		P1HUD->InitOverlay(this, PS, InASC, PS->GetAttributeSet());
	}
}

void AP1PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
			else
			{
				UE_LOG(LogP1, Warning, TEXT("AP1PlayerController::SetupInputComponent: DefaultMappingContext is not set on %s"), *GetName());
			}
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AP1PlayerController::HandleMove);
		}
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AP1PlayerController::HandleLook);
		}
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AP1PlayerController::HandleJumpStarted);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AP1PlayerController::HandleJumpCompleted);
		}

		for (const auto& [Action, Tag] : AbilityInputActions)
		{
			if (Action)
			{
				EnhancedInputComponent->BindAction(Action, ETriggerEvent::Started, this,
					&AP1PlayerController::HandleAbilityInputPressed, Tag);
				EnhancedInputComponent->BindAction(Action, ETriggerEvent::Completed, this,
					&AP1PlayerController::HandleAbilityInputReleased, Tag);
			}
		}
	}
}

void AP1PlayerController::HandleMove(const FInputActionValue& Value)
{
	ACharacter* ControlledCharacter = GetCharacter();
	if (!IsValid(ControlledCharacter))
	{
		return;
	}

	const FVector2D MoveVector = Value.Get<FVector2D>();

	const FRotator CurrentControlRotation = GetControlRotation();
	const FRotator YawRotation(0.0, CurrentControlRotation.Yaw, 0.0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	ControlledCharacter->AddMovementInput(ForwardDirection, MoveVector.Y);
	ControlledCharacter->AddMovementInput(RightDirection, MoveVector.X);
}

void AP1PlayerController::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();

	AddYawInput(LookVector.X);
	AddPitchInput(-LookVector.Y);
}

void AP1PlayerController::HandleJumpStarted(const FInputActionValue& Value)
{
	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		ControlledCharacter->Jump();
	}
}

void AP1PlayerController::HandleJumpCompleted(const FInputActionValue& Value)
{
	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		ControlledCharacter->StopJumping();
	}
}

void AP1PlayerController::HandleAbilityInputPressed(FGameplayTag InputTag)
{
	UE_LOG(LogP1, Log, TEXT("[Input] AbilityInputPressed: %s"), *InputTag.ToString());

	AP1CharacterBase* P1Character = GetPawn<AP1CharacterBase>();
	if (!P1Character)
	{
		UE_LOG(LogP1, Warning, TEXT("[Input] AbilityInputPressed: Pawn is null"));
		return;
	}

	UP1AbilitySystemComponent* ASC = Cast<UP1AbilitySystemComponent>(P1Character->GetAbilitySystemComponent());
	if (!ASC)
	{
		UE_LOG(LogP1, Warning, TEXT("[Input] AbilityInputPressed: ASC is null or not UP1AbilitySystemComponent"));
		return;
	}

	ASC->AbilityInputTagPressed(InputTag);
}

void AP1PlayerController::HandleAbilityInputReleased(FGameplayTag InputTag)
{
	if (AP1CharacterBase* P1Character = GetPawn<AP1CharacterBase>())
	{
		if (UP1AbilitySystemComponent* ASC = Cast<UP1AbilitySystemComponent>(P1Character->GetAbilitySystemComponent()))
		{
			ASC->AbilityInputTagReleased(InputTag);
		}
	}
}
