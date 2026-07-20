// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1SignupWidget.generated.h"

class UEditableTextBox;
class UButton;
class UP1BackendSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequestLoginScreen);

// PreGame Signup 화면 전용 위젯 — UP1LoginWidget과 대칭 구조. 자기 입력 필드와 회원가입 요청 발신만
// 안다. 결과 처리/화면 전환은 UP1PreGameHUDWidget이 담당하고, 이 위젯은 "로그인 화면으로 돌아가고
// 싶다"는 의사만 OnRequestLoginScreen으로 알린다.
UCLASS()
class P1_API UP1SignupWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> UsernameBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> PasswordBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SignupButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> GoToLoginButton;

public:
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnRequestLoginScreen OnRequestLoginScreen;

private:
	UFUNCTION()
	void HandleSignupClicked();
	UFUNCTION()
	void HandleGoToLoginClicked();

	UP1BackendSubsystem* GetBackendSubsystem() const;
};
