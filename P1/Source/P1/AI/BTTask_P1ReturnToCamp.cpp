// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTTask_P1ReturnToCamp.h"
#include "AI/P1JungleMonsterAIController.h"
#include "Characters/P1JungleMonsterCharacter.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "P1.h"

UBTTask_P1ReturnToCamp::UBTTask_P1ReturnToCamp()
{
	NodeName = TEXT("Return To Camp");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_P1ReturnToCamp::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	AP1JungleMonsterCharacter* Monster = AICon ? Cast<AP1JungleMonsterCharacter>(AICon->GetPawn()) : nullptr;
	if (!Monster)
	{
		return EBTNodeResult::Failed;
	}

	// 무적+빠른 회복 시작 — 도착 전까지(TickTask가 감지할 때까지) 계속 유지된다.
	Monster->EnterLeashRecovery();
	ElapsedSeconds = 0.0f;

	const FVector Home = Monster->GetHomeLocation();
	// bStopOnOverlap=false — 기본값(true)이면 AIController가 "캡슐이 목표 지점과 겹치면 도착"으로
	// 판정해 실제로는 AcceptanceRadius+캡슐반경만큼 떨어진 채로 이동을 멈춰버린다(예: AcceptRadius=100,
	// 캡슐반경=38이면 138 거리에서 멈춤). 그런데 TickTask의 도착 판정은 순수 점대점 거리로 AcceptanceRadius만
	// 비교하므로 이 상태에선 영원히 "도착"으로 안 잡히고 MaxReturnDuration 강제 포기까지 무적이 풀리지
	// 않는 버그가 있었다 — bStopOnOverlap=false로 캡슐 여유 없이 실제로 AcceptanceRadius 안까지 걸어오게
	// 강제해 두 판정 기준을 일치시킨다.
	const EPathFollowingRequestResult::Type MoveResult = AICon->MoveToLocation(Home, AcceptanceRadius, false);

	UE_LOG(LogP1, Log, TEXT("[JungleMonsterAI] ReturnToCamp 시작 — Home=%s AcceptRadius=%.0f 현재거리=%.0f MoveToResult=%d (%s)"),
		*Home.ToCompactString(), AcceptanceRadius,
		FVector::Dist(Monster->GetActorLocation(), Home),
		static_cast<int32>(MoveResult), *Monster->GetName());

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		// 경로를 아예 못 찾음(NavMesh가 홈 지점을 안 덮거나 도달 불가) — 무적/회복을 켜둔 채로 멈춰있으면
		// 안 되니 즉시 정리하고 포기. 몬스터는 그냥 현재 위치에 남고, 다음 틱에 타겟이 있으면 Combat으로,
		// 없으면 Idle로 자연스럽게 넘어간다.
		UE_LOG(LogP1, Warning, TEXT("[JungleMonsterAI] ReturnToCamp — MoveToLocation 실패(경로 없음), 즉시 포기 (%s) — HomeLocation이 NavMesh 밖에 있는지 확인할 것"), *Monster->GetName());
		FinishReturnHome(OwnerComp, Monster);
		return EBTNodeResult::Failed;
	}

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		FinishReturnHome(OwnerComp, Monster);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_P1ReturnToCamp::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	AP1JungleMonsterCharacter* Monster = AICon ? Cast<AP1JungleMonsterCharacter>(AICon->GetPawn()) : nullptr;
	if (!Monster)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (FVector::Dist(Monster->GetActorLocation(), Monster->GetHomeLocation()) <= AcceptanceRadius)
	{
		UE_LOG(LogP1, Log, TEXT("[JungleMonsterAI] ReturnToCamp 도착 감지 — 거리기준 정상 완료 (%s)"), *Monster->GetName());
		FinishReturnHome(OwnerComp, Monster);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 안전장치 — 경로가 막혔거나 밀려서 도착 판정이 영원히 안 걸리는 경우, MaxReturnDuration을 넘으면
	// 강제로 포기한다. 안 그러면 무적+회복이 영원히 켜진 채로 굳어버릴 수 있다(실제로 겪은 버그).
	ElapsedSeconds += DeltaSeconds;
	if (ElapsedSeconds >= MaxReturnDuration)
	{
		UE_LOG(LogP1, Warning, TEXT("[JungleMonsterAI] ReturnToCamp — %.0f초 동안 도착 못 함(남은거리=%.0f), 강제 포기 (%s)"),
			MaxReturnDuration, FVector::Dist(Monster->GetActorLocation(), Monster->GetHomeLocation()), *Monster->GetName());
		FinishReturnHome(OwnerComp, Monster);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

void UBTTask_P1ReturnToCamp::FinishReturnHome(UBehaviorTreeComponent& OwnerComp, AP1JungleMonsterCharacter* Monster)
{
	// ExitLeashRecovery는 bIsLeashRecovering 가드가 없어도(EnterLeashRecovery 자체엔 있지만) 항상 안전하게
	// 호출 가능 — 무적 핸들이 없으면 그 분기만 스킵되고 SetHealth(MaxHealth)/bIsLeashRecovering=false는 그대로 적용된다.
	Monster->ExitLeashRecovery();

	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsBool(AP1JungleMonsterAIController::BBKey_IsLeashing, false);

		// SetValueAsBool은 옵저버(Reset 브랜치의 IsLeashing==true 데코레이터)를 동기적으로 즉시 통지할 수
		// 있다 — 그 데코레이터가 Observer Aborts=Self로 설정돼 있으면 이 호출 도중에 트리가 이미 이
		// 태스크를 어보트시키기 시작했을 수 있고, 뒤이은 FinishLatentTask(Succeeded) 호출과 경합해서
		// "정리는 됐는데 트리가 다음 브랜치로 안 넘어가는" 상태로 굳는 경우가 있다 — 실제로 값이 반영됐는지
		// 여기서 즉시 읽어봐서 확인한다.
		UE_LOG(LogP1, Log, TEXT("[JungleMonsterAI] FinishReturnHome — IsLeashing=false 기록, 읽어보면=%d (%s)"),
			BB->GetValueAsBool(AP1JungleMonsterAIController::BBKey_IsLeashing) ? 1 : 0, *Monster->GetName());
	}
}
