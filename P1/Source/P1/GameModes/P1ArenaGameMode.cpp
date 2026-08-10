// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/P1ArenaGameMode.h"
#include "Characters/P1HeroCharacter.h"
#include "GameModes/P1GameState.h"
#include "Player/P1PlayerController.h"
#include "Player/P1PlayerState.h"
#include "GameFramework/PlayerStart.h"
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
		P1GS->SetMatchStartTime();
		UE_LOG(LogP1, Log, TEXT("[ArenaGameMode] 매치 시작 시각 기록 — ServerWorldTime=%.2f (%s)"),
			P1GS->GetServerWorldTimeSeconds(), *GetName());
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[ArenaGameMode] BeginPlay — GetGameState<AP1GameState>()가 null, GameStateClass 설정 확인 필요"));
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
