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

	// 스코어보드 등에서 "이 플레이어가 어떤 영웅을 플레이 중인지" 표시할 때 쓴다. AP1HeroCharacter::
	// HandleAbilitySystemReady()가 Possess 시점마다 서버에서 채워준다. PlayerState::GetPawn()으로
	// Pawn을 거쳐 매번 조회하는 방식은 안 쓴다 — Pawn→PlayerState 역방향(PlayerState->GetPawn())은
	// 소유 클라이언트 로컬에서만 채워지고 다른 클라이언트에는 복제되지 않아, 남의 정보를 봐야 하는
	// 스코어보드에서는 자기 자신 행만 뜨고 나머지는 비는 버그가 났다 — 그래서 값 자체를 PlayerState의
	// 복제 프로퍼티로 들고 다닌다(Pawn이 사망~리스폰 사이 없어도 값이 유지되는 부가 이점도 있음).
	FText GetHeroDisplayName() const { return HeroDisplayName; }
	void SetHeroDisplayName(const FText& NewName) { if (HasAuthority()) { HeroDisplayName = NewName; } }

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

	// --- 스턴 종료 시각(머리 위 스턴바 카운트다운용) ---
	// 서버가 스턴 적용 시 "스턴이 끝나는 서버 월드 시각"(GetWorld()->GetTimeSeconds() + 지속시간)을 여기
	// 기록한다. 이 값은 전 클라이언트에 복제되므로(Mixed 모드에서 스턴 GE 자체는 소유자에게만 가지만 이
	// 프로퍼티는 전원에게 감), 적을 보는 클라이언트도 `StunEndServerTime - GameState->GetServerWorldTimeSeconds()`
	// 로 정확한 남은시간을 계산할 수 있다 — 스턴바 표시/숨김은 State.Stunned '태그'(전원 복제)가 담당하고,
	// 이 값은 오직 카운트다운 정확도를 위해서만 쓴다. GetServerWorldTimeSeconds()는 GameState가 동기화하는
	// 서버 시계라 서버/클라 로컬 시계 차이 문제를 피한다(GetWorld()->GetTimeSeconds()를 직접 비교하면 안 됨).
	float GetStunEndServerTime() const { return StunEndServerTime; }
	void SetStunEndServerTime(float NewEndServerTime);   // 서버에서만 호출

	// StunEndServerTime이 클라에 도착한 순간(OnRep) 또는 서버에서 세팅된 순간에 발화 — 위젯 컨트롤러가
	// 이걸 받아 정확한 남은시간으로 스턴바를 다시 그린다. 태그와 이 값이 서로 다른 프레임에 도착해도(순서
	// 무보장) 각자 도착 시점에 RefreshStun을 트리거해 마지막엔 정확한 상태로 수렴하게 하는 게 핵심.
	DECLARE_MULTICAST_DELEGATE(FOnStunTimeChangedNative);
	FOnStunTimeChangedNative OnStunTimeChangedNative;

	// --- 상점 / 인벤토리 ---
	// 아이템 정의(가격/이름/아이콘)는 ShopItemTable(DataTable, Row=FP1ShopItemData)에 데이터로만
	// 존재 — 별도 "상점" 액터/매니저는 없다(칼바람처럼 벤더별로 다른 재고가 있는 게 아니라 어디서든
	// 열리는 전역 카탈로그라 상태를 들고 있을 이유가 없음). 보유 아이템은 중복 보유 불가 플랫 배열
	// (ServerBuyItem이 Inventory.Contains(ItemRowName)이면 거부).
	const TArray<FName>& GetInventory() const { return Inventory; }

	// 상점 UI가 전체 카탈로그를 그릴 때 씀 — RowName(=아이템 ID) 기준으로 FP1ShopItemData를 조회한다.
	UDataTable* GetShopItemTable() const { return ShopItemTable; }

	// 인벤토리 UI(하단 보유 아이템 6칸)가 몇 칸을 그려야 하는지 알아야 해서 클라이언트에도 노출.
	int32 GetMaxInventorySlots() const { return MaxInventorySlots; }

	DECLARE_MULTICAST_DELEGATE(FOnInventoryChangedNative);
	FOnInventoryChangedNative OnInventoryChangedNative;

	// 골드 검증(충분한지)+차감/환불+인벤토리 반영을 한 서버 함수 안에서 원자적으로 처리한다.
	// UP1AbilitySystemComponent::ServerInvestSkillPoint()와 같은 이유로 GameplayAbility가 아니라
	// 그냥 Server RPC — 전투 예측/쿨다운이 필요 없는 경제 액션이라 GAS 어빌리티 머신러리가 과함.
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBuyItem(FName ItemRowName);

	// 패시브 골드 수입 GE(Infinite+Period)를 매치당 딱 한 번만 적용한다(멱등 — bPassiveGoldIncomeStarted로
	// 중복 적용 방지). AP1HeroCharacter::HandleAbilitySystemReady()가 스폰마다(리스폰 포함) 호출하지만
	// 실제 적용은 최초 1회뿐이다. PassiveRegenEffectClass처럼 리스폰마다 재적용하는 방식을 일부러 안 쓴다 —
	// 그러면 사망~리스폰 사이에 GE가 재시작되면서 그 구간 골드 수입이 끊기는 버그가 생긴다(체력 리젠은
	// 죽어있는 동안 의미가 없어 무관하지만, 골드는 죽어있어도 계속 올라야 하는 게 의도). ASC 자체가
	// PlayerState 소유라 폰이 죽어 사라져도 파괴되지 않으므로, 한 번 적용된 Periodic GE는 계속 틱한다.
	void StartPassiveGoldIncome();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSellItem(FName ItemRowName);

protected:
	// VisibleAnywhere — BP_P1PlayerState의 Components 패널에 노출해 GenericCooldownReductionEffectClass
	// 같은 이 컴포넌트의 EditDefaultsOnly 프로퍼티를 에디터에서 지정할 수 있게 한다(FloatingStatusComponent와
	// 동일한 컨벤션). 이게 빠져있으면 컴포넌트 자체가 BP 에디터의 Components 트리에 안 나타난다.
	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UP1AbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UP1AttributeSet> AttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamId)
	FGenericTeamId MyTeamId;

	UFUNCTION()
	void OnRep_MyTeamId();

	UPROPERTY(ReplicatedUsing = OnRep_CharacterLevel)
	int32 CharacterLevel = 1;

	// RepNotify 불필요 — 스코어보드가 델리게이트 구독 없이 매 리프레시(Tab 홀드)마다 직접 폴링해서 읽음.
	UPROPERTY(Replicated)
	FText HeroDisplayName;

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

	// 스턴이 끝나는 서버 월드 시각(위 GetStunEndServerTime() 주석 참고). 전원에게 복제.
	UPROPERTY(ReplicatedUsing = OnRep_StunEndServerTime)
	float StunEndServerTime = 0.0f;

	UFUNCTION()
	void OnRep_StunEndServerTime() { OnStunTimeChangedNative.Broadcast(); }

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

	// Duration=Infinite, Period=N초, Gold Modifier=Additive 고정값 — StartPassiveGoldIncome() 참고.
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UGameplayEffect> PassiveGoldEffectClass;

	// 서버 전용 — StartPassiveGoldIncome()이 이미 적용했으면 다시 적용하지 않는다. 복제 불필요(클라이언트는
	// Gold 어트리뷰트 값만 보면 되고 "수입이 시작됐는지" 자체는 알 필요 없음).
	bool bPassiveGoldIncomeStarted = false;

	// 장비 아이템 슬롯 수(LoL 아레나 스타일 — 컴포넌트 합성 없이 완성 아이템만 직접 구매). 가득 차면
	// ServerBuyItem이 거부한다 — 새 아이템을 사려면 먼저 하나를 팔아야 함.
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	int32 MaxInventorySlots = 6;

	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	TArray<FName> Inventory;

	UFUNCTION()
	void OnRep_Inventory() { OnInventoryChangedNative.Broadcast(); }

	// 아이템ID → 그 아이템 구매로 적용된 모든 FActiveGameplayEffectHandle(깡스탯 StatEffectClass
	// 하나 + UniqueAbilities[].EffectClass 여러 개를 한꺼번에 담음). 판매 시 이 핸들들로 정확히 그
	// 효과들만 제거하기 위한 서버 전용 북키핑 — 클라이언트는 Inventory(FName 배열)만 보면 되므로
	// 복제 안 함(같은 아이템 중복 보유가 금지돼 있어 FName 하나당 이 배열 하나로 충분히 안전하다).
	TMap<FName, TArray<FActiveGameplayEffectHandle>> ActiveItemEffects;

	// Instant GE 하나로 골드를 가감(Delta 부호로 증감 결정) — ApplyFlatRestoreEffect(AP1HeroCharacter)와
	// 같은 패턴이지만 PlayerState는 자기 ASC를 직접 갖고 있어 Character를 거칠 필요가 없다.
	void ApplyGoldDelta(float Delta);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
