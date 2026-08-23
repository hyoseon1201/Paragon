// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/P1BotLobbyComponent.h"
#include "Online/P1BackendSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Misc/CommandLine.h"
#include "P1.h"

UP1BotLobbyComponent::UP1BotLobbyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UP1BotLobbyComponent::BeginPlay()
{
	Super::BeginPlay();

	const APlayerController* OwningPC = GetOwner<APlayerController>();
	if (!OwningPC || !OwningPC->IsLocalController())
	{
		return;
	}

	int32 BotId = -1;
	if (!FParse::Value(FCommandLine::Get(), TEXT("BotId="), BotId))
	{
		return; // -BotId= 없으면 일반 플레이어 — 조용히 종료.
	}

	BeginBotLogin(BotId);
}

void UP1BotLobbyComponent::BeginBotLogin(int32 BotId)
{
	BotEmail = FString::Printf(TEXT("bot%d@p1test.local"), BotId);
	BotUsername = FString::Printf(TEXT("Bot%d"), BotId);
	BotPassword = TEXT("BotLoadTest123!");

	UP1BackendSubsystem* Backend = GetBackendSubsystem();
	if (!Backend)
	{
		UE_LOG(LogP1, Warning, TEXT("[Bot] UP1BackendSubsystem을 찾을 수 없음 — 봇 로그인 불가 (BotId=%d)"), BotId);
		return;
	}

	Backend->OnLoginComplete.AddDynamic(this, &UP1BotLobbyComponent::HandleBotLoginComplete);
	Backend->OnSignupComplete.AddDynamic(this, &UP1BotLobbyComponent::HandleBotSignupComplete);
	Backend->OnMatchFound.AddDynamic(this, &UP1BotLobbyComponent::HandleBotMatchFound);

	UE_LOG(LogP1, Log, TEXT("[Bot] 로그인 시도 — %s"), *BotEmail);
	Backend->Login(BotEmail, BotPassword);
}

void UP1BotLobbyComponent::HandleBotLoginComplete(bool bSuccess, FString ErrorMessage)
{
	UP1BackendSubsystem* Backend = GetBackendSubsystem();
	if (!Backend)
	{
		return;
	}

	if (bSuccess)
	{
		UE_LOG(LogP1, Log, TEXT("[Bot] 로그인 성공 — %s, Greystone으로 큐 참가"), *BotEmail);
		Backend->SetSelectedHeroId(TEXT("Greystone"));
		Backend->JoinQueue();
		return;
	}

	// 계정이 아직 없어서 실패한 경우(최초 실행) — 회원가입 후 재로그인.
	UE_LOG(LogP1, Log, TEXT("[Bot] 로그인 실패(%s) — 회원가입 시도 (%s)"), *ErrorMessage, *BotEmail);
	Backend->Signup(BotEmail, BotUsername, BotPassword);
}

void UP1BotLobbyComponent::HandleBotSignupComplete(bool bSuccess, FString ErrorMessage)
{
	UP1BackendSubsystem* Backend = GetBackendSubsystem();
	if (!Backend)
	{
		return;
	}

	if (bSuccess)
	{
		Backend->Login(BotEmail, BotPassword);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[Bot] 회원가입도 실패(%s) — 봇 %s 포기(로비에 남음)"), *ErrorMessage, *BotEmail);
	}
}

void UP1BotLobbyComponent::HandleBotMatchFound(FString ServerAddress)
{
	APlayerController* OwningPC = GetOwner<APlayerController>();
	UP1BackendSubsystem* Backend = GetBackendSubsystem();
	if (!OwningPC || !Backend)
	{
		return;
	}

	// UP1PreGameHUDWidget::HandleMatchFound와 동일한 로직 — AP1ArenaGameMode::InitNewPlayer가
	// 이 URL 옵션으로 스폰 클래스를 결정한다.
	const FString HeroId = Backend->GetSelectedHeroId().ToString();
	const FString TravelURL = ServerAddress + TEXT("?HeroId=") + HeroId;
	UE_LOG(LogP1, Log, TEXT("[Bot] 매칭 완료 — %s로 이동 (%s)"), *TravelURL, *BotEmail);
	OwningPC->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
}

UP1BackendSubsystem* UP1BotLobbyComponent::GetBackendSubsystem() const
{
	const APlayerController* OwningPC = GetOwner<APlayerController>();
	const UGameInstance* GameInstance = OwningPC ? OwningPC->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UP1BackendSubsystem>() : nullptr;
}
