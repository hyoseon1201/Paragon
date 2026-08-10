// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1ScoreboardPlayerRowWidget.generated.h"

class UTextBlock;
class AP1PlayerState;

// 점수판의 개별 플레이어 한 줄 — 닉네임/캐릭터/KDA만 표시하는 스냅샷 위젯이라 ASC/위젯 컨트롤러가
// 필요 없다(UP1PreGameHUDWidget과 동일한 이유로 UP1UserWidget이 아니라 UUserWidget을 직접 상속).
UCLASS()
class P1_API UP1ScoreboardPlayerRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 팀 위젯이 리프레시될 때마다 호출 — GameState->PlayerArray에서 읽은 현재 값을 그대로 밀어넣는다.
	// 실시간 구독이 아니라 스냅샷이라, 스코어보드를 든 채로 오래 버티면 다음 리프레시(재오픈) 전까지는
	// 값이 갱신되지 않는다 — 필요해지면 나중에 델리게이트 구독으로 바꿀 수 있다.
	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	void SetPlayerData(AP1PlayerState* PlayerState);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NicknameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CharacterText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KDAText;
};
