// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1JungleMonsterTypes.generated.h"

// DT_MonsterGoldXP 같은 DataTable의 Row(RowName=몬스터 타입, 예: "Wolves") — 처치 시 지급할 1레벨
// 기준 기본 골드/경험치. 실제 지급량은 AP1JungleMonsterCharacter::GetKillReward()가 이 값에
// RewardMultiplierTable(CT_MonsterRewardMultiplier, MonsterLevel로 조회)을 곱해 계산한다.
USTRUCT(BlueprintType)
struct P1_API FP1MonsterRewardData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JungleMonster")
	int32 BaseGold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JungleMonster")
	int32 BaseExperience = 0;
};
