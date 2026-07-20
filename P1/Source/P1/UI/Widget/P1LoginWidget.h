// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1LoginWidget.generated.h"

class UEditableTextBox;
class UButton;
class UP1BackendSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequestSignupScreen);

// PreGame Login 화면 전용 위젯. 자기 입력 필드와 로그인 요청 발신만 안다 — 실제 화면 전환/상태 메시지
// 표시는 이 위젯을 소유한 UP1PreGameHUDWidget이 UP1BackendSubsystem 결과 델리게이트를 직접 구독해서
// 처리한다. "회원가입 화면으로 가고 싶다"는 의사만 OnRequestSignupScreen으로 HUD에 알린다.
UCLASS()
class P1_API UP1LoginWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> UsernameBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> PasswordBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LoginButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> GoToSignupButton;

public:
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnRequestSignupScreen OnRequestSignupScreen;

private:
	UFUNCTION()
	void HandleLoginClicked();
	UFUNCTION()
	void HandleGoToSignupClicked();

	UP1BackendSubsystem* GetBackendSubsystem() const;
};
