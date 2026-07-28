// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_P1ReturnToCamp.generated.h"

class AP1JungleMonsterCharacter;

// Reset 브랜치의 유일한 태스크 — 무적+회복 시작 → MoveTo(HomeLocation) → 도착 감지 시 무적 해제+풀피
// 확정+IsLeashing 해제. 레이턴트(InProgress) 태스크로, MoveTo가 끝날 때까지 매 틱 도착 여부를 검사한다.
//
// MoveToLocation이 경로를 아예 못 찾거나(NavMesh가 HomeLocation을 안 덮는 경우 등), 어떤 이유로든
// 도착 판정이 영원히 안 걸리면 무적+회복이 영구히 켜진 채로 굳어버리는 문제가 있었다(캠프 위치가
// NavMesh 밖에 있어서 실제로 겪은 버그) — 그래서 (1) MoveToLocation의 즉시 실패를 감지해서 바로
// 포기하고, (2) MaxReturnDuration을 넘도록 도착을 못 하면 강제로 포기하는 안전장치 두 개를 둔다.
// 두 경우 모두 무적/회복/IsLeashing은 반드시 정리되므로 몬스터가 어딘가에 영구히 갇히지 않는다.
UCLASS()
class P1_API UBTTask_P1ReturnToCamp : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_P1ReturnToCamp();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Leash")
	float AcceptanceRadius = 50.0f;

	// 이 시간(초) 안에 도착 못 하면 강제로 포기 — NavMesh 문제 등으로 영영 도착 못 하는 경우의 안전장치.
	UPROPERTY(EditAnywhere, Category = "Leash", meta = (ClampMin = "1.0"))
	float MaxReturnDuration = 15.0f;

private:
	// 성공/실패 공통 정리(무적 해제+IsLeashing=false) — ExitLeashRecovery가 이미 안전하게 no-op 처리를
	// 하므로 성공/실패 어느 쪽이든 그대로 호출해도 된다.
	void FinishReturnHome(UBehaviorTreeComponent& OwnerComp, AP1JungleMonsterCharacter* Monster);

	float ElapsedSeconds = 0.0f;
};
