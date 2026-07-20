// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/P1CharacterBase.h"
#include "AbilitySystemComponent.h"
#include "P1HeroCharacter.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UP1CameraComponent;
class UP1HeroComponent;
class UMotionWarpingComponent;
class UAnimMontage;
class UCurveTable;
class AP1PlayerState;
struct FGameplayEventData;

UCLASS()
class P1_API AP1HeroCharacter : public AP1CharacterBase
{
	GENERATED_BODY()

public:
	AP1HeroCharacter();

	// IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	// UP1AttributeSet::HandleKillRewards()에서 호출 — 서버 전용(호출 경로 자체가 서버 전용 Damage GE
	// 적용 안에서만 발생하므로 별도 IsNetAuthority 체크는 방어적 이중 체크에 가깝지만 명시적으로 넣어둔다).
	// VictimKillStreak/VictimTimeSinceLastDeath는 피해자가 죽기 직전 상태 — GoldRewardKillEffectClass의
	// MMC(UP1MMC_GoldKillBounty)가 현상금 보정에 사용한다. VictimLevel은 ChampionKillXPTable/
	// ChampionAssistXPTable 커브 조회 키(피해자 레벨이 높을수록 더 많은 경험치).
	void GrantKillReward(int32 VictimKillStreak, float VictimTimeSinceLastDeath, int32 VictimLevel);
	void GrantAssistReward(int32 VictimLevel);

	// UP1AttributeSet이 Experience 증가로 레벨업 임계치를 넘었는지 감지하면 호출 — 임계치를 넘는 동안
	// (한 번에 여러 레벨 상승 가능) 반복해서 레벨을 올리고, 매 레벨마다 ApplyBaseStatsForLevel(bFullHeal=false)로
	// 최대치 증가분만큼만 현재 체력/마나를 함께 늘린다(레벨업이 공짜 완전회복이 되면 안 되므로).
	void CheckLevelUp();

protected:
	// 머리 위 FloatingStatus 위젯에 표시할 영웅 이름(예: "Greystone"). 영웅 BP마다 설정.
	UPROPERTY(EditDefaultsOnly, Category = "Character")
	FText HeroDisplayName;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	// --- 성장/보상 (레벨업, 킬/어시스트) ---

	// Duration=Instant, Health/Mana Modifier=Additive, Magnitude=SetByCaller Data.Heal.Flat / Data.Mana.Flat.
	// 스폰/리스폰 시 완전 회복, 레벨업 시 최대치 증가분만큼 회복에 공용으로 쓴다(ApplyBaseStatsForLevel 참고).
	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	TSubclassOf<UGameplayEffect> HealEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	TSubclassOf<UGameplayEffect> ManaRestoreEffectClass;

	// Gold Modifier=Custom Calculation Class(UP1MMC_GoldKillBounty) — Data.KillStreak/Data.TimeSinceLastDeath를
	// SetByCaller로 읽어 현상금을 계산한다. 챔피언 킬 전용(어시스트는 아래 Flat 버전 사용).
	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	TSubclassOf<UGameplayEffect> GoldRewardKillEffectClass;

	// Gold Modifier=Additive, Magnitude=SetByCaller Data.Gold.Flat — 어시스트처럼 고정 액수 보상용.
	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	TSubclassOf<UGameplayEffect> GoldRewardFlatEffectClass;

	// Experience Modifier=Additive, Magnitude=SetByCaller Data.Experience.Flat — 킬/어시스트 공용, 액수만 다르게 호출.
	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	TSubclassOf<UGameplayEffect> ExperienceRewardEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	float AssistGoldAmount = 50.0f;

	// Row="ChampionKillXP"/"ChampionAssistXP", Time=피해자 CharacterLevel(1~18) — Data/CT_ChampionKillXP.json,
	// CT_ChampionAssistXP.json에서 임포트. GetChampionXPReward()가 조회한다.
	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	TObjectPtr<UCurveTable> ChampionKillXPTable;

	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	TObjectPtr<UCurveTable> ChampionAssistXPTable;

	// Table이 비어있으면(에셋 미설정) 0을 반환하지 않고 안전한 폴백 값을 쓰기 위한 커브 조회 헬퍼.
	float GetChampionXPReward(UCurveTable* Table, int32 VictimLevel) const;

	// DefaultAttributesEffect를 지정 레벨로 재적용(레벨 스케일 Scalable Float 커브 평가) 후,
	// bFullHeal=true면 현재 체력/마나를 최대치까지 완전 회복(스폰/리스폰용), false면 방금 오른
	// 최대치 증가분만큼만 회복(레벨업용 — 레벨업이 공짜 풀힐이 되면 안 됨).
	void ApplyBaseStatsForLevel(int32 Level, bool bFullHeal);

	// SetByCaller 태그 하나짜리 Instant GE를 자신에게 적용하는 공용 헬퍼(회복/마나복구/골드/경험치 보상 공유).
	void ApplyFlatRestoreEffect(TSubclassOf<UGameplayEffect> EffectClass, FGameplayTag SetByCallerTag, float Magnitude);

	// Health<=0 감지 시 적용하는 Duration GE — State.Dead 태그를 부여하며, Duration은 SetByCaller
	// Data.RespawnDelay(=RespawnDelay 값)로 주입한다. GE가 자연 만료되는 시점이 곧 리스폰 시점이다.
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TSubclassOf<UGameplayEffect> DeathEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	float RespawnDelay = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	// 기절 지속시간 내내 반복 재생할 몽타주(에셋 자체를 Loop로 설정) — DeathMontage와 동일한 패턴으로
	// 영웅마다 다른 애셋을 지정. State.Stunned 태그가 사라지는 순간(OnStunTagChanged) 정지한다.
	UPROPERTY(EditDefaultsOnly, Category = "CC")
	TObjectPtr<UAnimMontage> StunMontage;

	// PossessedBy(서버)/OnRep_PlayerState(클라) 양쪽에서 호출되는 오케스트레이터 — "ASC를 쓸 수 있게
	// 된 시점에 캐릭터가 해야 할 일 전체"를 순서대로 실행한다(GAS 와이어링, 사망/코스메틱 이벤트 구독,
	// 권위 전용 베이스스탯 적용, 로컬 전용 UI 생성). PossessedBy/OnRep_PlayerState가 이 이름을 직접
	// 호출하는 이유: 이 함수 하나가 "지금 이 시점에 ASC가 유효하다"는 계약을 보장하는 유일한 지점이라서다.
	void HandleAbilitySystemReady();

	// GAS ASC 와이어링 + 캐싱만 담당 — 이름 그대로 좁게 유지(다른 캐릭터 초기화 로직과 섞지 않음).
	void InitAbilityActorInfo(AP1PlayerState* P1PS, UAbilitySystemComponent* ASC);

	void AddDefaultAbilities();
	void BindMoveSpeedAttribute();

	FDelegateHandle MoveSpeedChangedDelegateHandle;

	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);

	// AttributeSet이 Health<=0을 감지해 보낸 이벤트 수신 — 서버에서만 DeathEffectClass를 자신에게 적용한다.
	// GenericGameplayEventCallbacks의 델리게이트 시그니처는 (const FGameplayEventData*) 하나뿐이다
	// (태그는 이미 맵의 키로 구분되므로 콜백 파라미터에는 실려오지 않는다).
	void OnDiedEventReceived(const FGameplayEventData* EventData);

	// State.Dead 태그 카운트 변경(GAS 태그 복제로 모든 클라이언트에서 호출됨) — 부여 시 코스메틱 반응
	// (입력 차단, 사망 몽타주), 제거(GE 자연 만료) 시 서버에서 RestartPlayer + 자신 Destroy.
	void OnDeadTagChanged(FGameplayTag Tag, int32 NewCount);

	// State.Stunned 태그 카운트 변경 — OnDeadTagChanged와 동일한 패턴. 부여 시 이동 입력 차단(어빌리티
	// 발동 차단은 이미 베이스 어빌리티의 ActivationBlockedTags가 처리하므로 여기선 이동만 신경 쓰면 됨)
	// + StunMontage 반복 재생, 제거 시 입력 복구 + 몽타주 정지.
	void OnStunTagChanged(FGameplayTag Tag, int32 NewCount);

	// DeathMontage의 착지 프레임(UP1AnimNotify_SendGameplayEvent, Event.Montage.Death.Impact)에서
	// 발신되는 이벤트 수신 — 래그돌 전환을 트리거한다. 태그 이벤트와 동일하게 각 클라이언트가
	// 독립적으로 반응(리플리케이션 불필요, 사후 순수 코스메틱이라 클라이언트마다 약간 다르게 쓰러져도 무방).
	void OnDeathImpactEventReceived(const FGameplayEventData* EventData);

	// 스켈레탈 메시 전체를 물리 시뮬레이션으로 전환해 래그돌처럼 쓰러지게 한다. 캡슐 충돌은 끄고
	// (죽은 캐릭터가 다른 플레이어를 막지 않도록) CharacterMovementComponent도 정지시킨다.
	// 리스폰 시 이 액터 자체가 Destroy()되므로 래그돌을 원상복구하는 로직은 따로 필요 없다.
	void StartRagdoll();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<UP1CameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<UP1HeroComponent> HeroComponent;

	// 도약/대시 스킬(RMB, R 등)의 루트모션 워프 타깃 처리.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;
};
