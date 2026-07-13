// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "P1MMC_GoldKillBounty.generated.h"

class UCurveTable;

// GE_GoldReward_Kill의 Gold 모디파이어 계산 클래스. 이 GE를 적용하는 쪽(AP1HeroCharacter::GrantKillReward)이
// 피해자의 죽기 직전 KillStreak/생존시간을 Data.KillStreak / Data.TimeSinceLastDeath로 SetByCaller에 실어두면,
// 여기서 그 둘을 읽어 "기본 킬 골드 + 연속킬 현상금(커브 테이블) + 오래 생존한 보정(선형)"을 합산한
// 최종 골드를 반환한다 — 실제 계산 로직은 전부 여기 있고, C++ 쪽(AttributeSet/HeroCharacter)은
// "누가 죽였는지, 피해자 상태가 어땠는지"만 알면 되고 골드 액수 자체는 몰라도 된다.
UCLASS()
class P1_API UP1MMC_GoldKillBounty : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

	// 킬 기본 골드(고정값) — 사용자 지정 300.
	UPROPERTY(EditDefaultsOnly, Category = "Bounty")
	float BaseKillGold = 300.0f;

	// 연속킬 수(Data.KillStreak) → 추가 골드 커브. Data/CT_KillStreakBonusGold.json을 CurveTable로
	// 임포트해 지정 — RowName="KillStreakBonusGold".
	UPROPERTY(EditDefaultsOnly, Category = "Bounty")
	TObjectPtr<UCurveTable> KillStreakBonusTable;

	// 생존시간(Data.TimeSinceLastDeath, 초) 1초당 추가 골드 — 선형 가산, MaxTimeAliveBonus에서 캡.
	UPROPERTY(EditDefaultsOnly, Category = "Bounty")
	float GoldPerSecondAlive = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bounty")
	float MaxTimeAliveBonus = 200.0f;
};
