// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1MatchQueueWidget.h"
#include "Online/P1BackendSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void UP1MatchQueueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (QueueButton)
	{
		QueueButton->OnClicked.AddDynamic(this, &UP1MatchQueueWidget::HandleQueueClicked);
	}

	if (UP1BackendSubsystem* Backend = GetBackendSubsystem())
	{
		Backend->OnQueueJoined.AddDynamic(this, &UP1MatchQueueWidget::HandleQueueJoined);
		Backend->OnQueueLeft.AddDynamic(this, &UP1MatchQueueWidget::HandleQueueLeft);
	}

	SetQueuedState(false);
}

void UP1MatchQueueWidget::HandleQueueClicked()
{
	UP1BackendSubsystem* Backend = GetBackendSubsystem();
	if (!Backend)
	{
		return;
	}

	if (bIsQueued)
	{
		Backend->LeaveQueue();
	}
	else
	{
		Backend->JoinQueue();
	}
}

void UP1MatchQueueWidget::HandleQueueJoined(bool bSuccess, FString ErrorMessage)
{
	if (bSuccess)
	{
		SetQueuedState(true);
	}
}

void UP1MatchQueueWidget::HandleQueueLeft(bool bSuccess, FString ErrorMessage)
{
	if (bSuccess)
	{
		SetQueuedState(false);
	}
}

UP1BackendSubsystem* UP1MatchQueueWidget::GetBackendSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UP1BackendSubsystem>() : nullptr;
}

void UP1MatchQueueWidget::SetQueuedState(bool bInIsQueued)
{
	bIsQueued = bInIsQueued;
	if (QueueButtonLabel)
	{
		QueueButtonLabel->SetText(FText::FromString(bIsQueued ? TEXT("취소") : TEXT("매칭 시작")));
	}
}
