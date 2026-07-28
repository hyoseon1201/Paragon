// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTTask_P1MeleeAttack.h"
#include "Characters/P1JungleMonsterCharacter.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_P1MeleeAttack::UBTTask_P1MeleeAttack()
{
	NodeName = TEXT("Jungle Melee Attack");
}

EBTNodeResult::Type UBTTask_P1MeleeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (AP1JungleMonsterCharacter* Monster = AICon ? Cast<AP1JungleMonsterCharacter>(AICon->GetPawn()) : nullptr)
	{
		Monster->RequestMeleeAttack();
	}

	return EBTNodeResult::Succeeded;
}
