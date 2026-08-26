// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "P1BotArenaComponent.generated.h"

class AP1JungleCampAnchor;
class AP1CharacterBase;

// 부하테스트용 헤드리스 봇 전용 컴포넌트 — Arena 단계(정글 캠프 순찰 + Q/기본공격 반복)를 담당한다.
// AP1PlayerController 생성자에 이 컴포넌트를 붙이기만 하면 되고, PC 클래스 본문엔 봇 관련 코드가
// 전혀 없다. 커맨드라인에 "-BotId=N"이 없으면 아무것도 안 하므로 일반 플레이어와 무관.
//
// 봇 행동은 의도적으로 완전히 결정론적이다(재현 가능한 부하 시나리오가 목적 — NetSerialize/Push
// Model/Replication Graph 작업의 Before/After를 같은 조건에서 비교하려면 랜덤 요소가 없어야 함):
// 정글 캠프를 고정 순번으로 순찰하고, 도착한 캠프에서 일정 시간 머물며 Q+기본공격을 반복한 뒤 다음
// 캠프로 이동한다.
//
// **이동 중엔 공격을 절대 같이 하지 않는다** — 처음엔 매 틱 이동+공격을 동시에 시도했는데, 기본공격은
// 진짜 쿨다운이 없어서 0.2초마다 계속 새로 스윙이 재시전됐고, 그 공격 애니메이션의 루트모션이 매번
// AddMovementInput을 덮어써서 캐릭터가 제자리에서 계속 공격만 하고 전혀 못 움직이는 버그가 있었다
// (실제 발견된 버그). 그래서 "이동"과 "전투"를 상태로 완전히 분리 — 도착 전엔 이동만, 도착 후엔
// 정지한 채로 공격만 한다. 덕분에 정글몹한테 실제로 적중하는 진짜 전투 트래픽도 생겨서 일석이조.
//
// **이동 입력(AddMovementInput)은 BotTick(0.2초 타이머)이 아니라 TickComponent(매 프레임)가 담당한다**
// — CharacterMovementComponent가 자기 틱마다 입력 벡터를 소비 즉시 리셋해서, 0.2초에 한 번씩만 넣어주면
// 대부분의 프레임이 입력 0으로 브레이킹돼 순 이동량이 거의 0에 수렴하는 버그가 있었다(실제 발견된 버그).
// BotTick은 "도착했는지/공격할지" 같은 상태 판단만 담당.
//
// **목표까지 직선이 아니라 내비메시 경로(UNavigationSystemV1::FindPathToLocationSynchronously)를 따라
// 이동한다** — 직선 이동은 정글몹이나 지형 턱에 캐릭터가 그대로 박혀 영구히 멈추는 문제가 있었다
// (실제 발견된 버그, CharacterMovement 로그의 "is stuck and failed to move"로 확인). AAIController
// 전용 API(MoveToLocation)는 못 쓰므로(봇은 실제 접속 클라이언트라 APlayerController가 조종) 경로만
// 미리 계산해 웨이포인트 배열로 받아두고, 그 각 지점을 향해 TickComponent가 매 프레임 AddMovementInput을
// 넣는 방식으로 직접 구현했다.
UCLASS(ClassGroup = (Bot), meta = (BlueprintSpawnableComponent))
class P1_API UP1BotArenaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UP1BotArenaComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, Category = "Bot")
	float BotTickIntervalSeconds = 0.2f;

	// 목표 지점에 이만큼 가까워지면 "도착"으로 전환(이동 중지, 전투 시작).
	UPROPERTY(EditDefaultsOnly, Category = "Bot")
	float PatrolArrivalRadius = 150.0f;

	// 도착한 캠프에서 이만큼 머물며 싸운 뒤 다음 캠프로 이동.
	UPROPERTY(EditDefaultsOnly, Category = "Bot")
	float CampDwellSeconds = 6.0f;

	// Q/기본공격 입력을 시도하는 간격(초) — BotTickIntervalSeconds(0.2초)와 별개로 더 길게 둔다.
	// 원래는 BotTick과 같은 0.2초마다 매번 눌렀는데, 쿨다운이 훨씬 긴 Q 같은 스킬은 실제 캐스트 한 번당
	// "쿨다운 중이라 안 됨" 판정(EvaluateCanExecute)을 ~40번씩 헛되이 반복하는 꼴이라(Insights로 실측
	// 확인됨 — 45,508회 호출 중 상당수가 이 낭비), 봇 수가 늘어날수록 불필요한 서버 부하만 커졌다.
	// 기본공격은 쿨다운이 짧아(공격속도 기준 약 0.7~1초) 1초 간격이면 거의 매번 실제로 재시전되므로
	// DPS/전투 트래픽 패턴은 거의 그대로 유지된다.
	UPROPERTY(EditDefaultsOnly, Category = "Bot")
	float CombatInputIntervalSeconds = 1.0f;

private:
	void BotTick();

	// 팀마다 순찰 시작 캠프를 다르게 흩어놓는다(같은 캠프에 전 팀이 몰려있으면 서로 항상 가까워서
	// NetCullDistanceSquared 거리 컬링이 실측 테스트에서 거의 안 걸리는 문제가 있었음, 2026-08-26).
	// PlayerState->GetGenericTeamId()가 BeginPlay 시점엔 아직 미배정(NoTeam=255)일 수 있어서(팀 배정은
	// AP1ArenaGameMode::ChoosePlayerStart_Implementation에서 이뤄지는데, 그게 이 컴포넌트의 BeginPlay보다
	// 먼저 끝난다는 보장이 없음) BotTick 첫 실행 시점에 지연 적용하고, 아직도 미배정이면 다음 틱에 재시도한다.
	void ApplyTeamPatrolOffsetIfReady();
	bool bPatrolStartOffsetApplied = false;

	// PatrolLocations[CurrentPatrolIndex]까지 내비메시 경로를 계산해 CurrentPathPoints에 채운다
	// (실패 시 직선 목표 하나짜리 경로로 폴백). CurrentPathPoints가 비어있을 때 TickComponent()가
	// 호출한다 — BeginPlay 시점엔 아직 캐릭터가 스폰 전이라 여기서 미리 계산해둘 수 없다.
	void RequestPathToCurrentTarget(AP1CharacterBase* Character);

	FTimerHandle BotTickTimerHandle;
	TArray<FVector> PatrolLocations;
	int32 CurrentPatrolIndex = 0;

	// 현재 목표 캠프까지 내비메시가 계산해준 경로 웨이포인트 — 장애물(정글몹/지형)에 낌 방지를 위해
	// 직선 대신 이 경로를 따라 이동한다. 목표(순찰 인덱스)가 바뀌면 비워져서 다음 프레임에 재계산된다.
	TArray<FVector> CurrentPathPoints;
	int32 CurrentPathPointIndex = 0;

	// CurrentPathPoints가 진짜 내비메시 경로가 아니라 직선 폴백인지 — 스폰 직후 몇 프레임 동안은
	// UNavigationSystemV1이 아직 준비 전이라(GetDefaultNavDataInstance()==nullptr) 첫 경로 요청이
	// null을 반환하는 경우가 실제로 있었다. 폴백을 "영구 확정"으로 취급하면 그 직선이 장애물을 지나갈
	// 경우 예전 버그가 그대로 재현되므로, 폴백 중엔 주기적으로 진짜 경로를 다시 요청해 업그레이드한다.
	bool bCurrentPathIsFallback = false;
	double NextPathRetryTime = 0.0;

	// 현재 목표 지점에 이미 도착해서 정지+전투 상태인지 — 이동/전투 상태 전환의 기준.
	bool bHasArrivedAtCurrentTarget = false;

	// bHasArrivedAtCurrentTarget이 true가 된 시점 + CampDwellSeconds — 이 시각이 되면 다음 캠프로.
	double CampDepartureTime = 0.0;

	// 진단용 — BotTick()이 매 0.2초마다 도는 걸 매번 로그로 찍으면 너무 시끄러워서 1초 간격으로만 찍는다.
	double LastDiagnosticLogTime = 0.0;

	// Q/기본공격을 마지막으로 시도한 시각 — CombatInputIntervalSeconds 간격으로만 재시도하기 위한 기준.
	double LastCombatInputTime = 0.0;
};
