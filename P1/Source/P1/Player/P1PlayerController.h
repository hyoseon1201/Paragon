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
class UP1ScoreboardWidget;

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

	// Tab을 누르고 있는 동안만 점수판을 띄운다(홀드, 토글 아님) — JumpAction과 동일하게 Started/Completed
	// 페어로 바인딩.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ScoreboardAction;

	// 상점은 마우스로 클릭해야 하니 홀드가 아니라 토글(Started만 바인딩, 누를 때마다 열림↔닫힘 전환).
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ShopAction;

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted(const FInputActionValue& Value);
	void HandleJumpCompleted(const FInputActionValue& Value);
	void HandleScoreboardShow(const FInputActionValue& Value);
	void HandleScoreboardHide(const FInputActionValue& Value);
	void HandleToggleShop(const FInputActionValue& Value);
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

	// 상점을 닫고 입력모드/마우스 커서를 게임 모드로 되돌린다 — ShopAction 토글(HandleToggleShop)과
	// WBP_Shop의 CloseButton(UP1ShopWidget::HandleCloseClicked) 양쪽이 공유하는 진입점. 위젯이 자기
	// Visibility만 Collapsed로 바꾸고 끝내면(예전 버그) 마우스 커서가 안 사라지고 GameAndUI 입력모드에
	// 그대로 남는다 — 반드시 이 함수를 통해서 닫을 것.
	void CloseShop();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<AP1DamageNumberActor> DamageNumberActorClass;

	// WBP_Scoreboard(parent=UP1ScoreboardWidget) 지정.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UP1ScoreboardWidget> ScoreboardWidgetClass;

private:
	// 처음 Tab을 누를 때 지연 생성 — 뷰포트에 올라간 채로 Visibility만 토글한다(홀드마다 새로 만들지
	// 않음). Collapsed 상태로 시작해 첫 표시 전까지 화면을 가리지 않는다.
	UPROPERTY()
	TObjectPtr<UP1ScoreboardWidget> ScoreboardWidgetInstance;
};
