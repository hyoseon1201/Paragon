// Copyright Epic Games, Inc. All Rights Reserved.

#include "Online/P1BackendSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "P1.h"

namespace
{
	// 백엔드 에러 응답({"code": "..."})에서 코드를 뽑아낸다. 응답 자체가 없거나(네트워크 에러)
	// 예상한 형태가 아니면 HTTP 상태코드 기반 폴백 메시지를 만든다.
	FString ExtractErrorCode(const FHttpResponsePtr& Response)
	{
		if (!Response.IsValid())
		{
			return TEXT("NETWORK_ERROR");
		}

		TSharedPtr<FJsonObject> JsonObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			FString Code;
			if (JsonObject->TryGetStringField(TEXT("code"), Code))
			{
				return Code;
			}
		}

		return FString::Printf(TEXT("HTTP_%d"), Response->GetResponseCode());
	}
}

void UP1BackendSubsystem::Deinitialize()
{
	StopPolling();
	Super::Deinitialize();
}

TSharedRef<IHttpRequest> UP1BackendSubsystem::CreateRequest(const FString& Endpoint, const FString& Verb, bool bAuthorized) const
{
	const TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BackendBaseUrl + Endpoint);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (bAuthorized)
	{
		Request->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + AuthToken);
	}
	return Request;
}

void UP1BackendSubsystem::Signup(const FString& Username, const FString& Password)
{
	const TSharedRef<IHttpRequest> Request = CreateRequest(TEXT("/api/auth/signup"), TEXT("POST"), false);

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("username"), Username);
	Body->SetStringField(TEXT("password"), Password);

	FString BodyString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body, Writer);
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindUObject(this, &UP1BackendSubsystem::OnSignupResponseReceived);
	Request->ProcessRequest();
}

void UP1BackendSubsystem::Login(const FString& Username, const FString& Password)
{
	const TSharedRef<IHttpRequest> Request = CreateRequest(TEXT("/api/auth/login"), TEXT("POST"), false);

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("username"), Username);
	Body->SetStringField(TEXT("password"), Password);

	FString BodyString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body, Writer);
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindUObject(this, &UP1BackendSubsystem::OnLoginResponseReceived);
	Request->ProcessRequest();
}

void UP1BackendSubsystem::JoinQueue()
{
	if (AuthToken.IsEmpty())
	{
		UE_LOG(LogP1, Warning, TEXT("[Backend] JoinQueue 호출됐지만 로그인 상태가 아님"));
		OnQueueJoined.Broadcast(false, TEXT("NOT_LOGGED_IN"));
		return;
	}

	const TSharedRef<IHttpRequest> Request = CreateRequest(TEXT("/api/match/queue"), TEXT("POST"), true);
	Request->OnProcessRequestComplete().BindUObject(this, &UP1BackendSubsystem::OnQueueResponseReceived);
	Request->ProcessRequest();
}

void UP1BackendSubsystem::OnSignupResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (bConnectedSuccessfully && Response.IsValid() && Response->GetResponseCode() == 201)
	{
		UE_LOG(LogP1, Log, TEXT("[Backend] Signup 성공"));
		OnSignupComplete.Broadcast(true, FString());
		return;
	}

	const FString ErrorCode = ExtractErrorCode(Response);
	UE_LOG(LogP1, Warning, TEXT("[Backend] Signup 실패 — %s"), *ErrorCode);
	OnSignupComplete.Broadcast(false, ErrorCode);
}

void UP1BackendSubsystem::OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (bConnectedSuccessfully && Response.IsValid() && Response->GetResponseCode() == 200)
	{
		TSharedPtr<FJsonObject> JsonObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		FString Token;
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid()
			&& JsonObject->TryGetStringField(TEXT("token"), Token) && !Token.IsEmpty())
		{
			AuthToken = Token;
			UE_LOG(LogP1, Log, TEXT("[Backend] Login 성공"));
			OnLoginComplete.Broadcast(true, FString());
			return;
		}
	}

	const FString ErrorCode = ExtractErrorCode(Response);
	UE_LOG(LogP1, Warning, TEXT("[Backend] Login 실패 — %s"), *ErrorCode);
	OnLoginComplete.Broadcast(false, ErrorCode);
}

void UP1BackendSubsystem::OnQueueResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid() || Response->GetResponseCode() != 200)
	{
		const FString ErrorCode = ExtractErrorCode(Response);
		UE_LOG(LogP1, Warning, TEXT("[Backend] JoinQueue 실패 — %s"), *ErrorCode);
		OnQueueJoined.Broadcast(false, ErrorCode);
		return;
	}

	OnQueueJoined.Broadcast(true, FString());

	const bool bMatched = HandleMatchStatusPayload(Response->GetContentAsString());
	if (!bMatched)
	{
		StartPolling();
	}
}

void UP1BackendSubsystem::OnStatusResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid() || Response->GetResponseCode() != 200)
	{
		UE_LOG(LogP1, Warning, TEXT("[Backend] 매칭 상태 폴링 실패 — %s"), *ExtractErrorCode(Response));
		return;
	}

	HandleMatchStatusPayload(Response->GetContentAsString());
}

bool UP1BackendSubsystem::HandleMatchStatusPayload(const FString& JsonPayload)
{
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonPayload);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogP1, Warning, TEXT("[Backend] 매칭 상태 응답 파싱 실패: %s"), *JsonPayload);
		return false;
	}

	FString Status;
	JsonObject->TryGetStringField(TEXT("status"), Status);

	if (Status == TEXT("MATCHED"))
	{
		FString ServerAddress;
		JsonObject->TryGetStringField(TEXT("serverAddress"), ServerAddress);
		StopPolling();
		UE_LOG(LogP1, Log, TEXT("[Backend] 매칭 성사 — ServerAddress=%s"), *ServerAddress);
		OnMatchFound.Broadcast(ServerAddress);
		return true;
	}

	UE_LOG(LogP1, Log, TEXT("[Backend] 매칭 상태=%s (대기 중)"), *Status);
	return false;
}

void UP1BackendSubsystem::PollMatchStatus()
{
	const TSharedRef<IHttpRequest> Request = CreateRequest(TEXT("/api/match/status"), TEXT("GET"), true);
	Request->OnProcessRequestComplete().BindUObject(this, &UP1BackendSubsystem::OnStatusResponseReceived);
	Request->ProcessRequest();
}

void UP1BackendSubsystem::StartPolling()
{
	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		World->GetTimerManager().SetTimer(MatchStatusPollTimerHandle, this, &UP1BackendSubsystem::PollMatchStatus, MatchStatusPollIntervalSeconds, true);
	}
}

void UP1BackendSubsystem::StopPolling()
{
	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(MatchStatusPollTimerHandle);
	}
}
