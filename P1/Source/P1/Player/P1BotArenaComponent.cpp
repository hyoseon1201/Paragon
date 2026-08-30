// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/P1BotArenaComponent.h"
#include "AI/P1JungleCampAnchor.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Characters/P1CharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/P1GameState.h"
#include "Player/P1PlayerState.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "P1.h"

UP1BotArenaComponent::UP1BotArenaComponent()
{
	// 이동 입력(AddMovementInput)만은 매 프레임 넣어줘야 한다 — CharacterMovementComponent는 자기 틱마다
	// 입력 벡터를 소비하자마자 리셋하므로, 0.2초 결정 타이머(BotTick) 안에서만 한 번씩 호출하면 대부분의
	// 프레임에 입력이 0이 되어 브레이킹(감속)되고, 순 이동량이 거의 0에 수렴하는 버그가 있었다(실제로
	// 겪은 버그 — 실제 플레이어는 Enhanced Input이 키가 눌려있는 동안 매 프레임 호출해줘서 이 문제가 없음).
	PrimaryComponentTick.bCanEverTick = true;
}

void UP1BotArenaComponent::BeginPlay()
{
	Super::BeginPlay();

	const APlayerController* OwningPC = GetOwner<APlayerController>();
	if (!OwningPC || !OwningPC->IsLocalController())
	{
		return;
	}

	if (!FParse::Value(FCommandLine::Get(), TEXT("BotId="), BotId))
	{
		return; // -BotId= 없으면 일반 플레이어 — 조용히 종료.
	}

	// 정글 캠프 앵커 위치를 캐싱 — 액터 이터레이션 순서는 실행마다 달라질 수 있으므로, 순찰 경로를
	// 진짜 결정론적으로 만들기 위해 반드시 정렬한다(X좌표 기준, 동률이면 Y좌표).
	for (TActorIterator<AP1JungleCampAnchor> It(GetWorld()); It; ++It)
	{
		PatrolLocations.Add(It->GetActorLocation());
	}
	PatrolLocations.Sort([](const FVector& A, const FVector& B)
	{
		return A.X != B.X ? A.X < B.X : A.Y < B.Y;
	});

	if (PatrolLocations.IsEmpty())
	{
		UE_LOG(LogP1, Warning, TEXT("[Bot] 정글 캠프 앵커를 하나도 못 찾음 — 순찰 불가 (BotId=%d)"), BotId);
		return;
	}

	UE_LOG(LogP1, Log, TEXT("[Bot] Arena 진입 — 순찰 지점 %d개 확보, 전투 루프 시작 (BotId=%d)"), PatrolLocations.Num(), BotId);
	GetWorld()->GetTimerManager().SetTimer(BotTickTimerHandle, this, &UP1BotArenaComponent::BotTick, BotTickIntervalSeconds, true);
}

void UP1BotArenaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 도착했거나(bHasArrivedAtCurrentTarget) 애초에 봇이 아니면(PatrolLocations 비어있음) 매 프레임
	// 이동 입력을 넣을 필요가 없다 — 상태 판단 자체는 여전히 BotTick(0.2초 타이머)이 전담한다.
	if (bHasArrivedAtCurrentTarget || PatrolLocations.IsEmpty())
	{
		return;
	}

	APlayerController* PC = GetOwner<APlayerController>();
	AP1CharacterBase* Character = PC ? Cast<AP1CharacterBase>(PC->GetCharacter()) : nullptr;
	if (!Character)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (CurrentPathPoints.IsEmpty() || (bCurrentPathIsFallback && Now >= NextPathRetryTime))
	{
		// 폴백(직선) 경로로 이미 이동 중이더라도 1초마다 진짜 내비메시 경로로 업그레이드를 재시도한다 —
		// 스폰 직후 내비게이션 시스템이 아직 준비 전이라 첫 요청이 실패하는 경우가 있었는데, 그때 폴백을
		// "영구 확정"으로 취급하면 그 직선이 장애물을 지나갈 경우 예전 버그가 재현되기 때문(아래 참고).
		RequestPathToCurrentTarget(Character);
		NextPathRetryTime = Now + 1.0;
		if (CurrentPathPoints.IsEmpty())
		{
			return; // 이번 프레임에도 계산 실패 — 다음 프레임 재시도.
		}
	}

	// 직선 대신 내비메시 경로의 웨이포인트를 하나씩 따라간다 — 정글몹/지형에 캐릭터가 낌지는 낌 문제를
	// 피하기 위함(직선 이동 시절 실제로 겪은 버그, PatrolArrivalRadius를 그대로 "웨이포인트 도착" 기준으로
	// 재사용). 목적지 자체에 도착했는지(전투 전환)는 BotTick()이 PatrolLocations 기준으로 별도 판단한다.
	FVector ToWaypoint = CurrentPathPoints[CurrentPathPointIndex] - Character->GetActorLocation();
	ToWaypoint.Z = 0.0f;

	if (ToWaypoint.SizeSquared() <= FMath::Square(PatrolArrivalRadius) && CurrentPathPointIndex < CurrentPathPoints.Num() - 1)
	{
		++CurrentPathPointIndex;
		ToWaypoint = CurrentPathPoints[CurrentPathPointIndex] - Character->GetActorLocation();
		ToWaypoint.Z = 0.0f;
	}

	Character->AddMovementInput(ToWaypoint.GetSafeNormal(), 1.0f);
}

FVector UP1BotArenaComponent::GetOffsetPatrolTarget(int32 Index) const
{
	const FVector Base = PatrolLocations[Index];
	// 47은 7(캠프 개수)과 서로소라 BotId가 늘어도 같은 각도로 겹치는 경우가 잘 안 생긴다.
	const float AngleRad = FMath::DegreesToRadians(static_cast<float>((BotId * 47) % 360));
	constexpr float OffsetRadius = 300.0f;
	return Base + FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0.0f) * OffsetRadius;
}

void UP1BotArenaComponent::RequestPathToCurrentTarget(AP1CharacterBase* Character)
{
	CurrentPathPoints.Reset();
	CurrentPathPointIndex = 0;

	if (!PatrolLocations.IsValidIndex(CurrentPatrolIndex))
	{
		return;
	}

	const FVector Start = Character->GetActorLocation();
	const FVector End = GetOffsetPatrolTarget(CurrentPatrolIndex);

	if (UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), Start, End))
	{
		UE_LOG(LogP1, Log, TEXT("[Bot][Diag] 경로 계산 — Owner=%s Start=%s End=%s IsValid=%d IsPartial=%d PointCount=%d LastPoint=%s"),
			*GetOwner()->GetName(), *Start.ToString(), *End.ToString(), NavPath->IsValid(), NavPath->IsPartial(),
			NavPath->PathPoints.Num(), NavPath->PathPoints.Num() > 0 ? *NavPath->PathPoints.Last().ToString() : TEXT("N/A"));

		if (NavPath->IsValid() && NavPath->PathPoints.Num() > 0)
		{
			CurrentPathPoints = NavPath->PathPoints;
			bCurrentPathIsFallback = false;
		}
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[Bot][Diag] FindPathToLocationSynchronously가 null 반환 — Owner=%s"), *GetOwner()->GetName());
	}

	if (CurrentPathPoints.IsEmpty())
	{
		// 내비메시 경로 계산 실패 — 직선 목표 하나짜리 경로로 임시 폴백(완전히 못 움직이는 것보단 낫다).
		// bCurrentPathIsFallback=true로 표시해두면 TickComponent가 1초마다 진짜 경로로 업그레이드를
		// 재시도한다 — 스폰 직후 내비게이션 시스템 초기화 타이밍 레이스로 실패하는 경우가 실제로 있었다.
		UE_LOG(LogP1, Warning, TEXT("[Bot] 내비메시 경로 계산 실패 — 직선 이동으로 임시 폴백 (Owner=%s)"), *GetOwner()->GetName());
		CurrentPathPoints.Add(End);
		bCurrentPathIsFallback = true;
	}
}

void UP1BotArenaComponent::ApplyTeamPatrolOffsetIfReady()
{
	if (bPatrolStartOffsetApplied || PatrolLocations.IsEmpty())
	{
		return;
	}

	const APlayerController* PC = GetOwner<APlayerController>();
	const AP1PlayerState* P1PS = PC ? Cast<AP1PlayerState>(PC->PlayerState) : nullptr;
	if (!P1PS)
	{
		return; // PlayerState가 아직 안 붙었음 — 다음 BotTick에서 재시도.
	}

	const uint8 TeamId = P1PS->GetGenericTeamId().GetId();
	if (TeamId == 255) // FGenericTeamId::NoTeam — 팀 배정 전, 다음 BotTick에서 재시도.
	{
		return;
	}

	const AP1GameState* P1GS = GetWorld() ? GetWorld()->GetGameState<AP1GameState>() : nullptr;
	const int32 NumTeams = P1GS ? P1GS->GetNumTeams() : 0;
	if (NumTeams > 0)
	{
		// 팀마다 캠프 목록을 균등하게 나눠 서로 다른 지점에서 순찰을 시작 — 같은 캠프에 몰리지 않고
		// 흩어지게 해서, 팀 간 거리가 자연스럽게 벌어져 히어로 NetCullDistanceSquared 컬링이 실제로
		// 걸릴 기회를 만든다. **+BotId**: 팀 기준 인덱스만 쓰면 같은 팀 봇 전원이 정확히 같은 좌표를
		// 목표로 걷다가 서로의 CollisionCylinder에 부딪혀 영구히 낌(실제로 겪은 버그 — 같은 팀 3마리가
		// 전부 동일 StartIndex로 배정된 로그, 그리고 한 봇이 다른 봇 캐릭터에 막혀 몇 분간 완전히
		// 정지한 CharacterMovement 로그로 확인됨). BotId를 더해 같은 팀이라도 서로 다른 캠프로
		// 갈라지게 한다.
		CurrentPatrolIndex = ((TeamId * PatrolLocations.Num()) / NumTeams + BotId) % PatrolLocations.Num();

		// 팀 배정(AP1ArenaGameMode::ChoosePlayerStart_Implementation)이 이 컴포넌트의 BeginPlay보다
		// 먼저 끝난다는 보장이 없어서, TickComponent()가 이미 기본값(인덱스 0) 목표로 걷기 시작한
		// *이후에* 여기서 인덱스가 바뀌는 경우가 실제로 있다 — 이때 CurrentPathPoints를 그대로 두면
		// TickComponent는 옛 목표(인덱스 0)를 향해 계속 걸어가 결국 거기 도착하는데, BotTick()의 도착
		// 판정은 이미 바뀐 새 인덱스 좌표와 비교하니 거리가 영원히 안 좁혀져 그 자리에 영구 정지한다
		// (실제로 겪은 버그 — 진단 로그로 확인: TargetIdx는 바뀐 값인데 CharLoc은 옛 목표 지점에 고정).
		// 인덱스가 실제로 바뀔 때는 진행 중이던 경로/도착 상태를 BotTick()의 "다음 캠프로" 전환과
		// 동일하게 리셋해 TickComponent가 새 목표로 다시 계산하게 한다.
		CurrentPathPoints.Reset();
		bHasArrivedAtCurrentTarget = false;
	}

	bPatrolStartOffsetApplied = true;
	UE_LOG(LogP1, Log, TEXT("[Bot] 팀 기준 순찰 시작 캠프 배정 — Team=%d, StartIndex=%d/%d (%s)"),
		TeamId, CurrentPatrolIndex, PatrolLocations.Num(), *GetOwner()->GetName());
}

void UP1BotArenaComponent::BotTick()
{
	ApplyTeamPatrolOffsetIfReady();

	APlayerController* PC = GetOwner<APlayerController>();
	AP1CharacterBase* Character = PC ? Cast<AP1CharacterBase>(PC->GetCharacter()) : nullptr;

	const double Now = GetWorld()->GetTimeSeconds();
	const bool bShouldLog = (Now - LastDiagnosticLogTime) >= 1.0;

	if (!Character)
	{
		if (bShouldLog)
		{
			LastDiagnosticLogTime = Now;
			UE_LOG(LogP1, Warning, TEXT("[Bot][Diag] Owner=%s Character가 null — PC=%s Pawn=%s"),
				*GetOwner()->GetName(), PC ? TEXT("Valid") : TEXT("Null"),
				(PC && PC->GetPawn()) ? *PC->GetPawn()->GetName() : TEXT("None"));
		}
		return; // 스폰 전이거나 사망 중 — 다음 틱에 다시 시도.
	}

	const FVector Target = GetOffsetPatrolTarget(CurrentPatrolIndex);
	FVector ToTarget = Target - Character->GetActorLocation();
	ToTarget.Z = 0.0f;

	if (bShouldLog)
	{
		LastDiagnosticLogTime = Now;
		UE_LOG(LogP1, Log, TEXT("[Bot][Diag] Owner=%s CharLoc=%s TargetIdx=%d Target=%s DistSq=%.0f ArrivalRadiusSq=%.0f Arrived=%d MoveInputIgnored=%d MovementMode=%d IsLocallyControlled=%d"),
			*GetOwner()->GetName(), *Character->GetActorLocation().ToString(), CurrentPatrolIndex, *Target.ToString(),
			ToTarget.SizeSquared(), FMath::Square(PatrolArrivalRadius), bHasArrivedAtCurrentTarget,
			Character->IsMoveInputIgnored(),
			Character->GetCharacterMovement() ? (int32)Character->GetCharacterMovement()->MovementMode.GetValue() : -1,
			Character->IsLocallyControlled());
	}

	if (ToTarget.SizeSquared() > FMath::Square(PatrolArrivalRadius))
	{
		// 아직 도착 전 — 이동만 하고 공격은 안 한다. 기본공격은 진짜 쿨다운이 없어서 매 틱
		// 재시전되는데, 그 애니메이션의 루트모션이 AddMovementInput을 덮어써서 제자리에서
		// 계속 공격만 하고 전혀 못 움직이는 버그가 있었다 — 이동/전투를 상태로 분리해서 해결.
		// 실제 AddMovementInput 호출은 TickComponent()가 매 프레임 담당(아래 참고) — 여기선
		// "아직 도착 안 함" 상태만 갱신한다.
		bHasArrivedAtCurrentTarget = false;
		return;
	}

	// 도착 — 이동을 멈추고 일정 시간 머물며 공격, 그 다음 다음 캠프로.
	if (!bHasArrivedAtCurrentTarget)
	{
		bHasArrivedAtCurrentTarget = true;
		CampDepartureTime = GetWorld()->GetTimeSeconds() + CampDwellSeconds;
	}

	if (GetWorld()->GetTimeSeconds() >= CampDepartureTime)
	{
		CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolLocations.Num();
		bHasArrivedAtCurrentTarget = false;
		CurrentPathPoints.Reset(); // 목표가 바뀌었으니 다음 TickComponent()에서 새 경로를 계산하게 한다.
		return;
	}

	// 전투 — Q와 기본공격을 무조건 눌렀다 뗀다. 쿨다운 중이면 AbilityInputTagPressed 내부의
	// TryActivateAbility가 알아서 조용히 무시하므로 여기서 쿨다운 여부까지 판단할 필요는 없다.
	// 다만 BotTick(0.2초)마다 매번 시도하면 Q처럼 쿨다운이 훨씬 긴 스킬은 실제 캐스트 한 번당
	// "쿨다운 중" 판정(EvaluateCanExecute)을 수십 번 헛되이 반복하게 되므로(Insights로 실측된 낭비),
	// 이 시도 자체는 CombatInputIntervalSeconds(기본 1초) 간격으로만 한다.
	if (Now - LastCombatInputTime >= CombatInputIntervalSeconds)
	{
		LastCombatInputTime = Now;
		if (UP1AbilitySystemComponent* ASC = Cast<UP1AbilitySystemComponent>(Character->GetAbilitySystemComponent()))
		{
			ASC->AbilityInputTagPressed(TAG_InputTag_Ability_Q);
			ASC->AbilityInputTagReleased(TAG_InputTag_Ability_Q);
			ASC->AbilityInputTagPressed(TAG_InputTag_Ability_BasicAttack);
			ASC->AbilityInputTagReleased(TAG_InputTag_Ability_BasicAttack);
		}
	}
}
