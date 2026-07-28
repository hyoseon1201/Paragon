// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_P1MeleeAttack.generated.h"

// Combat 브랜치에서 MoveTo(TargetActor, AcceptanceRadius=사거리) 뒤에 배치 — 사거리 안에 들어와
// Selector(Loop)가 이 태스크를 반복 실행하는 동안 매번 공격을 요청한다. 실제 재사용 대기시간 게이팅은
// UP1GameplayAbility_JungleMonsterMeleeAttack의 CooldownGameplayEffectClass가 전담하므로 이 태스크는
// 순간 완료(InstantTask)로 매번 요청만 보내면 된다.
UCLASS()
class P1_API UBTTask_P1MeleeAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_P1MeleeAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
