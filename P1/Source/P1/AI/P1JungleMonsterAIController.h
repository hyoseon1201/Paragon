// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "P1JungleMonsterAIController.generated.h"

class UBehaviorTree;

// 정글 몬스터 전용 AIController — Behavior Tree 기반 3상태(Idle/Combat/Reset) FSM을 돌린다.
// 블랙보드는 최소한만 쓴다: TargetActor(현재 어그로 대상)와 IsLeashing(리시/복귀 중인지) 둘 뿐 —
// HomeLocation/LeashRadius 등 "설정값"은 블랙보드에 중복 저장하지 않고 BT 노드들이 그때그때
// AP1JungleMonsterCharacter에서 직접 읽는다(값이 두 곳에 흩어지지 않도록 단일 소스 오브 트루스 유지).
//
// BT 레이아웃(에디터에서 구성, 이 클래스는 그 골격만 제공 — 자세한 설명은 CLAUDE.md 참고):
//   Root Selector
//     1. Reset   — Decorator: IsLeashing==true               → BTTask_P1ReturnToCamp
//     2. Combat  — Decorator: TargetActor is set              → Service: BTService_P1JungleLeashCheck
//                    Selector(Loop)
//                      Sequence: MoveTo(TargetActor, 사거리) → BTTask_P1MeleeAttack
//     3. Idle    — (fallback, 아무것도 안 함 / 제자리 대기)
UCLASS()
class P1_API AP1JungleMonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
	AP1JungleMonsterAIController();

	// 블랙보드 키 이름 — BT/Blackboard 에셋의 키 이름과 반드시 일치해야 한다(에디터 작업 필요).
	static const FName BBKey_TargetActor;
	static const FName BBKey_IsLeashing;

	// AP1JungleMonsterCharacter::OnHitReactEventReceived가 호출 — 공격자를 타겟으로 세팅해 Combat
	// 브랜치를 깨운다. 이미 리시 중(IsLeashing=true, 집으로 가는 길)이면 무시한다 — 리시 중엔 무적이라
	// 데미지가 안 들어오므로 사실상 발생하지 않지만, 방어적으로 한 번 더 막아둔다.
	void NotifyAggro(AActor* InstigatorActor);

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
};
