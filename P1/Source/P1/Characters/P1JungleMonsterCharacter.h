// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/P1CharacterBase.h"
#include "ActiveGameplayEffectHandle.h"
#include "P1JungleMonsterCharacter.generated.h"

class UAbilitySystemComponent;
class UP1AttributeSet;
class UGameplayEffect;
class UGameplayAbility;
class UAnimMontage;
struct FGameplayEventData;

// 정글 몬스터(중립 캠프) — ASC/AttributeSet을 PlayerState가 아니라 Pawn 자신이 직접 들고 있다
// (AP1CharacterBase.h 주석에 이미 예고된 패턴: "AP1MinionCharacter는 Pawn 자신에 있음"과 동일 컨벤션).
// TeamId는 기본값(255=NoTeam)을 그대로 둬서 별도 설정 없이 모든 영웅에게 적대 판정이 되게 한다.
//
// AI 쪽 FSM(Idle/Chase/Reset)은 AP1JungleMonsterAIController가 돌리는 Behavior Tree가 담당하고,
// 이 클래스는 그 BT가 참조/호출하는 "능력"만 제공한다 — 홈/리시 반경 데이터, 리시 복귀 시
// 무적+급속회복 시작/종료, 근접 공격 트리거, 그리고 맞았을 때 어그로를 AIController에 통지하는
// 이벤트 구독까지.
UCLASS()
class P1_API AP1JungleMonsterCharacter : public AP1CharacterBase
{
	GENERATED_BODY()

public:
	AP1JungleMonsterCharacter();

	virtual void Tick(float DeltaSeconds) override;

	// 캠프 스폰 지점 — 기본값은 BeginPlay 시점의 스폰 위치(레벨에 배치된 그대로)지만, 나중에 캠프
	// 앵커 액터/스포너가 생기면 스폰 직후 이 함수로 정확한 캠프 좌표를 덮어쓸 수 있다.
	void SetHomeLocation(const FVector& NewHomeLocation) { HomeLocation = NewHomeLocation; }
	FVector GetHomeLocation() const { return HomeLocation; }
	float GetLeashRadius() const { return LeashRadius; }
	// AP1JungleCampAnchor가 스폰 직후 캠프별 반경으로 덮어쓸 때 사용(0 이하로는 안 내려가게 호출부가 방어).
	void SetLeashRadius(float NewLeashRadius) { LeashRadius = NewLeashRadius; }

	// 죽었을 때(Event.Character.Died) 캠프 스포너(AP1JungleCampAnchor)가 리스폰 타이머를 시작할 수
	// 있도록 통지(즉시 브로드캐스트 — 리스폰 대기시간은 죽은 순간부터 카운트되는 게 자연스러움).
	// 히어로처럼 "그 자리에서 GE 만료까지 대기"하는 방식이 아니라, 캠프가 완전히 새 인스턴스를 다시
	// 스폰하는 방식(정글 몬스터 리스폰의 표준적인 형태 — 캐릭터가 사라졌다 나타나는 편이 자연스러움).
	// 실제 액터 파괴는 DeathMontage가 재생될 시간을 확보한 뒤 지연되므로(아래 OnDiedEventReceived
	// 참고) 델리게이트 발신 시점과 Destroy() 시점이 다를 수 있다.
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnMonsterDiedNative, AP1JungleMonsterCharacter* /*DeadMonster*/);
	FOnMonsterDiedNative OnMonsterDied;

	// 리시(원점 복귀) 시작 — 무적 부여 + 이후 Tick에서 빠른 회복 시작. BTTask_P1ReturnToCamp가 MoveTo
	// 시작 시 호출한다. ResetInvulnerabilityEffectClass가 비어있으면(에디터 설정 전) 무적 없이
	// 회복만 진행 — 조용히 스킵되므로 세팅 전에도 AI 흐름 자체는 깨지지 않는다.
	void EnterLeashRecovery();

	// 리시 종료 — 무적 해제 + 체력을 최대치로 확정. BTTask_P1ReturnToCamp가 홈 도착을 감지하면 호출한다.
	void ExitLeashRecovery();

	// BTTask_P1MeleeAttack이 매 틱 호출 — 실제 발동 여부/쿨다운 판정은 어빌리티 쪽
	// (UP1GameplayAbility_JungleMonsterMeleeAttack의 CooldownGameplayEffectClass)이 전담한다.
	void RequestMeleeAttack();

	// AP1JungleCampAnchor가 스폰 직전(SpawnActorDeferred~FinishSpawning 사이)에 호출 — 반드시
	// BeginPlay() 실행 전에 설정해야 ApplyDefaultAttributes()가 이 레벨로 GE를 적용한다. BeginPlay
	// 이후(즉시 스폰 SpawnActor 사용 등)에 호출하면 이미 늦다(투사체 바운스 설정과 동일한 타이밍 함정).
	void SetMonsterLevel(int32 NewLevel) { MonsterLevel = FMath::Max(1, NewLevel); }
	int32 GetMonsterLevel() const { return MonsterLevel; }

protected:
	virtual void BeginPlay() override;

	// 이 캐릭터가 데미지를 받고 생존했을 때(Event.Character.HitReact) 호출 — Instigator를 공격자
	// Pawn으로 풀어서 AIController에 어그로를 통지한다.
	void OnHitReactEventReceived(const FGameplayEventData* Payload);

	// Health<=0 감지(Event.Character.Died) 시 호출 — BT/이동 정지 + OnMonsterDied 브로드캐스트 +
	// 사망 몽타주 재생 후, DeathDestroyDelay만큼 뒤에 액터를 파괴한다(DeathMontage 미설정 시 즉시 파괴).
	void OnDiedEventReceived(const FGameplayEventData* Payload);

	// 서버에서 호출 — 전 클라이언트의 로컬 AnimInstance에서 DeathMontage를 재생한다. 이 캐릭터는
	// NetExecutionPolicy=ServerOnly인 어빌리티만 쓰고 소유 클라이언트도 없어서(AI 컨트롤러 소유),
	// raw Montage_Play로는 서버 화면에만 보이므로 코스메틱 Multicast가 필요하다(AP1CharacterBase의
	// MulticastPlayParticleEffect 등과 동일한 컨벤션).
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayDeathMontage();

	// 사망 시 재생할 몽타주. 미설정 시 몽타주 없이 즉시 파괴(리스폰 타이머는 어차피 죽는 순간 시작).
	UPROPERTY(EditDefaultsOnly, Category = "JungleMonster")
	TObjectPtr<UAnimMontage> DeathMontage;

	// DeathMontage가 재생될 시간을 확보하기 위한 파괴 지연(초). DeathMontage 미설정 시 무시됨(즉시 파괴).
	UPROPERTY(EditDefaultsOnly, Category = "JungleMonster", meta = (ClampMin = "0.0"))
	float DeathDestroyDelay = 2.0f;

	// 홈 기준 리시 반경(cm). 이 반경을 벗어나면 AI가 무조건 복귀한다 — 판정은 몬스터 자신의
	// HomeLocation과의 거리로만 이뤄지고 플레이어 위치는 전혀 관여하지 않는다(카이팅으로 반복
	// 재유인해도 흔들리지 않는 이유 — 상세 설계 배경은 UBTService_P1JungleLeashCheck 주석 참고).
	UPROPERTY(EditAnywhere, Category = "JungleMonster|Leash")
	float LeashRadius = 1500.0f;

	// 리시 복귀 중 초당 회복량(최대체력 대비 비율, 0~1). 예: 0.5 = 초당 최대체력의 50%.
	UPROPERTY(EditAnywhere, Category = "JungleMonster|Leash", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LeashRecoveryHealPercentPerSecond = 0.5f;

	// 리시 복귀 중 부여할 무적 GE(State.Invulnerable 태그 부여, UP1AttributeSet이 데미지를 완전 무시).
	// 미설정 시 무적 없이 회복만 진행.
	UPROPERTY(EditDefaultsOnly, Category = "JungleMonster|Leash")
	TSubclassOf<UGameplayEffect> ResetInvulnerabilityEffectClass;

	// 스폰 시 부여할 초기 스탯(Health/MaxHealth/PhysicalPower 등) — 미설정이면 UP1AttributeSet
	// 생성자 기본값(영웅 기준 수치)이 그대로 남는다. 몬스터 전용 수치를 쓰려면 반드시 설정할 것.
	// GE의 각 Modifier Magnitude를 Scalable Float(CurveTable, Row=어트리뷰트 이름, Time=MonsterLevel)로
	// 구성하면 MonsterLevel에 따라 값이 달라진다 — 히어로의 DefaultAttributesEffect와 동일한 패턴.
	UPROPERTY(EditDefaultsOnly, Category = "JungleMonster")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffectClass;

	// DefaultAttributesEffectClass를 평가할 레벨(1~18) — GE 스펙의 Level로 그대로 전달되어 Scalable
	// Float 커브를 이 값 기준으로 조회한다. 지금은 스폰 시 고정값이고, 나중에 매치 경과 시간 기반
	// 몬스터 레벨링 시스템이 붙으면 스폰 전에 이 값만 설정해주면 된다(재적용 로직은 그때 추가).
	UPROPERTY(EditAnywhere, Category = "JungleMonster", meta = (ClampMin = "1"))
	int32 MonsterLevel = 1;

	// 근접 공격 어빌리티 — BeginPlay에 GiveAbility, RequestMeleeAttack()이 이벤트로 트리거한다.
	UPROPERTY(EditDefaultsOnly, Category = "JungleMonster")
	TSubclassOf<UGameplayAbility> MeleeAttackAbilityClass;

private:
	void ApplyDefaultAttributes();

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UP1AttributeSet> AttributeSet;

	FVector HomeLocation = FVector::ZeroVector;

	// EnterLeashRecovery()~ExitLeashRecovery() 구간에서만 true — Tick의 빠른 회복 램프를 활성화한다.
	bool bIsLeashRecovering = false;

	FActiveGameplayEffectHandle ResetInvulnerabilityEffectHandle;
};
