// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/P1JungleCampAnchor.h"
#include "AI/P1JungleMonsterAIController.h"
#include "Characters/P1JungleMonsterCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CurveTable.h"
#include "GameModes/P1GameState.h"
#include "TimerManager.h"
#include "P1.h"

AP1JungleCampAnchor::AP1JungleCampAnchor()
{
	MarkerMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMeshComponent"));
	SetRootComponent(MarkerMeshComponent);
	MarkerMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMeshComponent->SetCastShadow(false);
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
	}
}
