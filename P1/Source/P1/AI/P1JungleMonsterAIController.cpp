// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/P1JungleMonsterAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "P1.h"

const FName AP1JungleMonsterAIController::BBKey_TargetActor(TEXT("TargetActor"));
const FName AP1JungleMonsterAIController::BBKey_IsLeashing(TEXT("IsLeashing"));

AP1JungleMonsterAIController::AP1JungleMonsterAIController()
{
}

void AP1JungleMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!BehaviorTreeAsset)
	{
		UE_LOG(LogP1, Warning, TEXT("[JungleMonsterAI] BehaviorTreeAsset이 설정되지 않았습니다 (%s) — BP에서 지정해주세요."), *GetName());
		return;
	}

	// UseBlackboard가 요구하는 시그니처는 raw 포인터 참조(UBlackboardComponent*&)라 TObjectPtr인
	// Blackboard 멤버를 직접 넘길 수 없다 — 로컬 raw 포인터로 받은 뒤 대입한다.
	UBlackboardComponent* BlackboardComp = Blackboard;
	if (UseBlackboard(BehaviorTreeAsset->BlackboardAsset, BlackboardComp))
	{
		Blackboard = BlackboardComp;
		RunBehaviorTree(BehaviorTreeAsset);
	}
	else
	{
		UE_LOG(LogP1, Warning, TEXT("[JungleMonsterAI] UseBlackboard 실패 (%s) — BehaviorTreeAsset에 BlackboardAsset이 지정됐는지 확인"), *GetName());
	}
}

void AP1JungleMonsterAIController::NotifyAggro(AActor* InstigatorActor)
{
	if (!IsValid(InstigatorActor) || !Blackboard)
	{
		return;
	}

	if (Blackboard->GetValueAsBool(BBKey_IsLeashing))
	{
		return;
	}

	Blackboard->SetValueAsObject(BBKey_TargetActor, InstigatorActor);
}
