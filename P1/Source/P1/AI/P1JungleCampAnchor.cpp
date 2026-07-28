// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/P1JungleCampAnchor.h"
#include "Characters/P1JungleMonsterCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CurveTable.h"
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

	const float MatchTimeMinutes = GetWorld() ? GetWorld()->GetTimeSeconds() / 60.0f : 0.0f;
	return FMath::Max(1, FMath::RoundToInt(Curve->Eval(MatchTimeMinutes)));
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

	// SpawnActorDeferred를 쓰는 이유 — 즉시 스폰(SpawnActor)은 스폰 호출 안에서 BeginPlay()를 동기
	// 실행해버려서, 스폰 "이후"에 SetMonsterLevel()을 불러봐야 이미 ApplyDefaultAttributes()가
	// 레벨 1로 GE를 적용한 뒤라 늦는다(투사체 바운스 설정 때 겪었던 것과 동일한 타이밍 함정). Deferred로
	// 스폰해서 FinishSpawning() 전에 레벨/홈/리시반경을 전부 확정해야 BeginPlay가 정확한 값을 본다.
	const FTransform SpawnTransform(GetActorRotation(), GetActorLocation());
	AP1JungleMonsterCharacter* Monster = World->SpawnActorDeferred<AP1JungleMonsterCharacter>(
		MonsterClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Monster)
	{
		UE_LOG(LogP1, Warning, TEXT("[JungleCampAnchor] 몬스터 스폰 실패 (%s)"), *GetName());
		return;
	}

	Monster->SetHomeLocation(GetActorLocation());
	if (LeashRadiusOverride > 0.0f)
	{
		Monster->SetLeashRadius(LeashRadiusOverride);
	}

	const int32 ComputedLevel = ComputeMonsterLevel();
	Monster->SetMonsterLevel(ComputedLevel);
	Monster->OnMonsterDied.AddUObject(this, &AP1JungleCampAnchor::OnMonsterDied);

	Monster->FinishSpawning(SpawnTransform);

	UE_LOG(LogP1, Log, TEXT("[JungleCampAnchor] 몬스터 스폰 — Level=%d (%s → %s)"), ComputedLevel, *GetName(), *Monster->GetName());

	CurrentMonster = Monster;
}

void AP1JungleCampAnchor::OnMonsterDied(AP1JungleMonsterCharacter* DeadMonster)
{
	CurrentMonster = nullptr;
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AP1JungleCampAnchor::SpawnMonster, RespawnDelay, false);
}
