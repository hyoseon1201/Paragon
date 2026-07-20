// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/P1SignupWidget.h"
#include "Online/P1BackendSubsystem.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"

void UP1SignupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SignupButton)
	{
		SignupButton->OnClicked.AddDynamic(this, &UP1SignupWidget::HandleSignupClicked);
	}
	if (GoToLoginButton)
	{
		GoToLoginButton->OnClicked.AddDynamic(this, &UP1SignupWidget::HandleGoToLoginClicked);
	}
}

void UP1SignupWidget::HandleSignupClicked()
{
	if (UP1BackendSubsystem* Backend = GetBackendSubsystem())
	{
		if (UsernameBox && PasswordBox)
		{
			Backend->Signup(UsernameBox->GetText().ToString(), PasswordBox->GetText().ToString());
		}
	}
}

void UP1SignupWidget::HandleGoToLoginClicked()
{
	OnRequestLoginScreen.Broadcast();
}

UP1BackendSubsystem* UP1SignupWidget::GetBackendSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UP1BackendSubsystem>() : nullptr;
}
