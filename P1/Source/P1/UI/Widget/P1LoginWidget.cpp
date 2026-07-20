// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1LoginWidget.h"
#include "Online/P1BackendSubsystem.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"

void UP1LoginWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoginButton)
	{
		LoginButton->OnClicked.AddDynamic(this, &UP1LoginWidget::HandleLoginClicked);
	}
	if (GoToSignupButton)
	{
		GoToSignupButton->OnClicked.AddDynamic(this, &UP1LoginWidget::HandleGoToSignupClicked);
	}
}

void UP1LoginWidget::HandleLoginClicked()
{
	if (UP1BackendSubsystem* Backend = GetBackendSubsystem())
	{
		if (UsernameBox && PasswordBox)
		{
			Backend->Login(UsernameBox->GetText().ToString(), PasswordBox->GetText().ToString());
		}
	}
}

void UP1LoginWidget::HandleGoToSignupClicked()
{
	OnRequestSignupScreen.Broadcast();
}

UP1BackendSubsystem* UP1LoginWidget::GetBackendSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UP1BackendSubsystem>() : nullptr;
}
