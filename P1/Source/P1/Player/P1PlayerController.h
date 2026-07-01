// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "P1PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AP1HUD;
class UAbilitySystemComponent;

UCLASS()
class P1_API AP1PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AP1PlayerController();

	// 소유 클라이언트(또는 리슨서버 로컬 플레이어)에서 HUD를 생성하고 ASC에 바인딩.
	// AP1HeroCharacter::InitAbilityActorInfo에서 IsLocallyControlled() 시 호출된다.
	void CreateHUDForASC(UAbilitySystemComponent* InASC);

	// 클라이언트가 접속 시 선택한 캐릭터 클래스.
	// TODO: AP1ArenaGameMode::Login()에서 URL Options를 파싱해 설정.
	//       현재는 null → GetDefaultPawnClassForController가 GameMode 기본값을 사용.
	UPROPERTY()
	TSubclassOf<APawn> SelectedCharacterClass;

protected:
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted(const FInputActionValue& Value);
	void HandleJumpCompleted(const FInputActionValue& Value);
	void HandleAbilityInputPressed(FGameplayTag InputTag);
	void HandleAbilityInputReleased(FGameplayTag InputTag);

	// IMC에 포함된 IA와 이에 대응하는 GAS InputTag를 쌍으로 등록.
	// 에디터에서 BasicAttackAction → InputTag.Ability.BasicAttack 식으로 설정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Input|Abilities")
	TMap<TObjectPtr<UInputAction>, FGameplayTag> AbilityInputActions;
};
