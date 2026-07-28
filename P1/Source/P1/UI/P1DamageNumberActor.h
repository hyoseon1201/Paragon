// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1DamageNumberActor.generated.h"

class UP1FloatingWidgetComponent;
class UP1DamageNumberWidget;

// 데미지 팝업 텍스트 1회성 액터 — AP1PlayerController::ClientShowDamageNumber()가 로컬로만
// 스폰한다(Server가 아니라 순수 클라이언트 SpawnActor라 다른 클라이언트에는 전혀 안 보임, 리플리케이트
// 되지도 않음 — "내가 입힌 데미지만 나에게 보인다"는 요구사항 자체가 이 방식으로 자연히 충족된다).
// FloatingStatusComponent와 동일한 UP1FloatingWidgetComponent(Screen space + 카메라거리 스케일)를
// 재사용 — 항상 정면을 보고 카메라에 가까울수록 커지는 동일한 느낌을 공유한다.
UCLASS()
class P1_API AP1DamageNumberActor : public AActor
{
	GENERATED_BODY()

public:
	AP1DamageNumberActor();

	// 스폰 직후 1회 호출 — 위젯에 데미지 숫자와 색상 구분(물리/마법)을 세팅한다.
	void InitializeDamageNumber(float DamageAmount, bool bIsMagicalDamage);

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "DamageNumber")
	TObjectPtr<UP1FloatingWidgetComponent> WidgetComponent;

	// 이 시간(초) 동안 존재하다 자동 파괴(SetLifeSpan). 상승 애니메이션도 이 시간에 맞춰야 자연스럽다.
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float LifeSpanSeconds = 1.2f;

	// 위로 떠오르는 속도(cm/s) — 색상/폭발적 등장 등 나머지 연출은 WBP UMG Animation이 담당하고,
	// 월드 위치 자체를 움직이는 이 부분만 C++이 소유한다(MulticastPlayMovingParticleEffect와 같은 이유
	// — "게임 오브젝트의 위치 이동"이지 "위젯 레이아웃/스타일 애니메이션"이 아니라서 C++ 소관).
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float RiseSpeed = 80.0f;

private:
	UP1DamageNumberWidget* GetDamageNumberWidget() const;
};
