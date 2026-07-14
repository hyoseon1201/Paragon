// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1LobbyWidget.h"
#include "Online/P1BackendSubsystem.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "P1.h"

void UP1LobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoginButton)
	{
		LoginButton->OnClicked.AddDynamic(this, &UP1LobbyWidget::HandleLoginClicked);
	}
	if (SignupButton)
	{
		SignupButton->OnClicked.AddDynamic(this, &UP1LobbyWidget::HandleSignupClicked);
	}
	if (QueueButton)
	{
		QueueButton->OnClicked.AddDynamic(this, &UP1LobbyWidget::HandleQueueClicked);
	}

	if (UP1BackendSubsystem* Backend = GetBackendSubsystem())
	{
		Backend->OnSignupComplete.AddDynamic(this, &UP1LobbyWidget::HandleSignupComplete);
		Backend->OnLoginComplete.AddDynamic(this, &UP1LobbyWidget::HandleLoginComplete);
		Backend->OnQueueJoined.AddDynamic(this, &UP1LobbyWidget::HandleQueueJoined);
		Backend->OnMatchFound.AddDynamic(this, &UP1LobbyWidget::HandleMatchFound);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[Lobby] UP1BackendSubsystem을 찾을 수 없습니다"));
	}

	if (LoginPanel)
	{
		LoginPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (QueuePanel)
	{
		QueuePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UP1LobbyWidget::HandleLoginClicked()
{
	UP1BackendSubsystem* Backend = GetBackendSubsystem();
	if (!Backend || !UsernameBox || !PasswordBox)
	{
		return;
	}

	SetStatusText(TEXT("로그인 중..."));
	Backend->Login(UsernameBox->GetText().ToString(), PasswordBox->GetText().ToString());
}

void UP1LobbyWidget::HandleSignupClicked()
{
	UP1BackendSubsystem* Backend = GetBackendSubsystem();
	if (!Backend || !UsernameBox || !PasswordBox)
	{
		return;
	}

	SetStatusText(TEXT("회원가입 중..."));
	Backend->Signup(UsernameBox->GetText().ToString(), PasswordBox->GetText().ToString());
}

void UP1LobbyWidget::HandleQueueClicked()
{
	if (UP1BackendSubsystem* Backend = GetBackendSubsystem())
	{
		SetStatusText(TEXT("매칭 대기열 참가 중..."));
		Backend->JoinQueue();
	}
}

void UP1LobbyWidget::HandleSignupComplete(bool bSuccess, FString ErrorMessage)
{
	SetStatusText(bSuccess
		? TEXT("회원가입 완료 — 로그인해주세요.")
		: FString::Printf(TEXT("회원가입 실패: %s"), *ErrorMessage));
}

void UP1LobbyWidget::HandleLoginComplete(bool bSuccess, FString ErrorMessage)
{
	if (!bSuccess)
	{
		SetStatusText(FString::Printf(TEXT("로그인 실패: %s"), *ErrorMessage));
		return;
	}

	SetStatusText(TEXT("로그인 성공. 매칭을 시작하세요."));
	if (LoginPanel)
	{
		LoginPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (QueuePanel)
	{
		QueuePanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void UP1LobbyWidget::HandleQueueJoined(bool bSuccess, FString ErrorMessage)
{
	if (!bSuccess)
	{
		SetStatusText(FString::Printf(TEXT("매칭 대기열 참가 실패: %s"), *ErrorMessage));
		return;
	}

	SetStatusText(TEXT("매칭 대기 중..."));
}

void UP1LobbyWidget::HandleMatchFound(FString ServerAddress)
{
	SetStatusText(FString::Printf(TEXT("매칭 완료! %s 접속 중..."), *ServerAddress));

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->ClientTravel(ServerAddress, ETravelType::TRAVEL_Absolute);
	}
}

UP1BackendSubsystem* UP1LobbyWidget::GetBackendSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UP1BackendSubsystem>() : nullptr;
}

void UP1LobbyWidget::SetStatusText(const FString& Message) const
{
	UE_LOG(LogP1, Log, TEXT("[Lobby] %s"), *Message);
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
	}
}
