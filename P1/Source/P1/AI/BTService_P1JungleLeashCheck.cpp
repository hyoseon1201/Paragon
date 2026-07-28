// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTService_P1JungleLeashCheck.h"
#include "AI/P1JungleMonsterAIController.h"
#include "Characters/P1JungleMonsterCharacter.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "P1.h"

UBTService_P1JungleLeashCheck::UBTService_P1JungleLeashCheck()
{
	NodeName = TEXT("Jungle Leash Check");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void UBTService_P1JungleLeashCheck::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AICon = OwnerComp.GetAIOwner();
	AP1JungleMonsterCharacter* Monster = AICon ? Cast<AP1JungleMonsterCharacter>(AICon->GetPawn()) : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Monster || !BB)
	{
		return;
	}

	if (BB->GetValueAsBool(AP1JungleMonsterAIController::BBKey_IsLeashing))
	{
		// 이미 래치됨 — Reset 브랜치가 곧 인터럽트로 실행된다.
		return;
	}

	bool bShouldLeash = false;
	const TCHAR* LeashReason = TEXT("");

	const float DistFromHome = FVector::Dist(Monster->GetActorLocation(), Monster->GetHomeLocation());
	if (DistFromHome > Monster->GetLeashRadius())
	{
		bShouldLeash = true;
		LeashReason = TEXT("LeashRadius 초과");
	}
	else
	{
		AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(AP1JungleMonsterAIController::BBKey_TargetActor));
		if (!IsValid(TargetActor))
		{
			bShouldLeash = true;
			LeashReason = TEXT("TargetActor 무효화");
		}
		else if (const IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetActor))
		{
			const UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
			if (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_State_Dead))
			{
				// 히어로는 죽어도 Pawn이 파괴되지 않고 State.Dead GE가 자연 만료될 때까지 그 자리에
				// 남아있다(리스폰 컨벤션) — 그래서 IsValid만으론 부족하고 이 태그를 직접 봐야 한다.
				bShouldLeash = true;
				LeashReason = TEXT("타겟 State.Dead");
			}
		}
	}

	if (bShouldLeash)
	{
		UE_LOG(LogP1, Log, TEXT("[JungleMonsterAI] 리시 발동 — 사유=%s DistFromHome=%.0f LeashRadius=%.0f (%s)"),
			LeashReason, DistFromHome, Monster->GetLeashRadius(), *Monster->GetName());
		BB->SetValueAsBool(AP1JungleMonsterAIController::BBKey_IsLeashing, true);
		BB->ClearValue(AP1JungleMonsterAIController::BBKey_TargetActor);
	}
}
