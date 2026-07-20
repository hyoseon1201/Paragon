// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "P1PlayerState.generated.h"

class UP1AbilitySystemComponent;
class UP1AttributeSet;
class UCurveTable;
class UDataTable;
class UGameplayEffect;

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

	// 레벨업마다 1씩 지급되는 스킬 강화 포인트. 소비(SpendSkillPoint)는
	// UP1AbilitySystemComponent::ServerInvestSkillPoint()가 호출한다(포인트 검증 + 소비를 그쪽 로직과
	// 같은 서버 함수 안에서 원자적으로 처리하기 위해, 여기서는 0 이하로 내려가지 않도록만 방어).
	int32 GetSkillPoints() const { return SkillPoints; }
	void AddSkillPoint() { if (HasAuthority()) { ++SkillPoints; OnSkillPointsChangedNative.Broadcast(SkillPoints); } }
	void SpendSkillPoint() { if (HasAuthority() && SkillPoints > 0) { --SkillPoints; OnSkillPointsChangedNative.Broadcast(SkillPoints); } }

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

	// --- 상점 / 인벤토리 ---
	// 아이템 정의(가격/이름/아이콘)는 ShopItemTable(DataTable, Row=FP1ShopItemData)에 데이터로만
	// 존재 — 별도 "상점" 액터/매니저는 없다(칼바람처럼 벤더별로 다른 재고가 있는 게 아니라 어디서든
	// 열리는 전역 카탈로그라 상태를 들고 있을 이유가 없음). 보유 아이템은 중복 허용 플랫 배열
	// (같은 아이템 2개 사면 엔트리 2개) — 스택 수량 UI가 필요해지면 그때 TMap으로 바꿔도 늦지 않다.
	const TArray<FName>& GetInventory() const { return Inventory; }

	DECLARE_MULTICAST_DELEGATE(FOnInventoryChangedNative);
	FOnInventoryChangedNative OnInventoryChangedNative;

	// 골드 검증(충분한지)+차감/환불+인벤토리 반영을 한 서버 함수 안에서 원자적으로 처리한다.
	// UP1AbilitySystemComponent::ServerInvestSkillPoint()와 같은 이유로 GameplayAbility가 아니라
	// 그냥 Server RPC — 전투 예측/쿨다운이 필요 없는 경제 액션이라 GAS 어빌리티 머신러리가 과함.
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBuyItem(FName ItemRowName);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSellItem(FName ItemRowName);

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

	// 1레벨에 Q/E/RMB 중 하나를 바로 선택해서 투자할 수 있어야 하므로 시작값 1(LoL과 동일하게
	// 캐릭터 1레벨 자체에 스킬 포인트 1개가 딸려온다 — 레벨업해야만 포인트가 생기는 게 아님).
	UPROPERTY(ReplicatedUsing = OnRep_SkillPoints)
	int32 SkillPoints = 1;

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

	// Row=FP1ShopItemData(P1ShopTypes.h). RowName이 곧 아이템 ID(Inventory/ServerBuyItem/ServerSellItem이
	// 참조하는 FName과 동일).
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TObjectPtr<UDataTable> ShopItemTable;

	// Duration=Instant, Gold Modifier=Additive, Magnitude=SetByCaller Data.Gold.Flat — 킬/어시스트 보상이
	// 쓰는 것과 같은 GE 애셋을 재사용 가능(부호만 다르게: 구매는 음수, 판매 환불은 양수).
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UGameplayEffect> GoldAdjustEffectClass;

	// 장비 아이템 슬롯 수(LoL 아레나 스타일 — 컴포넌트 합성 없이 완성 아이템만 직접 구매). 가득 차면
	// ServerBuyItem이 거부한다 — 새 아이템을 사려면 먼저 하나를 팔아야 함.
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	int32 MaxInventorySlots = 6;

	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	TArray<FName> Inventory;

	UFUNCTION()
	void OnRep_Inventory() { OnInventoryChangedNative.Broadcast(); }

	// 아이템ID → 그 StatEffectClass가 적용된 FActiveGameplayEffectHandle. 판매 시 이 핸들로 정확히
	// 그 효과만 제거하기 위한 서버 전용 북키핑 — 클라이언트는 Inventory(FName 배열)만 보면 되므로
	// 복제 안 함(같은 아이템 중복 보유가 금지돼 있어 FName 하나당 핸들 하나로 충분히 안전하다).
	TMap<FName, FActiveGameplayEffectHandle> ActiveItemStatEffects;

	// Instant GE 하나로 골드를 가감(Delta 부호로 증감 결정) — ApplyFlatRestoreEffect(AP1HeroCharacter)와
	// 같은 패턴이지만 PlayerState는 자기 ASC를 직접 갖고 있어 Character를 거칠 필요가 없다.
	void ApplyGoldDelta(float Delta);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
