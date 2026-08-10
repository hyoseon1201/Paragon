// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1PreGameHUDWidget.generated.h"

class UP1LoginWidget;
class UP1SignupWidget;
class UP1MatchQueueWidget;
class UTextBlock;
class UP1BackendSubsystem;

// PreGame(로비) 화면 3종 — 한 번에 하나만 보인다(ShowScreen()이 나머지를 Collapsed 처리).
enum class EP1LobbyScreen : uint8
{
	Login,
	Signup,
	MatchQueue
};

// PreGame(로비) 레벨의 최상위 HUD 위젯. UP1UserWidget(위젯 컨트롤러+ASC 패턴)을 상속하지 않는다 —
// 데이터 소스가 GAS/ASC가 아니라 UP1BackendSubsystem(HTTP)이라 위젯 컨트롤러 패턴 자체가 안 맞다.
//
// Login/Signup/MatchQueue를 각각 독립된 위젯 클래스(UP1LoginWidget/UP1SignupWidget/UP1MatchQueueWidget)로
// 분리하고, 이 HUD는 셋을 멤버로 들고 있으면서 "어느 화면을 보여줄지"만 오케스트레이션한다 —
// 각 자식은 자기 입력 필드+요청 발신만 알고, 결과 처리(상태 메시지/화면 전환/매칭 성사 시 ClientTravel)는
// 전부 이 HUD가 UP1BackendSubsystem 델리게이트를 직접 구독해서 처리한다. 화면 간 이동 요청
// (예: Login→Signup)은 자식이 쏘는 네비게이션 델리게이트(OnRequestSignupScreen 등)로 받는다.
UCLASS()
class P1_API UP1PreGameHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1LoginWidget> LoginWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1SignupWidget> SignupWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1MatchQueueWidget> MatchQueueWidget;

	// 3개 화면 공용 상태 메시지 — WBP에서 세 자식 위젯 바깥(항상 보이는 자리)에 배치할 것.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

private:
	UFUNCTION()
	void HandleRequestSignupScreen();
	UFUNCTION()
	void HandleRequestLoginScreen();

	UFUNCTION()
	void HandleSignupComplete(bool bSuccess, FString ErrorMessage);
	UFUNCTION()
	void HandleLoginComplete(bool bSuccess, FString ErrorMessage);
	UFUNCTION()
	void HandleQueueJoined(bool bSuccess, FString ErrorMessage);
	UFUNCTION()
	void HandleMatchFound(FString ServerAddress);

	UP1BackendSubsystem* GetBackendSubsystem() const;

	void ShowScreen(EP1LobbyScreen Screen) const;
	void SetStatusText(const FString& Message) const;
};
