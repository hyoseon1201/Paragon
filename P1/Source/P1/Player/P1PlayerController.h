// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "P1PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AP1DamageNumberActor;

UCLASS()
class P1_API AP1PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AP1PlayerController();

	// 클라이언트가 접속 시 선택한 캐릭터 클래스.
	// TODO: AP1ArenaGameMode::Login()에서 URL Options를 파싱해 설정.
	//       현재는 null → GetDefaultPawnClassForController가 GameMode 기본값을 사용.
	UPROPERTY()
	TSubclassOf<APawn> SelectedCharacterClass;

protected:
	virtual void BeginPlay() override;
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

public:
	// 이 컨트롤러가 입힌 데미지만 화면에 표시(LoL/Dota 컨벤션 — 남이 입힌 데미지는 안 보임) —
	// UP1AttributeSet::PostGameplayEffectExecute가 가해자(Instigator)의 컨트롤러에만 이 RPC를 보낸다.
	// 로컬(이 클라이언트)에서만 액터를 스폰하므로 다른 클라이언트에는 전혀 리플리케이트되지 않는다.
	UFUNCTION(Client, Reliable)
	void ClientShowDamageNumber(FVector WorldLocation, float DamageAmount, bool bIsMagicalDamage);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<AP1DamageNumberActor> DamageNumberActorClass;
};
