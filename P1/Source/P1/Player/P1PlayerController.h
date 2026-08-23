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
class UP1MatchResultWidget;
class UP1BotArenaComponent;

UCLASS()
class P1_API AP1PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AP1PlayerController();

	// 클라이언트가 PreGame에서 선택한 히어로의 Pawn 클래스 — AP1ArenaGameMode::InitNewPlayer가 접속
	// URL의 "?HeroId=" 옵션을 HeroTable에서 조회해 채워준다. 비어있으면(옵션 없음/화이트리스트에 없는
	// 값) GetDefaultPawnClassForController_Implementation이 GameMode 기본값으로 폴백한다.
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

	// GameState->GetMatchState()==WaitingForPlayers인 동안 true — 매칭된 인원이 전부 접속하기 전까지는
	// 먼저 들어온 클라이언트가 이동/전투로 유리해지지 않도록 HandleMove/HandleJumpStarted/
	// HandleAbilityInputPressed 맨 앞에서 이 값을 체크해 조용히 무시한다. 지금은 순수 입력 무시뿐이고
	// (캐릭터 자체는 이미 스폰돼 화면엔 보임), 나중에 이 상태 동안 로딩 화면 위젯을 띄우는 UI 작업이
	// 별도로 붙을 예정.
	bool IsMatchWaitingForPlayers() const;

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

	// AP1ArenaGameMode::EndMatch()가 매치 종료 시 전원에게 호출 — 결과창을 띄우고 입력모드를 UI 전용으로
	// 바꾼 뒤, ResultScreenDurationSeconds 후 자동으로 LobbyMapPath로 ClientTravel한다. Duration/맵 경로를
	// GameMode가 RPC 파라미터로 실어 보내는 이유는 그 값을 GameMode 한 곳에서만 관리하기 위함
	// (PlayerController가 별도로 같은 값을 EditDefaultsOnly로 중복해서 안 들고 있어도 됨).
	UFUNCTION(Client, Reliable)
	void ClientShowMatchResult(int32 WinningTeamId, float ResultScreenDurationSeconds, const FString& LobbyMapPath);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<AP1DamageNumberActor> DamageNumberActorClass;

	// WBP_Scoreboard(parent=UP1ScoreboardWidget) 지정.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UP1ScoreboardWidget> ScoreboardWidgetClass;

	// WBP_MatchResult(parent=UP1MatchResultWidget) 지정.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UP1MatchResultWidget> MatchResultWidgetClass;

private:
	// 처음 Tab을 누를 때 지연 생성 — 뷰포트에 올라간 채로 Visibility만 토글한다(홀드마다 새로 만들지
	// 않음). Collapsed 상태로 시작해 첫 표시 전까지 화면을 가리지 않는다.
	UPROPERTY()
	TObjectPtr<UP1ScoreboardWidget> ScoreboardWidgetInstance;

	// 매치당 한 번만 뜨는 화면이라 Scoreboard처럼 재사용 인스턴스를 캐싱해두지 않는다 — 생성 즉시 표시.
	UPROPERTY()
	TObjectPtr<UP1MatchResultWidget> MatchResultWidgetInstance;

	FTimerHandle MatchResultReturnTimerHandle;

	// ClientShowMatchResult()가 예약한 타이머가 만료되면 호출 — 로컬(데디케이티드 서버가 없는) PreGame
	// 맵으로 개별 ClientTravel. IP:Port가 없는 순수 맵 경로이므로 각 클라이언트가 독립적으로 Arena
	// 서버 접속을 끊고 로컬 레벨을 로드한다.
	void ReturnToLobby(FString LobbyMapPath);

	// 부하테스트용 헤드리스 봇 전용 — 실제 순찰/전투 로직은 전부 이 컴포넌트 안에 있다(이 클래스는
	// 생성자에서 붙이기만 함). 커맨드라인에 "-BotId=N"이 없으면 컴포넌트가 스스로 아무 일도 안 하므로
	// 일반 플레이어에게는 무관 — 자세한 내용은 P1BotArenaComponent.h 참고.
	UPROPERTY(VisibleAnywhere, Category = "Bot")
	TObjectPtr<UP1BotArenaComponent> BotArenaComponent;
};
