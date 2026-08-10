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
#include "UI/P1DamageNumberActor.h"
#include "UI/Widget/Scoreboard/P1ScoreboardWidget.h"
#include "UI/WidgetController/P1ScoreboardWidgetController.h"
#include "UI/Widget/Shop/P1ShopWidget.h"
#include "UI/HUD/P1HUD.h"
#include "Blueprint/UserWidget.h"
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
		if (ScoreboardAction)
		{
			EnhancedInputComponent->BindAction(ScoreboardAction, ETriggerEvent::Started, this, &AP1PlayerController::HandleScoreboardShow);
			EnhancedInputComponent->BindAction(ScoreboardAction, ETriggerEvent::Completed, this, &AP1PlayerController::HandleScoreboardHide);
			EnhancedInputComponent->BindAction(ScoreboardAction, ETriggerEvent::Canceled, this, &AP1PlayerController::HandleScoreboardHide);
		}
		if (ShopAction)
		{
			EnhancedInputComponent->BindAction(ShopAction, ETriggerEvent::Started, this, &AP1PlayerController::HandleToggleShop);
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
	// State.Rooted는 캐스팅 중 이동을 막고 싶은 어빌리티(Photon Disruptor 등)가 자기 ActivationOwnedTags에
	// 얹는 범용 태그 — 새 어빌리티가 이동 차단이 필요해도 이 함수를 다시 손댈 필요 없이 그 태그만 추가하면 된다.
	if (const AP1CharacterBase* P1Character = Cast<AP1CharacterBase>(ControlledCharacter))
	{
		if (const UAbilitySystemComponent* ASC = P1Character->GetAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(TAG_State_Stunned) || ASC->HasMatchingGameplayTag(TAG_State_Rooted))
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

	AP1CharacterBase* P1Character = Cast<AP1CharacterBase>(ControlledCharacter);
	UP1AbilitySystemComponent* ASC = P1Character ? Cast<UP1AbilitySystemComponent>(P1Character->GetAbilitySystemComponent()) : nullptr;
	if (ASC && ASC->HasMatchingGameplayTag(TAG_State_Stunned))
	{
		return;
	}

	// 이미 공중(낙하 중)이면 일반 점프 대신 공중 패시브(예: Dekker의 Rocket Boots)를 시도한다 —
	// InputTag.Ability.Passive를 가진 어빌리티가 없는 영웅은 AbilityInputTagPressed가 아무것도 못
	// 찾아 조용히 무시되므로(원래 IsFalling() 상태의 Jump() 재호출도 JumpMaxCount=1이라 아무 효과가
	// 없었던 것과 동일하게) 별도 폴백 없이 안전하다.
	if (ASC && ControlledCharacter->GetCharacterMovement() && ControlledCharacter->GetCharacterMovement()->IsFalling())
	{
		ASC->AbilityInputTagPressed(TAG_InputTag_Ability_Passive);
		return;
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

void AP1PlayerController::HandleScoreboardShow(const FInputActionValue& Value)
{
	if (!ScoreboardWidgetClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[Scoreboard] ScoreboardWidgetClass 미설정 — BP_P1PlayerController에서 지정하세요."));
		return;
	}

	// 처음 누를 때만 생성 — 이후로는 계속 재사용하며 Visibility만 토글한다.
	if (!ScoreboardWidgetInstance)
	{
		ScoreboardWidgetInstance = CreateWidget<UP1ScoreboardWidget>(this, ScoreboardWidgetClass);
		if (ScoreboardWidgetInstance)
		{
			// Scoreboard는 ASC가 필요 없어 AP1HUD 레지스트리를 타지 않지만, 컨트롤러 전파 패턴은
			// 상점과 통일한다 — 델리게이트 없이 GameState 접근만 제공하는 가벼운 컨트롤러.
			UP1ScoreboardWidgetController* Controller = NewObject<UP1ScoreboardWidgetController>(this);
			Controller->SetWidgetControllerParams(FWidgetControllerParams(this, PlayerState, nullptr, nullptr));
			ScoreboardWidgetInstance->SetWidgetController(Controller);

			ScoreboardWidgetInstance->AddToViewport();
		}
	}

	if (ScoreboardWidgetInstance)
	{
		// 매번 다시 그린다 — 홀드하는 순간의 최신 KDA/캐릭터 스냅샷을 보여줘야 하므로.
		ScoreboardWidgetInstance->RefreshScoreboard();
		ScoreboardWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void AP1PlayerController::HandleScoreboardHide(const FInputActionValue& Value)
{
	if (ScoreboardWidgetInstance)
	{
		ScoreboardWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AP1PlayerController::HandleToggleShop(const FInputActionValue& Value)
{
	// 상점 위젯은 스코어보드처럼 여기서 만들지 않는다 — AP1HUD::InitShop()이 HandleAbilitySystemReady
	// 시점에 이미 만들어 Collapsed 상태로 뷰포트에 올려뒀다(ASC/Gold/Inventory 델리게이트를 구독하는
	// 위젯 컨트롤러가 필요해서 Overlay와 같은 경로를 탄다 — 자세한 배경은 AP1HUD::InitShop() 참고).
	AP1HUD* P1HUD = GetHUD<AP1HUD>();
	UP1ShopWidget* ShopWidget = P1HUD ? P1HUD->GetShopWidget() : nullptr;
	if (!ShopWidget)
	{
		UE_LOG(LogP1, Warning, TEXT("[Shop] ShopWidget이 아직 없습니다 — AP1HUD::InitShop() 호출 여부 확인 필요"));
		return;
	}

	const bool bWasOpen = ShopWidget->GetVisibility() != ESlateVisibility::Collapsed;
	if (bWasOpen)
	{
		CloseShop();
	}
	else
	{
		ShopWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		// GameAndUI — WASD 이동은 그대로 살리고 마우스만 커서로 풀어서 Buy/Sell 버튼을 클릭할 수 있게.
		SetInputMode(FInputModeGameAndUI());
		bShowMouseCursor = true;
	}
}

void AP1PlayerController::CloseShop()
{
	AP1HUD* P1HUD = GetHUD<AP1HUD>();
	if (UP1ShopWidget* ShopWidget = P1HUD ? P1HUD->GetShopWidget() : nullptr)
	{
		ShopWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
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
	// Enhanced Input의 Completed 이벤트가 실제 키 릴리즈보다 먼저(예: IA 트리거 설정이 Down이 아니라
	// Pressed 등으로 잘못돼 있으면) 발화하는지 확인하기 위한 진단 로그 — HandleAbilityInputPressed는
	// 이미 찍고 있는데 Released 쪽엔 하나도 없어서 "홀드 중 어빌리티가 갑자기 끝남" 재현 시 이 로그가
	// Pressed 로그 직후 곧바로(같은 프레임/다음 틱) 찍히는지가 Enhanced Input 레이어 문제인지 판별 포인트.
	UE_LOG(LogP1, Log, TEXT("[Input] AbilityInputReleased: %s"), *InputTag.ToString());

	if (AP1CharacterBase* P1Character = GetPawn<AP1CharacterBase>())
	{
		if (UP1AbilitySystemComponent* ASC = Cast<UP1AbilitySystemComponent>(P1Character->GetAbilitySystemComponent()))
		{
			ASC->AbilityInputTagReleased(InputTag);
		}
	}
}

void AP1PlayerController::ClientShowDamageNumber_Implementation(FVector WorldLocation, float DamageAmount, bool bIsMagicalDamage)
{
	if (!DamageNumberActorClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[DamageNumber] DamageNumberActorClass 미설정 — BP_P1PlayerController에서 지정하세요."));
		return;
	}

	// 여러 데미지가 같은 프레임에 몰려도 겹쳐 보이지 않게 살짝 랜덤 오프셋을 준다.
	const FVector DamageNumberSpawnLocation = WorldLocation + FVector(
		FMath::FRandRange(-20.0f, 20.0f), FMath::FRandRange(-20.0f, 20.0f), 80.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AP1DamageNumberActor* DamageNumberActor = GetWorld()->SpawnActor<AP1DamageNumberActor>(
		DamageNumberActorClass, DamageNumberSpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (DamageNumberActor)
	{
		DamageNumberActor->InitializeDamageNumber(DamageAmount, bIsMagicalDamage);
	}
}
