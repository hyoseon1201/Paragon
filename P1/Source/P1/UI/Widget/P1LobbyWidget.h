// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1LobbyWidget.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;
class UWidget;
class UP1BackendSubsystem;

// PreGame(로비) 레벨의 로그인+매칭 대기 UI. UP1UserWidget(위젯 컨트롤러+ASC 패턴)을 상속하지 않는다 —
// 이 위젯의 데이터 소스는 GAS/ASC가 아니라 UP1BackendSubsystem(웹 백엔드 HTTP 통신)이라
// 위젯 컨트롤러 패턴 자체가 안 맞기 때문에 의도적으로 순수 UUserWidget을 직접 상속한다.
UCLASS()
class P1_API UP1LobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// ---- 로그인 패널 ----
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> UsernameBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> PasswordBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LoginButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SignupButton;

	// ---- 대기열 패널 ----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QueueButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	// 로그인 성공 시 LoginPanel을 숨기고 QueuePanel을 보이게 전환한다. 컨테이너 타입을 가리지 않도록
	// (Border/Overlay/VerticalBox 등 WBP 디자이너 자유) 베이스 UWidget으로 바인딩.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> LoginPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> QueuePanel;

private:
	UFUNCTION()
	void HandleLoginClicked();
	UFUNCTION()
	void HandleSignupClicked();
	UFUNCTION()
	void HandleQueueClicked();

	UFUNCTION()
	void HandleSignupComplete(bool bSuccess, FString ErrorMessage);
	UFUNCTION()
	void HandleLoginComplete(bool bSuccess, FString ErrorMessage);
	UFUNCTION()
	void HandleQueueJoined(bool bSuccess, FString ErrorMessage);
	UFUNCTION()
	void HandleMatchFound(FString ServerAddress);

	UP1BackendSubsystem* GetBackendSubsystem() const;

	void SetStatusText(const FString& Message) const;
};
