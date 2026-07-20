// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/P1PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/P1PlayerCameraManager.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Characters/P1CharacterBase.h"
#include "P1.h"

AP1PlayerController::AP1PlayerController()
{
	PlayerCameraManagerClass = AP1PlayerCameraManager::StaticClass();
}

void AP1PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// PreGame 로비(AP1LobbyPlayerController)가 FInputModeUIOnly + bShowMouseCursor=true로 전환해두는데,
	// 이건 PlayerController 프로퍼티가 아니라 GameViewportClient의 마우스 캡처 모드를 바꾸는 것이라
	// ClientTravel로 완전히 새 PlayerController가 생성돼도(뷰포트 자체는 재생성되지 않으므로) 그대로
	// 남아있을 수 있다. Arena는 게임플레이 입력이 필요하므로 명시적으로 게임 입력 모드로 되돌린다.
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
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

	// 이동은 어빌리티가 아니라 Enhanced Input이 직접 처리하는 경로라 베이스 어빌리티의
	// ActivationBlockedTags(State.Stunned 포함)를 안 거친다 — 여기서 직접 태그를 체크해야 막힌다.
	if (const AP1CharacterBase* P1Character = Cast<AP1CharacterBase>(ControlledCharacter))
	{
		if (const UAbilitySystemComponent* ASC = P1Character->GetAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(TAG_State_Stunned))
			{
				return;
			}
		}
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
	ACharacter* ControlledCharacter = GetCharacter();
	if (!ControlledCharacter)
	{
		return;
	}

	if (const AP1CharacterBase* P1Character = Cast<AP1CharacterBase>(ControlledCharacter))
	{
		if (const UAbilitySystemComponent* ASC = P1Character->GetAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(TAG_State_Stunned))
			{
				return;
			}
		}
	}

	ControlledCharacter->Jump();
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

	// Ctrl+스킬키 = 그 어빌리티에 스킬 포인트 투자(마우스가 게임 중엔 캡처돼 있어 UI 버튼 클릭이
	// 불가능하므로, 상점을 열었을 때만 쓰는 마우스 UI 모드 대신 키보드로 처리한다). 평소(Ctrl 없이)는
	// 기존과 동일하게 발동 시도.
	if (IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl))
	{
		ASC->ServerInvestSkillPoint(InputTag);
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
