// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_P1JungleLeashCheck.generated.h"

// Combat 브랜치에 붙는 서비스 — 매 틱 몬스터 자신의 위치와 HomeLocation 사이 거리를 재서 LeashRadius를
// 넘으면 IsLeashing=true로 래치한다(한 번 넘으면 Reset 브랜치가 인터럽트로 우선 실행되고, 홈 도착 시
// BTTask_P1ReturnToCamp가 다시 false로 풀 때까지 유지 — 국지적으로 왔다갔다하며 깜빡이지 않음).
//
// 핵심 설계: 판정 기준이 "플레이어가 원점에서 얼마나 떨어졌는지"가 아니라 "몬스터 자신이 홈에서 얼마나
// 떨어졌는지"다. 플레이어가 최대 사거리에서 때리고 살짝 물러나 다시 때리는 식으로(hit-and-run) 몬스터를
// 홈 근처에 붙잡아두면(플레이어 자신은 리시 반경을 들락날락해도) 몬스터는 계속 홈 근처에 있으므로 리시가
// 절대 발동하지 않는다 — 반대로 플레이어가 몬스터를 실제로 LeashRadius 밖까지 끌어냈을 때만 발동하므로,
// "몬스터를 얼마나 멀리 유인할 수 있는가"에 대한 자연스러운 상한이 된다(플레이어 위치 기준으로 판정하면
// 사거리+왕복 이동거리만큼 매 사이클 판정이 초기화돼 무한정 끌고 다닐 수 있는 게 원래 문제였음).
//
// 타겟이 사라졌거나(디스폰) State.Dead 상태가 되어도 같은 이유로 리시를 트리거한다 — 전투가 끝났으면
// 몬스터가 계속 그 자리에 서 있을 이유가 없다.
UCLASS()
class P1_API UBTService_P1JungleLeashCheck : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_P1JungleLeashCheck();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
