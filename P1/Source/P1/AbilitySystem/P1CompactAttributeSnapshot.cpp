// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1CompactAttributeSnapshot.h"

namespace P1CompactAttributeSnapshot_Private
{
	// 체력바 표시엔 정수 HP면 충분 — 32비트 float 대신 16비트 정수로 반올림해서 보낸다.
	// 최대 HP/Mana가 65535를 넘지 않는다는 전제. 실제 최대치를 알면 더 좁혀도 된다
	// (예: 최대 4000이면 12비트 SerializeBits로도 충분).
	void SerializeQuantized(FArchive& Ar, float& Value)
	{
		uint16 Quantized = Ar.IsSaving() ? (uint16)FMath::Clamp(FMath::RoundToInt(Value), 0, 65535) : 0;
		Ar << Quantized;
		if (Ar.IsLoading())
		{
			Value = (float)Quantized;
		}
	}
}

bool FP1CompactAttributeSnapshot::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	using namespace P1CompactAttributeSnapshot_Private;

	SerializeQuantized(Ar, Health);
	SerializeQuantized(Ar, MaxHealth);
	SerializeQuantized(Ar, Mana);
	SerializeQuantized(Ar, MaxMana);

	bOutSuccess = true;
	return true;
}
