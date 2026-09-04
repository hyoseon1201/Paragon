// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "P1CompactAttributeSnapshot.generated.h"

// 다른 플레이어(소유 클라이언트가 아닌 쪽)에게 보여줄 체력/마나 압축 스냅샷.
// AP1PlayerState가 bAlwaysRelevant=true라 UP1AttributeSet의 Health/MaxHealth/Mana/MaxMana는
// 컬링 없이 맵 전체 커넥션에 정밀 float로 나간다 — 남이 볼 땐 체력바 그릴 정도의 근사치면
// 충분하므로, 이 struct는 그 네 값만 16비트 정수로 양자화해서 하나로 묶어 보낸다.
// GameplayAbilityRepAnimMontage.h(FGameplayAbilityRepAnimMontage)와 동일한 패턴 — 수동
// NetSerialize를 쓰는 USTRUCT.
USTRUCT()
struct FP1CompactAttributeSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	float Health = 0.f;

	UPROPERTY()
	float MaxHealth = 0.f;

	UPROPERTY()
	float Mana = 0.f;

	UPROPERTY()
	float MaxMana = 0.f;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	bool operator==(const FP1CompactAttributeSnapshot& Other) const
	{
		return Health == Other.Health && MaxHealth == Other.MaxHealth
			&& Mana == Other.Mana && MaxMana == Other.MaxMana;
	}
	bool operator!=(const FP1CompactAttributeSnapshot& Other) const
	{
		return !(*this == Other);
	}
};

template<>
struct TStructOpsTypeTraits<FP1CompactAttributeSnapshot> : public TStructOpsTypeTraitsBase2<FP1CompactAttributeSnapshot>
{
	enum
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
	};
};
