// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "P1GameplayCueNotify_ParticleEffect.generated.h"

class UParticleSystem;

// 1회성(Burst) 파티클 이펙트 GameplayCue. GE 없이도 ASC->ExecuteGameplayCue()로 임의의 게임플레이
// 순간에 직접 발동할 수 있어, "강화된 공격이 적중하는 순간"처럼 몽타주 타임라인에 고정 배치할 수 없는
// 조건부 이펙트에 적합하다. 발동은 서버 ASC에서 하면 모든 클라이언트(시뮬레이티드 프록시 포함)에 복제된다.
UCLASS(meta = (DisplayName = "P1 Particle Effect Cue"))
class P1_API UP1GameplayCueNotify_ParticleEffect : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

	UPROPERTY(EditAnywhere, Category = "GameplayCue")
	TObjectPtr<UParticleSystem> ParticleTemplate;

	// 부착할 소켓/본 이름. 비어있으면(None) 액터 위치에 월드 스폰.
	UPROPERTY(EditAnywhere, Category = "GameplayCue")
	FName SocketName;
};
