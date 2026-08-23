// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/P1ArenaGameMode.h"
#include "Characters/P1HeroCharacter.h"
#include "Characters/P1HeroTypes.h"
#include "GameModes/P1GameState.h"
#include "Player/P1PlayerController.h"
#include "Player/P1PlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "P1.h"

AP1ArenaGameMode::AP1ArenaGameMode()
{
	DefaultPawnClass = AP1HeroCharacter::StaticClass();
	GameStateClass = AP1GameState::StaticClass();
}

void AP1ArenaGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (AP1GameState* P1GS = GetGameState<AP1GameState>())
	{
		P1GS->InitializeTeamScores(NumTeams);
		UE_LOG(LogP1, Log, TEXT("[ArenaGameMode] 레벨 로드 — 팀 %d개 점수 초기화, %d명 접속 대기 중(WaitingForPlayers) (%s)"),
			NumTeams, ExpectedPlayerCount, *GetName());
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[ArenaGameMode] BeginPlay — GetGameState<AP1GameState>()가 null, GameStateClass 설정 확인 필요"));
	}
}

void AP1ArenaGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AP1GameState* P1GS = GetGameState<AP1GameState>();
	if (!P1GS || P1GS->GetMatchState() != EP1MatchState::WaitingForPlayers)
	{
		return; // 이미 시작됐거나(InProgress/Ended) GameState가 없음 — 재시작 시도 불필요.
	}

	const int32 ConnectedCount = P1GS->PlayerArray.Num();
	UE_LOG(LogP1, Log, TEXT("[ArenaGameMode] PostLogin — %s 접속, %d/%d명"),
		*NewPlayer->GetName(), ConnectedCount, ExpectedPlayerCount);

	if (ConnectedCount >= ExpectedPlayerCount)
	{
		StartMatch();
	}
}

void AP1ArenaGameMode::StartMatch()
{
	AP1GameState* P1GS = GetGameState<AP1GameState>();
	if (!P1GS)
	{
		return;
	}

	P1GS->SetMatchStartTime();
	P1GS->SetMatchInProgress();

	UE_LOG(LogP1, Log, TEXT("[ArenaGameMode] 매치 시작 — 전원 접속 완료, ServerWorldTime=%.2f 기준으로 클럭 0초 고정 (%s)"),
		P1GS->GetServerWorldTimeSeconds(), *GetName());
}

void AP1ArenaGameMode::OnTeamKillScored(int32 TeamId)
{
	AP1GameState* P1GS = GetGameState<AP1GameState>();
	if (!P1GS || P1GS->GetMatchState() != EP1MatchState::InProgress)
	{
		return;
	}

	P1GS->AddTeamKill(TeamId);
	const int32 NewScore = P1GS->GetTeamKillScore(TeamId);

	UE_LOG(LogP1, Log, TEXT("[ArenaGameMode] Team %d 킬스코어 %d/%d"), TeamId, NewScore, KillScoreToWin);

	if (NewScore >= KillScoreToWin)
	{
		EndMatch(TeamId);
	}
}

void AP1ArenaGameMode::EndMatch(int32 WinningTeamId)
{
	AP1GameState* P1GS = GetGameState<AP1GameState>();
	if (!P1GS)
	{
		return;
	}

	P1GS->SetMatchEnded(WinningTeamId);

	UE_LOG(LogP1, Log, TEXT("[ArenaGameMode] 매치 종료 — 승리 팀=%d"), WinningTeamId);

	for (APlayerState* PS : P1GS->PlayerArray)
	{
		if (AP1PlayerController* P1PC = Cast<AP1PlayerController>(PS ? PS->GetOwner() : nullptr))
		{
			P1PC->ClientShowMatchResult(WinningTeamId, ResultScreenDurationSeconds, LobbyMapPath);
		}
	}
}

AActor* AP1ArenaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const APlayerController* PC = Cast<APlayerController>(Player);
	if (!PC)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	AP1PlayerState* PS = PC->GetPlayerState<AP1PlayerState>();
	if (!PS)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	// 팀이 아직 미배정(255)인 경우에만 새로 배정 — 리스폰 시에는 기존 팀 유지.
	// ChoosePlayerStart가 PostLogin보다 먼저 호출되므로 여기서 팀을 배정한다.
	uint8 TeamId = PS->GetGenericTeamId().GetId();
	if (TeamId == 255)
	{
		TeamId = static_cast<uint8>(NextTeamIndex % FMath::Max(1, NumTeams));
		PS->SetGenericTeamId(FGenericTeamId(TeamId));
		++NextTeamIndex;
	}

	const FName TargetTag = FName(*FString::Printf(TEXT("Team%d"), TeamId));

	// 같은 태그를 가진 PlayerStart를 전부 모아서 그중 하나를 랜덤으로 고른다 — 팀원 여러 명이
	// 전부 같은 자리 하나에 겹쳐 스폰되는 걸 막는다(팀당 PlayerStart를 여러 개 배치해뒀다는 전제).
	TArray<APlayerStart*> MatchingStarts;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if ((*It)->PlayerStartTag == TargetTag)
		{
			MatchingStarts.Add(*It);
		}
	}

	if (MatchingStarts.Num() > 0)
	{
		APlayerStart* Chosen = MatchingStarts[FMath::RandHelper(MatchingStarts.Num())];
		UE_LOG(LogP1, Log, TEXT("[ArenaGameMode] %s → Team %d → PlayerStart '%s' (태그 일치 %d개 중 랜덤 선택)"),
			*Player->GetName(), TeamId, *TargetTag.ToString(), MatchingStarts.Num());
		return Chosen;
	}

	UE_LOG(LogP1, Warning, TEXT("[ArenaGameMode] PlayerStart(tag='%s') not found — falling back"), *TargetTag.ToString());
	return Super::ChoosePlayerStart_Implementation(Player);
}

UClass* AP1ArenaGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const AP1PlayerController* P1PC = Cast<AP1PlayerController>(InController))
	{
		if (P1PC->SelectedCharacterClass)
		{
			return P1PC->SelectedCharacterClass;
		}
	}

	// SelectedCharacterClass가 설정되지 않은 경우 BP_P1ArenaGameMode에 지정된 DefaultPawnClass 사용.
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

FString AP1ArenaGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
	const FString& Options, const FString& Portal)
{
	const FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	if (!ErrorMessage.IsEmpty())
	{
		return ErrorMessage;
	}

	AP1PlayerController* P1PC = Cast<AP1PlayerController>(NewPlayerController);
	if (!P1PC || !HeroTable)
	{
		return ErrorMessage;
	}

	const FString HeroIdOption = UGameplayStatics::ParseOption(Options, TEXT("HeroId"));
	if (HeroIdOption.IsEmpty())
	{
		UE_LOG(LogP1, Warning, TEXT("[ArenaGameMode] InitNewPlayer — URL에 HeroId 옵션이 없음 (%s), 기본 스폰 클래스로 폴백"),
			*NewPlayerController->GetName());
		return ErrorMessage;
	}

	const FP1HeroDefinition* Row = HeroTable->FindRow<FP1HeroDefinition>(FName(*HeroIdOption), TEXT("AP1ArenaGameMode::InitNewPlayer"));
	if (!Row || !Row->HeroClass)
	{
		// HeroTable에 없는(조작되었거나 오래된) HeroId — 화이트리스트에 없는 값이므로 조용히 무시하고
		// GetDefaultPawnClassForController_Implementation의 기존 폴백(GameMode DefaultPawnClass)에 맡긴다.
		UE_LOG(LogP1, Warning, TEXT("[ArenaGameMode] InitNewPlayer — HeroTable에 없는 HeroId='%s' (%s), 기본 스폰 클래스로 폴백"),
			*HeroIdOption, *NewPlayerController->GetName());
		return ErrorMessage;
	}

	P1PC->SelectedCharacterClass = Row->HeroClass;
	UE_LOG(LogP1, Log, TEXT("[ArenaGameMode] InitNewPlayer — HeroId='%s' → %s (%s)"),
		*HeroIdOption, *Row->HeroClass->GetName(), *NewPlayerController->GetName());
	return ErrorMessage;
}
