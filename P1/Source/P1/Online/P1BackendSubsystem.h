// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "P1BackendSubsystem.generated.h"

// 로그인/회원가입 결과 — 성공 시 ErrorMessage는 비워둠.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAuthCompleteSignature, bool, bSuccess, FString, ErrorMessage);

// 매칭 대기열 참가 요청 자체의 성공/실패(네트워크 에러 등) — 매칭 성사 여부와는 별개.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQueueJoinedSignature, bool, bSuccess, FString, ErrorMessage);

// 매칭 성사 — ServerAddress로 ClientTravel하면 됨.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchFoundSignature, FString, ServerAddress);

// PreGame(로비) 레벨에서 웹 백엔드(Spring Boot)와 통신하는 창구. 로그인/회원가입/매칭 대기열 HTTP 호출과
// JWT 토큰 보관, 매칭 상태 폴링을 전담한다. GameInstance 생존주기 동안 유지되므로 ClientTravel로 Arena
// 서버에 접속한 뒤에도(향후 매치 결과 보고 등에 재사용 가능하도록) 살아있다.
UCLASS(Config = Game)
class P1_API UP1BackendSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "Backend")
	FOnAuthCompleteSignature OnSignupComplete;

	UPROPERTY(BlueprintAssignable, Category = "Backend")
	FOnAuthCompleteSignature OnLoginComplete;

	UPROPERTY(BlueprintAssignable, Category = "Backend")
	FOnQueueJoinedSignature OnQueueJoined;

	UPROPERTY(BlueprintAssignable, Category = "Backend")
	FOnMatchFoundSignature OnMatchFound;

	UFUNCTION(BlueprintCallable, Category = "Backend")
	void Signup(const FString& Username, const FString& Password);

	// 성공 시 JWT를 내부에 저장하고 OnLoginComplete를 브로드캐스트한다.
	UFUNCTION(BlueprintCallable, Category = "Backend")
	void Login(const FString& Username, const FString& Password);

	// 로그인 후 호출. 큐 참가 응답이 바로 MATCHED면 즉시 OnMatchFound도 브로드캐스트하고,
	// WAITING이면 내부 타이머로 /api/match/status를 주기적으로 폴링하기 시작한다.
	UFUNCTION(BlueprintCallable, Category = "Backend")
	void JoinQueue();

	UFUNCTION(BlueprintCallable, Category = "Backend")
	bool IsLoggedIn() const { return !AuthToken.IsEmpty(); }

private:
	// 배포 시 재컴파일 없이 주소만 바꿀 수 있도록 DefaultGame.ini에서 읽는다.
	// [/Script/P1.P1BackendSubsystem] BackendBaseUrl=http://127.0.0.1:8080
	UPROPERTY(Config)
	FString BackendBaseUrl = TEXT("http://127.0.0.1:8080");

	UPROPERTY(Config)
	float MatchStatusPollIntervalSeconds = 2.0f;

	FString AuthToken;

	FTimerHandle MatchStatusPollTimerHandle;

	TSharedRef<IHttpRequest> CreateRequest(const FString& Endpoint, const FString& Verb, bool bAuthorized) const;

	void OnSignupResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnQueueResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnStatusResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	// {status, serverAddress} 응답 바디를 공용으로 처리 — MATCHED면 폴링을 멈추고 OnMatchFound 브로드캐스트.
	// 반환값은 이번 응답이 MATCHED였는지 여부(호출부가 폴링 시작 여부를 판단하는 데 사용).
	bool HandleMatchStatusPayload(const FString& JsonPayload);

	void PollMatchStatus();
	void StartPolling();
	void StopPolling();
};
