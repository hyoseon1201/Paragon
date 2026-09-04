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
class AP1JungleCampAnchor;
class UCurveTable;
class UDataTable;
struct FGameplayEventData;

// 정글 몬스터(중립 캠프) — ASC/AttributeSet을 PlayerState가 아니라 Pawn 자신이 직접 들고 있다
// (AP1CharacterBase.h 주석에 이미 예고된 패턴: "AP1MinionCharacter는 Pawn 자신에 있음"과 동일 컨벤션).
// TeamId는 MonsterTeamId(254) 고정 — 몬스터끼리는 아군, 모든 히어로 팀과는 적대(자세한 배경은 위
// MonsterTeamId 선언부 주석 참고).
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

	// 전용 "몬스터" 팀 ID — 255(NoTeam)를 그대로 쓰면 IsSameTeam()의 "둘 중 하나라도 NoTeam이면
	// 무조건 적대" 규칙 때문에 몬스터끼리도 서로 적대로 판정돼(캠프 몬스터끼리 근접공격이 서로에게
	// 맞는 버그의 원인) 히어로 팀(0~NumTeams-1)과 절대 겹치지 않는 이 값으로 고정한다 — 몬스터끼리는
	// 같은 팀(아군, 안 맞음)이면서 모든 히어로 팀과는 계속 적대로 남는다.
	static constexpr uint8 MonsterTeamId = 254;

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

	// AP1JungleCampAnchor가 스폰 직후 자신을 등록 — 맞았을 때(OnHitReactEventReceived) 같은 캠프의
	// 나머지 무리에게도 어그로를 전파할 수 있게 역참조를 들고 있는다(단일 몬스터 캠프면 그냥 무시됨).
	// .cpp에서 정의(헤더에 인라인으로 두면 AP1JungleCampAnchor가 전방 선언뿐이라 TWeakObjectPtr 대입에
	// 필요한 전체 타입 정보가 없어 컴파일 에러 — 유니티 빌드에선 다른 파일이 우연히 전체 정의를 먼저
	// 끌어와서 가려져 있다가, 이 헤더만 단독 컴파일되는 상황에서 드러난 적이 있다).
	void SetOwningCampAnchor(AP1JungleCampAnchor* Anchor);

	// 처치 보상 계산 — UP1AttributeSet::HandleKillRewards()가 Health<=0 감지 시점에 호출한다.
	// RewardDataTable/MonsterTypeName 중 하나라도 비어있거나 해당 Row를 못 찾으면 0/0을 반환(조용한
	// 폴백 — 보상 데이터 미설정 몬스터도 빌드/플레이가 깨지지 않게).
	void GetKillReward(int32& OutGold, float& OutExperience) const;

protected:
	virtual void BeginPlay() override;

	// 이 캐릭터가 데미지를 받고 생존했을 때(Event.Character.HitReact) 호출 — Instigator를 공격자
	// Pawn으로 풀어서 AIController에 어그로를 통지한다.
	void OnHitReactEventReceived(const FGameplayEventData* Payload);

	// Health<=0 감지(Event.Character.Died) 시 호출 — BT/이동 정지 + OnMonsterDied 브로드캐스트 +
	// 사망 몽타주 재생 후, DeathDestroyDelay만큼 뒤에 액터를 파괴한다(DeathMontage 미설정 시 즉시 파괴).
	void OnDiedEventReceived(const FGameplayEventData* Payload);

	// State.Stunned 태그 카운트 변경 — 히어로는 AP1PlayerController::HandleMove()가 입력 단계에서
	// 이동을 막지만, 이 캐릭터는 BT로 움직여서 그 경로를 아예 안 거친다(AIController/PathFollowing이
	// 직접 이동을 처리) — 그래서 스턴을 맞아도 계속 걸어오는 버그가 있었다. 0→양수 전이 시
	// AIController::StopMovement()로 현재 이동을 즉시 멈추고 BrainComponent::PauseLogic()으로 BT
	// 틱 자체를 정지(사망 처리와 동일한 정지 방식, 다만 재개 가능), 양수→0 전이 시 ResumeLogic()으로
	// 재개(TargetActor 블랙보드 키가 그대로 남아있으면 Combat 브랜치가 자연스럽게 이어짐). 공격(어빌리티
	// 발동) 자체는 이미 베이스 UP1GameplayAbility의 ActivationBlockedTags(State.Stunned)로 막혀있으므로
	// 이동만 신경 쓰면 된다.
	void OnStunTagChanged(FGameplayTag Tag, int32 NewCount);

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

	// DefaultAttributesEffectClass를 평가할 레벨 — GE 스펙의 Level로 그대로 전달되어 Scalable Float 커브를
	// 이 값 기준으로 조회한다(체력/공격력 등이 레벨에 따라 커짐). AP1JungleCampAnchor가 매치 경과 시간
	// (CT_MonsterLevelByMatchTime)으로 계산해 스폰 직전(FinishSpawning 전) SetMonsterLevel()로 넣어주며,
	// 이후 바뀌지 않는다. 복제(COND_InitialOnly)는 머리 위 위젯의 레벨 표시용 — 안 하면 클라이언트는
	// 생성자 기본값 1만 보게 된다(실제로 겪은 버그).
	UPROPERTY(Replicated, EditAnywhere, Category = "JungleMonster", meta = (ClampMin = "1"))
	int32 MonsterLevel = 1;

	// 근접 공격 어빌리티 — BeginPlay에 GiveAbility, RequestMeleeAttack()이 이벤트로 트리거한다.
	UPROPERTY(EditDefaultsOnly, Category = "JungleMonster")
	TSubclassOf<UGameplayAbility> MeleeAttackAbilityClass;

	// Data/DT_MonsterGoldXP.json에서 임포트한 DataTable(Row Struct=FP1MonsterRewardData) — 이 몬스터의
	// 처치 보상(1레벨 기준 BaseGold/BaseExperience)을 MonsterTypeName 행에서 조회한다.
	UPROPERTY(EditDefaultsOnly, Category = "JungleMonster|Reward")
	TObjectPtr<UDataTable> RewardDataTable;

	// RewardDataTable에서 조회할 Row 이름(예: "Wolves", "Raptors") — DT_MonsterGoldXP.json의 "Name" 값과 일치해야 함.
	UPROPERTY(EditDefaultsOnly, Category = "JungleMonster|Reward")
	FName MonsterTypeName;

	// Data/CT_MonsterRewardMultiplier.json에서 임포트(Row="MonsterRewardMultiplier", Time=MonsterLevel) —
	// BaseGold/BaseExperience에 곱할 레벨 배율. 미설정 시 배율 1.0 고정(스케일링 없이 기본값 그대로 지급).
	UPROPERTY(EditDefaultsOnly, Category = "JungleMonster|Reward")
	TObjectPtr<UCurveTable> RewardMultiplierTable;

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

	// SetOwningCampAnchor()로 설정 — 레벨에 단독 배치돼 테스트하는 경우(캠프 앵커 없음) null일 수
	// 있으므로 항상 유효성 체크 후 사용.
	TWeakObjectPtr<AP1JungleCampAnchor> OwningCampAnchor;
};
