// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "P1BotLobbyComponent.generated.h"

class UP1BackendSubsystem;

// 부하테스트용 헤드리스 봇 전용 컴포넌트 — PreGame 단계(로그인→히어로 선택→매칭 큐 참가)를 UI 없이
// 순수 코드로 자동 진행한다. AP1LobbyPlayerController 생성자에 이 컴포넌트를 붙이기만 하면 되고,
// PC 클래스 본문엔 봇 관련 코드가 전혀 없다 — 커맨드라인에 "-BotId=N"이 없으면 BeginPlay()에서
// 아무것도 안 하고 조용히 끝나므로 일반 플레이어에게는 완전히 무관하다.
//
// UP1BackendSubsystem의 위젯과 완전히 분리된 공개 API(Login/Signup/SetSelectedHeroId/JoinQueue +
// OnLoginComplete/OnSignupComplete/OnMatchFound 델리게이트)를 그대로 재사용 — UI를 전혀 안 거친다.
UCLASS(ClassGroup = (Bot), meta = (BlueprintSpawnableComponent))
class P1_API UP1BotLobbyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UP1BotLobbyComponent();

protected:
	virtual void BeginPlay() override;

private:
	// BotEmail/BotPassword로 결정론적 계정 하나를 로그인 시도 → 실패하면(계정 없음, 최초 1회만)
	// 회원가입 후 재로그인 → 히어로 고정(Greystone) 선택 → 매칭 큐 참가.
	void BeginBotLogin(int32 BotId);

	UFUNCTION()
	void HandleBotLoginComplete(bool bSuccess, FString ErrorMessage);
	UFUNCTION()
	void HandleBotSignupComplete(bool bSuccess, FString ErrorMessage);
	UFUNCTION()
	void HandleBotMatchFound(FString ServerAddress);

	UP1BackendSubsystem* GetBackendSubsystem() const;

	FString BotEmail;
	FString BotUsername;
	FString BotPassword;
};
