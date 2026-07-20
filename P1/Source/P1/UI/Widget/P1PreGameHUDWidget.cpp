// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1PreGameHUDWidget.h"
#include "UI/Widget/P1LoginWidget.h"
#include "UI/Widget/P1SignupWidget.h"
#include "UI/Widget/P1MatchQueueWidget.h"
#include "Online/P1BackendSubsystem.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "P1.h"

void UP1PreGameHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoginWidget)
	{
		LoginWidget->OnRequestSignupScreen.AddDynamic(this, &UP1PreGameHUDWidget::HandleRequestSignupScreen);
	}
	if (SignupWidget)
	{
		SignupWidget->OnRequestLoginScreen.AddDynamic(this, &UP1PreGameHUDWidget::HandleRequestLoginScreen);
	}

	if (UP1BackendSubsystem* Backend = GetBackendSubsystem())
	{
		Backend->OnSignupComplete.AddDynamic(this, &UP1PreGameHUDWidget::HandleSignupComplete);
		Backend->OnLoginComplete.AddDynamic(this, &UP1PreGameHUDWidget::HandleLoginComplete);
		Backend->OnQueueJoined.AddDynamic(this, &UP1PreGameHUDWidget::HandleQueueJoined);
		Backend->OnMatchFound.AddDynamic(this, &UP1PreGameHUDWidget::HandleMatchFound);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[Lobby] UP1BackendSubsystem을 찾을 수 없습니다"));
	}

	ShowScreen(EP1LobbyScreen::Login);
}

void UP1PreGameHUDWidget::HandleRequestSignupScreen()
{
	ShowScreen(EP1LobbyScreen::Signup);
}

void UP1PreGameHUDWidget::HandleRequestLoginScreen()
{
	ShowScreen(EP1LobbyScreen::Login);
}

void UP1PreGameHUDWidget::HandleSignupComplete(bool bSuccess, FString ErrorMessage)
{
	if (!bSuccess)
	{
		SetStatusText(FString::Printf(TEXT("회원가입 실패: %s"), *ErrorMessage));
		return;
	}

	SetStatusText(TEXT("회원가입 완료 — 로그인해주세요."));
	ShowScreen(EP1LobbyScreen::Login);
}

void UP1PreGameHUDWidget::HandleLoginComplete(bool bSuccess, FString ErrorMessage)
{
	if (!bSuccess)
	{
		SetStatusText(FString::Printf(TEXT("로그인 실패: %s"), *ErrorMessage));
		return;
	}

	SetStatusText(TEXT("로그인 성공. 매칭을 시작하세요."));
	ShowScreen(EP1LobbyScreen::MatchQueue);
}

void UP1PreGameHUDWidget::HandleQueueJoined(bool bSuccess, FString ErrorMessage)
{
	SetStatusText(bSuccess
		? TEXT("매칭 대기 중...")
		: FString::Printf(TEXT("매칭 대기열 참가 실패: %s"), *ErrorMessage));
}

void UP1PreGameHUDWidget::HandleMatchFound(FString ServerAddress)
{
	SetStatusText(FString::Printf(TEXT("매칭 완료! %s 접속 중..."), *ServerAddress));

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->ClientTravel(ServerAddress, ETravelType::TRAVEL_Absolute);
	}
}

UP1BackendSubsystem* UP1PreGameHUDWidget::GetBackendSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UP1BackendSubsystem>() : nullptr;
}

void UP1PreGameHUDWidget::ShowScreen(EP1LobbyScreen Screen) const
{
	if (LoginWidget)
	{
		LoginWidget->SetVisibility(Screen == EP1LobbyScreen::Login ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (SignupWidget)
	{
		SignupWidget->SetVisibility(Screen == EP1LobbyScreen::Signup ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (MatchQueueWidget)
	{
		MatchQueueWidget->SetVisibility(Screen == EP1LobbyScreen::MatchQueue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UP1PreGameHUDWidget::SetStatusText(const FString& Message) const
{
	UE_LOG(LogP1, Log, TEXT("[Lobby] %s"), *Message);
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
	}
}
