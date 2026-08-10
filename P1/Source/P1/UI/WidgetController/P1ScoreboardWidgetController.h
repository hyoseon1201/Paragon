// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/P1WidgetController.h"
#include "P1ScoreboardWidgetController.generated.h"

class AP1GameState;

// 점수판 전용 WidgetController — 리액티브 델리게이트는 없다(Tab을 누를 때마다 스냅샷을 한 번씩 다시
// 그리는 방식이라 ASC/PlayerState 변경을 구독할 필요가 없음). UP1ScoreboardWidget이
// SetWidgetController()로 미리 배치된 팀 위젯들에까지 그대로 전파해, GameState 접근을 위젯 트리
// 전체에 공유하는 용도로만 쓴다 — 상점의 SetWidgetController 전파 패턴과 통일하기 위해 존재.
UCLASS(BlueprintType)
class P1_API UP1ScoreboardWidgetController : public UP1WidgetController
{
	GENERATED_BODY()

public:
	AP1GameState* GetP1GameState() const;
};
