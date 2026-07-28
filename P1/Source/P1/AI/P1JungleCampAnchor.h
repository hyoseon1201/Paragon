// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1JungleCampAnchor.generated.h"

class AP1JungleMonsterCharacter;
class UStaticMeshComponent;
class UCurveTable;

// 레벨에 직접 배치하는 정글 캠프 지점 — 시각 마커(바닥에 까는 표시용 메시, 예: 포탈 모양 데칼/메시)와
// 스폰 데이터(몬스터 클래스/리스폰 대기시간/리시 반경)를 하나의 액터로 묶은 단일 소스 오브 트루스.
// 별도의 "스폰 매니저" 싱글턴 없이, 캠프 하나하나가 자기 몬스터의 생사를 직접 관리한다(캠프가
// 서로 독립적이라 공용 매니저가 굳이 필요 없음).
//
// 흐름: BeginPlay(서버)에 첫 몬스터 스폰 → 몬스터의 OnMonsterDied(Event.Character.Died 브로드캐스트
// 직후 Destroy())를 구독 → 죽으면 RespawnDelay초 뒤 같은 자리에 새 인스턴스를 다시 스폰.
// 레벨에 배치되는 액터라 리플리케이션이 필요 없다(모든 클라이언트가 레벨 데이터로 동일하게 갖고
// 있음) — 실제로 리플리케이트되는 건 이 액터가 스폰하는 AP1JungleMonsterCharacter 쪽.
UCLASS()
class P1_API AP1JungleCampAnchor : public AActor
{
	GENERATED_BODY()

public:
	AP1JungleCampAnchor();

protected:
	virtual void BeginPlay() override;

	// 스폰할 몬스터 클래스 — BP_JungleMonster 등 AP1JungleMonsterCharacter의 BP 서브클래스를 지정.
	UPROPERTY(EditAnywhere, Category = "Camp")
	TSubclassOf<AP1JungleMonsterCharacter> MonsterClass;

	// 몬스터가 죽은 뒤 같은 자리에 다시 스폰하기까지의 대기시간(초).
	UPROPERTY(EditAnywhere, Category = "Camp", meta = (ClampMin = "0.0"))
	float RespawnDelay = 30.0f;

	// 0 이하면 몬스터 클래스 기본 리시 반경을 그대로 사용, 양수면 이 캠프에서만 리시 반경을 덮어쓴다
	// (캠프마다 개활지/구석 크기가 달라 리시 반경을 다르게 주고 싶을 때).
	UPROPERTY(EditAnywhere, Category = "Camp")
	float LeashRadiusOverride = 0.0f;

	// 캠프 위치를 표시하는 바닥 마커 — Static Mesh는 에디터에서 지정(예: 포탈 모양 메시). 콜리전 없이
	// 순수 시각용이며, 게임 중에도 계속 보이게 둬서 캠프 위치를 알려주는 필드 인디케이터 역할을 겸한다.
	UPROPERTY(VisibleAnywhere, Category = "Camp")
	TObjectPtr<UStaticMeshComponent> MarkerMeshComponent;

	// 매치 경과 시간(분)→몬스터 레벨 매핑 커브(Row="MonsterLevelByMatchTime", Data/CT_MonsterLevelByMatchTime.json
	// 임포트). 미설정 시 항상 레벨 1로 스폰. GameState에 정식 매치 타이머가 아직 없어서(로드맵 3번 항목,
	// 미착수) 지금은 GetWorld()->GetTimeSeconds()를 매치 경과 시간으로 대신 쓴다 — 나중에 GameState가
	// 생기면 그쪽의 권위 있는 매치 시계로 바꿔치기하면 된다(그 전까지는 레벨 시작 시점 = 매치 시작 시점
	// 이라는 전제가 성립하는 단일 아레나 구조라 근사치로 충분).
	UPROPERTY(EditDefaultsOnly, Category = "Camp")
	TObjectPtr<UCurveTable> MonsterLevelByMatchTimeTable;

private:
	void SpawnMonster();
	void OnMonsterDied(AP1JungleMonsterCharacter* DeadMonster);

	// 현재 매치 경과 시간을 MonsterLevelByMatchTimeTable에 대입해 몬스터 레벨을 계산(1 미만으로는 안 내려감).
	int32 ComputeMonsterLevel() const;

	FTimerHandle RespawnTimerHandle;

	UPROPERTY()
	TObjectPtr<AP1JungleMonsterCharacter> CurrentMonster;
};
