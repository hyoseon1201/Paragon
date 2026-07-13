// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "P1PlayerState.generated.h"

class UP1AbilitySystemComponent;
class UP1AttributeSet;
class UCurveTable;

UCLASS()
class P1_API AP1PlayerState : public APlayerState, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AP1PlayerState();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UP1AttributeSet* GetAttributeSet() const { return AttributeSet; }
	UP1AbilitySystemComponent* GetP1AbilitySystemComponent() const { return AbilitySystemComponent; }

	// IGenericTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

	// 캐릭터(영웅) 레벨 — 개별 어빌리티 레벨(FGameplayAbilitySpec::Level, 스킬 랭크 개념)과는 별개의 축.
	// 아직 실제 레벨업 시스템이 없어 항상 1로 시작하지만, Stoicism 패시브처럼 "스킬 랭크가 아니라
	// 캐릭터 레벨(1~18)에 따라 값이 달라져야 하는" 어빌리티가 참조할 자리를 미리 마련해둔 것 —
	// 나중에 레벨업 시스템이 SetCharacterLevel()만 호출해주면 된다.
	int32 GetCharacterLevel() const { return CharacterLevel; }
	void SetCharacterLevel(int32 NewLevel);

	// HUD(레벨/KDA/스킬포인트) 갱신용 네이티브 델리게이트 — GAS 어트리뷰트가 아닌 plain 복제 int라
	// GetGameplayAttributeValueChangeDelegate() 경로를 못 쓰므로 직접 브로드캐스트한다. 값을 바꾸는
	// 지점(서버, Setter 내부)과 OnRep(클라이언트) 양쪽에서 모두 호출 — GAS Attribute의 Set()이
	// 서버/클라 양쪽에서 델리게이트를 쏘는 것과 동일한 이유(리슨서버가 호스트를 겸할 경우 OnRep이
	// 로컬에서 발동하지 않으므로 Setter 쪽 즉시 호출이 없으면 호스트 자신의 HUD만 갱신되지 않는다).
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLevelChangedNative, int32 /*NewLevel*/);
	FOnLevelChangedNative OnCharacterLevelChangedNative;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSkillPointsChangedNative, int32 /*NewValue*/);
	FOnSkillPointsChangedNative OnSkillPointsChangedNative;

	// Kills/Deaths/Assists는 항상 "K / D / A" 한 덩어리로 표시되므로 하나로 묶어 브로드캐스트한다.
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnKDAChangedNative, int32 /*Kills*/, int32 /*Deaths*/, int32 /*Assists*/);
	FOnKDAChangedNative OnKDAChangedNative;

	// 현재 레벨에서 다음 레벨로 가는 데 필요한 경험치(Data/CT_XPToNextLevel.json → XPToNextLevelTable로
	// 임포트, RowName="XPToNextLevel", Time=현재 레벨). 테이블 미설정이거나 이미 최대 레벨이면 0(레벨업 불가).
	float GetXPRequiredForNextLevel() const;

	// 레벨업마다 1씩 지급되는 스킬 강화 포인트 — 실제 "포인트를 써서 어빌리티 랭크 올리기" UI/로직은
	// 별도 후속 작업. 지금은 누적치만 정확히 추적해둔다.
	int32 GetSkillPoints() const { return SkillPoints; }
	void AddSkillPoint() { if (HasAuthority()) { ++SkillPoints; OnSkillPointsChangedNative.Broadcast(SkillPoints); } }

	// --- 킬/데스/어시스트 (전투 보상 시스템) ---
	int32 GetKills() const { return Kills; }
	int32 GetDeaths() const { return Deaths; }
	int32 GetAssists() const { return Assists; }
	// 죽지 않고 연속으로 처치한 횟수. 사망 시 0으로 리셋(AddDeath 참고) — 현상금(킬 보상) 계산에 사용.
	int32 GetKillStreak() const { return KillStreak; }
	// 마지막으로 죽은 시각(GetWorld()->GetTimeSeconds() 기준). 서버 전용 북키핑이라 복제하지 않는다 —
	// "얼마나 오래 안 죽었는지"는 현상금 계산 시점에 서버가 직접 계산하면 되고 클라가 알 필요 없다.
	float GetLastDeathTime() const { return LastDeathTime; }

	// UP1AttributeSet::HandleKillRewards()에서만 호출 — 서버 전용(Damage GE는 항상 서버에서만 적용되므로
	// 이 함수들이 클라에서 불릴 일 자체가 없다).
	void AddKill() { ++Kills; ++KillStreak; OnKDAChangedNative.Broadcast(Kills, Deaths, Assists); }
	void AddDeath() { ++Deaths; KillStreak = 0; if (const UWorld* World = GetWorld()) { LastDeathTime = World->GetTimeSeconds(); } OnKDAChangedNative.Broadcast(Kills, Deaths, Assists); }
	void AddAssist() { ++Assists; OnKDAChangedNative.Broadcast(Kills, Deaths, Assists); }

protected:
	UPROPERTY()
	TObjectPtr<UP1AbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UP1AttributeSet> AttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamId)
	FGenericTeamId MyTeamId;

	UFUNCTION()
	void OnRep_MyTeamId();

	UPROPERTY(ReplicatedUsing = OnRep_CharacterLevel)
	int32 CharacterLevel = 1;

	UPROPERTY(ReplicatedUsing = OnRep_SkillPoints)
	int32 SkillPoints = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Kills)
	int32 Kills = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Deaths)
	int32 Deaths = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Assists)
	int32 Assists = 0;

	UPROPERTY(Replicated)
	int32 KillStreak = 0;

	UFUNCTION()
	void OnRep_CharacterLevel() { OnCharacterLevelChangedNative.Broadcast(CharacterLevel); }
	UFUNCTION()
	void OnRep_SkillPoints() { OnSkillPointsChangedNative.Broadcast(SkillPoints); }
	UFUNCTION()
	void OnRep_Kills() { OnKDAChangedNative.Broadcast(Kills, Deaths, Assists); }
	UFUNCTION()
	void OnRep_Deaths() { OnKDAChangedNative.Broadcast(Kills, Deaths, Assists); }
	UFUNCTION()
	void OnRep_Assists() { OnKDAChangedNative.Broadcast(Kills, Deaths, Assists); }

	// 복제 안 함 — 서버만 알면 되는 값(위 GetLastDeathTime() 주석 참고).
	float LastDeathTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	TObjectPtr<UCurveTable> XPToNextLevelTable;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
