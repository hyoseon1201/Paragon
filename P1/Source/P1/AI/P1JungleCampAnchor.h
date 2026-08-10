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

	// 캠프 하나가 한 번에 스폰하는 몬스터 수(무리). 1이면 기존과 동일하게 앵커 위치에 단독 스폰,
	// 2 이상이면 PackSpawnRadius 반경의 원형 배치로 서로 겹치지 않게 각자 다른 위치에 스폰한다.
	// 각 몬스터의 HomeLocation은 앵커 중심이 아니라 자신이 실제로 스폰된 위치 — 리시 판정도 각자
	// 자기 자리 기준으로 독립적으로 이뤄진다.
	UPROPERTY(EditAnywhere, Category = "Camp", meta = (ClampMin = "1"))
	int32 MonsterCount = 1;

	// MonsterCount>=2일 때 몬스터끼리 겹치지 않도록 원형으로 흩뿌리는 반경(cm). MonsterCount==1이면 무시.
	UPROPERTY(EditAnywhere, Category = "Camp", meta = (ClampMin = "0.0", EditCondition = "MonsterCount > 1"))
	float PackSpawnRadius = 200.0f;

	// 몬스터가 죽은 뒤 같은 자리에 다시 스폰하기까지의 대기시간(초) — 무리 전체가 죽은 뒤(CurrentMonsters
	// 배열이 비는 시점)부터 카운트된다. 무리 중 일부만 죽었을 땐 타이머가 돌지 않는다(나머지가 계속 싸움).
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
	// 임포트). 미설정 시 항상 레벨 1로 스폰. AP1GameState::GetElapsedMatchTime()(매치 시작 시각 기준
	// 권위 있는 서버 시계)로 평가한다.
	UPROPERTY(EditDefaultsOnly, Category = "Camp")
	TObjectPtr<UCurveTable> MonsterLevelByMatchTimeTable;

public:
	// 무리 중 한 마리가 맞았을 때(AP1JungleMonsterCharacter::OnHitReactEventReceived) 호출 — 나머지
	// 생존 개체 전원(SourceMonster 본인 제외)에게도 같은 공격자를 타겟으로 어그로를 전파한다.
	void NotifyCampAggro(AActor* Attacker, AP1JungleMonsterCharacter* SourceMonster);

private:
	// 무리 전체(MonsterCount마리)를 스폰 — RespawnTimerHandle이 만료되면 이 함수가 다시 불려 무리
	// 전체를 재스폰한다(부분 리스폰 없음, 죽은 순서와 무관하게 항상 한 번에 다시 채움).
	void SpawnMonster();
	// 배열에서 죽은 몬스터를 제거하고, 무리 전체가 비었을 때만 리스폰 타이머를 시작한다.
	void OnMonsterDied(AP1JungleMonsterCharacter* DeadMonster);

	// MonsterCount>=2일 때 Index번째 몬스터를 앵커 중심으로부터 원형으로 흩뿌리기 위한 오프셋.
	// MonsterCount==1이면 항상 ZeroVector(기존 단독 스폰 동작과 동일).
	FVector ComputeSpawnOffset(int32 Index) const;

	// 현재 매치 경과 시간을 MonsterLevelByMatchTimeTable에 대입해 몬스터 레벨을 계산(1 미만으로는 안 내려감).
	int32 ComputeMonsterLevel() const;

	FTimerHandle RespawnTimerHandle;

	// 현재 살아있는 무리 구성원 전체 — 죽으면 OnMonsterDied가 제거하고, 비면 리스폰 타이머가 돈다.
	UPROPERTY()
	TArray<TObjectPtr<AP1JungleMonsterCharacter>> CurrentMonsters;
};
