// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/P1JungleCampAnchor.h"
#include "AI/P1JungleMonsterAIController.h"
#include "Characters/P1JungleMonsterCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CurveTable.h"
#include "GameModes/P1GameState.h"
#include "Player/P1PlayerState.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "TimerManager.h"
#include "P1.h"

AP1JungleCampAnchor::AP1JungleCampAnchor()
{
	MarkerMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMeshComponent"));
	SetRootComponent(MarkerMeshComponent);
	MarkerMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMeshComponent->SetCastShadow(false);

	// 미니맵 안개 시스템(RespawnServerTime/TeamHasObservedDeath)이 리플리케이트되는 프로퍼티라 필요 —
	// 레벨 배치 액터라 기존엔 리플리케이션이 전혀 필요 없었지만(CurrentMonsters는 여전히 서버 전용),
	// 이 두 값만은 클라이언트가 직접 읽어야 하는 최초의 예외.
	bReplicates = true;
}

void AP1JungleCampAnchor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AP1JungleCampAnchor, RespawnServerTime, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1JungleCampAnchor, TeamHasObservedDeath, SharedParams);
}

void AP1JungleCampAnchor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnMonster();
	}
}

int32 AP1JungleCampAnchor::ComputeMonsterLevel() const
{
	if (!MonsterLevelByMatchTimeTable)
	{
		return 1;
	}

	static const FName RowName(TEXT("MonsterLevelByMatchTime"));
	const FRealCurve* Curve = MonsterLevelByMatchTimeTable->FindCurve(RowName, TEXT("AP1JungleCampAnchor::ComputeMonsterLevel"));
	if (!Curve)
	{
		UE_LOG(LogP1, Warning, TEXT("[JungleCampAnchor] MonsterLevelByMatchTimeTable에 '%s' Row가 없습니다 (%s)"), *RowName.ToString(), *GetName());
		return 1;
	}

	// AP1GameState의 권위 있는 매치 경과 시계 사용(매치 시작 시각 기준 — 서버 프로세스 업타임인
	// GetWorld()->GetTimeSeconds()와 달리 "매치가 실제로 시작된 시점"을 0초로 정확히 잡는다).
	const AP1GameState* P1GS = GetWorld() ? GetWorld()->GetGameState<AP1GameState>() : nullptr;
	const float MatchTimeMinutes = P1GS ? P1GS->GetElapsedMatchTime() / 60.0f : 0.0f;
	return FMath::Max(1, FMath::RoundToInt(Curve->Eval(MatchTimeMinutes)));
}

FVector AP1JungleCampAnchor::ComputeSpawnOffset(int32 Index) const
{
	if (MonsterCount <= 1)
	{
		return FVector::ZeroVector;
	}

	const float Angle = (2.0f * PI * static_cast<float>(Index)) / static_cast<float>(MonsterCount);
	return FVector(PackSpawnRadius * FMath::Cos(Angle), PackSpawnRadius * FMath::Sin(Angle), 0.0f);
}

void AP1JungleCampAnchor::SpawnMonster()
{
	// 리스폰 시점 — 미확인 상태였던 팀도 포함해 전원에게 즉시 "살아있음"으로 반영된다(안개 시스템의
	// 핵심 요구사항: 죽음은 직접 확인해야 알지만, 부활은 확인 여부와 무관하게 즉시 알려진다).
	RespawnServerTime = -1.0f;
	GetWorldTimerManager().ClearTimer(VisionCheckTimerHandle);

	if (!MonsterClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[JungleCampAnchor] MonsterClass가 설정되지 않았습니다 (%s) — 스폰 스킵"), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const int32 ComputedLevel = ComputeMonsterLevel();
	const int32 Count = FMath::Max(1, MonsterCount);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector SpawnLocation = GetActorLocation() + ComputeSpawnOffset(Index);

		// SpawnActorDeferred를 쓰는 이유 — 즉시 스폰(SpawnActor)은 스폰 호출 안에서 BeginPlay()를 동기
		// 실행해버려서, 스폰 "이후"에 SetMonsterLevel()을 불러봐야 이미 ApplyDefaultAttributes()가
		// 레벨 1로 GE를 적용한 뒤라 늦는다(투사체 바운스 설정 때 겪었던 것과 동일한 타이밍 함정). Deferred로
		// 스폰해서 FinishSpawning() 전에 레벨/홈/리시반경을 전부 확정해야 BeginPlay가 정확한 값을 본다.
		const FTransform SpawnTransform(GetActorRotation(), SpawnLocation);
		AP1JungleMonsterCharacter* Monster = World->SpawnActorDeferred<AP1JungleMonsterCharacter>(
			MonsterClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Monster)
		{
			UE_LOG(LogP1, Warning, TEXT("[JungleCampAnchor] 몬스터 스폰 실패 (%s, index=%d)"), *GetName(), Index);
			continue;
		}

		// 홈은 앵커 중심이 아니라 이 몬스터가 실제로 스폰된 자리 — 무리끼리 서로 다른 위치를 각자의
		// 리시 기준점으로 삼는다(한 마리를 유인해도 나머지는 자기 자리에 남아있는 게 자연스러움).
		Monster->SetHomeLocation(SpawnLocation);
		if (LeashRadiusOverride > 0.0f)
		{
			Monster->SetLeashRadius(LeashRadiusOverride);
		}

		Monster->SetMonsterLevel(ComputedLevel);
		UE_LOG(LogP1, Log, TEXT("[JungleCampAnchor] 몬스터 스폰 — 앵커=%s 클래스=%s index=%d 레벨=%d (매치 경과 시간=%.1f초)"),
			*GetName(), *GetNameSafe(MonsterClass), Index, ComputedLevel, World->GetTimeSeconds());
		Monster->SetOwningCampAnchor(this);
		Monster->OnMonsterDied.AddUObject(this, &AP1JungleCampAnchor::OnMonsterDied);

		Monster->FinishSpawning(SpawnTransform);

		CurrentMonsters.Add(Monster);
	}

	UE_LOG(LogP1, Log, TEXT("[JungleCampAnchor] 몬스터 무리 스폰 — %d/%d마리 성공, Level=%d (%s)"),
		CurrentMonsters.Num(), Count, ComputedLevel, *GetName());
}

void AP1JungleCampAnchor::NotifyCampAggro(AActor* Attacker, AP1JungleMonsterCharacter* SourceMonster)
{
	for (const TObjectPtr<AP1JungleMonsterCharacter>& Monster : CurrentMonsters)
	{
		if (!IsValid(Monster) || Monster == SourceMonster)
		{
			continue;
		}

		if (AP1JungleMonsterAIController* AICon = Cast<AP1JungleMonsterAIController>(Monster->GetController()))
		{
			AICon->NotifyAggro(Attacker);
		}
	}
}

void AP1JungleCampAnchor::OnMonsterDied(AP1JungleMonsterCharacter* DeadMonster)
{
	CurrentMonsters.RemoveSingleSwap(DeadMonster);

	// 아직 무리 중 누군가 살아있으면 그쪽이 계속 싸우는 중이므로 리스폰 타이머를 돌리지 않는다 —
	// "무리 전체가 죽은 뒤"에만 카운트를 시작한다는 설계 요구사항.
	if (CurrentMonsters.Num() == 0)
	{
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AP1JungleCampAnchor::SpawnMonster, RespawnDelay, false);

		// 미니맵 안개 시스템 — 이번 죽음 사이클을 아직 아무 팀도 확인 못 한 상태로 시작하고, 팀별
		// 확인 여부를 주기적으로 검사하는 타이머를 돌린다(AP1GameState::GetElapsedMatchTime()과 동일하게
		// GetWorld()->GetTimeSeconds() 기준 — 서버 자신에게는 이 값이 곧 GetServerWorldTimeSeconds()와
		// 같으므로 클라이언트가 나중에 그 값으로 역산해도 어긋나지 않는다, StunEndServerTime과 동일한 패턴).
		const AP1GameState* P1GS = GetWorld() ? GetWorld()->GetGameState<AP1GameState>() : nullptr;
		TeamHasObservedDeath.Init(false, P1GS ? P1GS->GetNumTeams() : 0);
		RespawnServerTime = GetWorld()->GetTimeSeconds() + RespawnDelay;
		MARK_PROPERTY_DIRTY_FROM_NAME(AP1JungleCampAnchor, TeamHasObservedDeath, this);
		MARK_PROPERTY_DIRTY_FROM_NAME(AP1JungleCampAnchor, RespawnServerTime, this);

		GetWorldTimerManager().SetTimer(VisionCheckTimerHandle, this, &AP1JungleCampAnchor::CheckTeamVisionOfDeadCamp, 0.5f, true);

		UE_LOG(LogP1, Log, TEXT("[JungleCampAnchor] 무리 전멸 — 안개 확인 사이클 시작. AnchorLocation=%s VisionCheckRadius=%.0f 팀 수=%d (%s)"),
			*GetActorLocation().ToString(), VisionCheckRadius, TeamHasObservedDeath.Num(), *GetName());
	}
}

void AP1JungleCampAnchor::CheckTeamVisionOfDeadCamp()
{
	const AP1GameState* P1GS = GetWorld() ? GetWorld()->GetGameState<AP1GameState>() : nullptr;
	if (!P1GS)
	{
		return;
	}

	const float VisionCheckRadiusSq = FMath::Square(VisionCheckRadius);
	for (APlayerState* PS : P1GS->PlayerArray)
	{
		AP1PlayerState* P1PS = Cast<AP1PlayerState>(PS);
		APawn* AllyPawn = P1PS ? P1PS->GetPawn() : nullptr;
		if (!P1PS || !AllyPawn)
		{
			continue;
		}

		const int32 TeamId = P1PS->GetGenericTeamId().GetId();
		if (!TeamHasObservedDeath.IsValidIndex(TeamId) || TeamHasObservedDeath[TeamId])
		{
			continue; // 이미 확인했거나 팀 범위 밖 — 검사할 필요 없음.
		}

		const UAbilitySystemComponent* ASC = P1PS->GetAbilitySystemComponent();
		if (ASC && ASC->HasMatchingGameplayTag(TAG_State_Dead))
		{
			continue; // 죽어있는 아군은 시야를 제공하지 않는다 — 미니맵 아군 시야 규칙과 동일.
		}

		const float DistSq = FVector::DistSquared(AllyPawn->GetActorLocation(), GetActorLocation());
		const bool bInRange = DistSq <= VisionCheckRadiusSq;

		// 성공/실패 모두 남긴다 — "탐지 범위 안에 들어갔는데 갱신이 안 된다"는 증상을 진단할 때, 이 로그가
		// 아예 안 찍히면 팀 배정/생존 여부(위 continue들) 쪽이 원인이고, 찍히는데 거리가 기준을 못 넘으면
		// 실제로는 VisionCheckRadius 밖이었다는 뜻 — 둘을 구분하기 위한 임시 진단용.
		UE_LOG(LogP1, Verbose, TEXT("[JungleCampAnchor] VisionCheck — Team %d, %s, 거리=%.0f, 기준=%.0f, 범위안=%d (%s)"),
			TeamId, *AllyPawn->GetName(), FMath::Sqrt(DistSq), VisionCheckRadius, bInRange, *GetName());

		if (bInRange)
		{
			TeamHasObservedDeath[TeamId] = true;
			MARK_PROPERTY_DIRTY_FROM_NAME(AP1JungleCampAnchor, TeamHasObservedDeath, this);
		}
	}

	const bool bAllTeamsObserved = !TeamHasObservedDeath.Contains(false);
	if (bAllTeamsObserved)
	{
		GetWorldTimerManager().ClearTimer(VisionCheckTimerHandle); // 더 확인할 팀이 없으면 스스로 정지.
	}
}
