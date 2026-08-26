// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/P1PlayerState.h"
#include "Player/P1ShopTypes.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Engine/CurveTable.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "P1.h"

AP1PlayerState::AP1PlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UP1AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UP1AttributeSet>(TEXT("AttributeSet"));

	MyTeamId = FGenericTeamId::NoTeam;

	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* AP1PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AP1PlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	if (HasAuthority())
	{
		MyTeamId = NewTeamId;
		MARK_PROPERTY_DIRTY_FROM_NAME(AP1PlayerState, MyTeamId, this);
	}
}

FGenericTeamId AP1PlayerState::GetGenericTeamId() const
{
	return MyTeamId;
}

void AP1PlayerState::OnRep_MyTeamId()
{
}

void AP1PlayerState::SetCharacterLevel(int32 NewLevel)
{
	if (HasAuthority())
	{
		CharacterLevel = FMath::Max(1, NewLevel);
		MARK_PROPERTY_DIRTY_FROM_NAME(AP1PlayerState, CharacterLevel, this);
		OnCharacterLevelChangedNative.Broadcast(CharacterLevel);
	}
}

void AP1PlayerState::SetStunEndServerTime(float NewEndServerTime)
{
	if (HasAuthority())
	{
		StunEndServerTime = NewEndServerTime;
		MARK_PROPERTY_DIRTY_FROM_NAME(AP1PlayerState, StunEndServerTime, this);
		// 서버(리슨서버/호스트)에선 OnRep이 안 불리므로 여기서 직접 브로드캐스트 — CharacterLevel 등과 동일 패턴.
		OnStunTimeChangedNative.Broadcast();
	}
}

float AP1PlayerState::GetXPRequiredForNextLevel() const
{
	if (!XPToNextLevelTable)
	{
		return 0.0f;
	}

	static const FString ContextString(TEXT("GetXPRequiredForNextLevel"));
	const FRealCurve* Curve = XPToNextLevelTable->FindCurve(FName(TEXT("XPToNextLevel")), ContextString, false);
	return Curve ? Curve->Eval(static_cast<float>(CharacterLevel)) : 0.0f;
}

void AP1PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 엔진 베이스 APlayerState::GetLifetimeReplicatedProps()(Score 등)와 동일한 패턴 — 매 넷업데이트마다
	// 무조건 비교하는 대신, Setter 쪽에서 MARK_PROPERTY_DIRTY_FROM_NAME으로 명시적으로 신고된 프로퍼티만
	// 비교/직렬화한다(Push Model). 여기 등록된 10개는 전부 그에 맞춰 각 Setter/변경 지점에 마킹을 추가해둠.
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AP1PlayerState, MyTeamId, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1PlayerState, HeroDisplayName, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1PlayerState, CharacterLevel, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1PlayerState, SkillPoints, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1PlayerState, Kills, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1PlayerState, Deaths, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1PlayerState, Assists, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1PlayerState, KillStreak, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1PlayerState, StunEndServerTime, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AP1PlayerState, Inventory, SharedParams);
}

void AP1PlayerState::ApplyGoldDelta(float Delta)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC) || !GoldAdjustEffectClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[Shop] ApplyGoldDelta: ASC 또는 GoldAdjustEffectClass 없음 (Delta=%.0f)"), Delta);
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GoldAdjustEffectClass, 1.0f, EffectContext);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Gold_Flat, Delta);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AP1PlayerState::StartPassiveGoldIncome()
{
	if (!HasAuthority() || bPassiveGoldIncomeStarted || !PassiveGoldEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	ASC->ApplyGameplayEffectToSelf(PassiveGoldEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, EffectContext);

	// 이후 리스폰마다 HandleAbilitySystemReady()가 다시 불러도 여기서 조용히 막힌다 — Infinite+Periodic
	// GE는 이미 ASC(PlayerState 소유, 폰 생사와 무관하게 생존)에 붙어 계속 틱 중이므로 재적용하면 안 된다.
	bPassiveGoldIncomeStarted = true;

	UE_LOG(LogP1, Log, TEXT("[Shop] 패시브 골드 수입 시작 — %s"), *GetName());
}

bool AP1PlayerState::ServerBuyItem_Validate(FName ItemRowName)
{
	return !ItemRowName.IsNone();
}

void AP1PlayerState::ServerBuyItem_Implementation(FName ItemRowName)
{
	if (!ShopItemTable)
	{
		UE_LOG(LogP1, Warning, TEXT("[Shop] ServerBuyItem: ShopItemTable 미설정 (%s)"), *ItemRowName.ToString());
		return;
	}

	// LoL 아레나 스타일 — 컴포넌트 합성 없이 완성 아이템만 직접 구매하므로, 같은 아이템을 두 개
	// 들고 있는 상태 자체가 존재하지 않는다(중복 구매 금지). 깡스탯뿐 아니라 나중에 아이템별 특수
	// 효과(패시브 등)가 생겼을 때 그게 중복 적용되는 걸 막는 가장 간단한 방법이기도 하다 — 중복
	// 구매를 허용하면 "깡스탯은 스택되지만 특수효과는 한 번만 적용" 같은 걸 GE를 둘로 쪼개서
	// 스택 리밋을 다르게 주고, 판매 시 제거 조건도 따로 관리해야 해서 더 복잡해진다.
	if (Inventory.Contains(ItemRowName))
	{
		UE_LOG(LogP1, Warning, TEXT("[Shop] ServerBuyItem: 이미 보유 중인 아이템 — %s"), *ItemRowName.ToString());
		return;
	}

	if (Inventory.Num() >= MaxInventorySlots)
	{
		UE_LOG(LogP1, Warning, TEXT("[Shop] ServerBuyItem: 인벤토리 가득참(%d/%d) — %s"),
			Inventory.Num(), MaxInventorySlots, *ItemRowName.ToString());
		return;
	}

	static const FString ContextString(TEXT("ServerBuyItem"));
	const FP1ShopItemData* ItemData = ShopItemTable->FindRow<FP1ShopItemData>(ItemRowName, ContextString, /*bWarnIfRowMissing=*/false);
	if (!ItemData)
	{
		UE_LOG(LogP1, Warning, TEXT("[Shop] ServerBuyItem: 존재하지 않는 아이템 — %s"), *ItemRowName.ToString());
		return;
	}

	if (!IsValid(AttributeSet) || AttributeSet->GetGold() < ItemData->Price)
	{
		UE_LOG(LogP1, Warning, TEXT("[Shop] ServerBuyItem: 골드 부족 — %s (필요=%d, 보유=%.0f)"),
			*ItemRowName.ToString(), ItemData->Price, AttributeSet ? AttributeSet->GetGold() : 0.0f);
		return;
	}

	ApplyGoldDelta(-static_cast<float>(ItemData->Price));
	Inventory.Add(ItemRowName);
	MARK_PROPERTY_DIRTY_FROM_NAME(AP1PlayerState, Inventory, this);
	OnInventoryChangedNative.Broadcast();

	// 깡스탯 GE + 고유 능력 GE들을 전부 적용 — 다들 Infinite Duration이라 핸들을 들고 있다가 판매 시
	// 그 핸들들로만 정확히 제거한다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		TArray<FActiveGameplayEffectHandle> AppliedHandles;
		TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		if (ItemData->StatEffectClass)
		{
			const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(
				ItemData->StatEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, EffectContext);
			UE_LOG(LogP1, Log, TEXT("[Shop]   StatEffectClass 적용 — %s | GE=%s | 핸들유효=%d"),
				*ItemRowName.ToString(), *ItemData->StatEffectClass->GetName(), Handle.IsValid());
			AppliedHandles.Add(Handle);
		}
		else
		{
			UE_LOG(LogP1, Warning, TEXT("[Shop]   StatEffectClass 미설정 — %s | DT_ShopItems 행의 StatEffectClass를 DataTable 에디터에서 지정해야 함"),
				*ItemRowName.ToString());
		}

		for (const FP1ItemUniqueAbility& Ability : ItemData->UniqueAbilities)
		{
			if (Ability.EffectClass)
			{
				const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(
					Ability.EffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, EffectContext);
				UE_LOG(LogP1, Log, TEXT("[Shop]   UniqueAbility GE 적용 — %s | Ability=%s | GE=%s | 핸들유효=%d"),
					*ItemRowName.ToString(), *Ability.AbilityName.ToString(), *Ability.EffectClass->GetName(), Handle.IsValid());
				AppliedHandles.Add(Handle);
			}
			else
			{
				UE_LOG(LogP1, Warning, TEXT("[Shop]   UniqueAbility EffectClass 미설정 — %s | Ability=%s"),
					*ItemRowName.ToString(), *Ability.AbilityName.ToString());
			}

			if (Ability.TriggeredAbilityClass)
			{
				const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(Ability.TriggeredAbilityClass, 1));
				UE_LOG(LogP1, Log, TEXT("[Shop]   UniqueAbility 어빌리티 부여 — %s | Ability=%s | Class=%s | 핸들유효=%d"),
					*ItemRowName.ToString(), *Ability.AbilityName.ToString(), *Ability.TriggeredAbilityClass->GetName(), Handle.IsValid());
				GrantedAbilityHandles.Add(Handle);
			}
		}

		if (AppliedHandles.Num() > 0)
		{
			ActiveItemEffects.Add(ItemRowName, MoveTemp(AppliedHandles));
		}

		if (GrantedAbilityHandles.Num() > 0)
		{
			ActiveItemAbilities.Add(ItemRowName, MoveTemp(GrantedAbilityHandles));
		}
	}

	UE_LOG(LogP1, Log, TEXT("[Shop] 구매 — %s (가격=%d, 보유 아이템=%d개)"),
		*ItemRowName.ToString(), ItemData->Price, Inventory.Num());
}

bool AP1PlayerState::ServerSellItem_Validate(FName ItemRowName)
{
	return !ItemRowName.IsNone();
}

void AP1PlayerState::ServerSellItem_Implementation(FName ItemRowName)
{
	if (!ShopItemTable)
	{
		UE_LOG(LogP1, Warning, TEXT("[Shop] ServerSellItem: ShopItemTable 미설정 (%s)"), *ItemRowName.ToString());
		return;
	}

	if (!Inventory.RemoveSingle(ItemRowName))
	{
		UE_LOG(LogP1, Warning, TEXT("[Shop] ServerSellItem: 보유하지 않은 아이템 — %s"), *ItemRowName.ToString());
		return;
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(AP1PlayerState, Inventory, this);

	// 구매 시 걸어둔 깡스탯+고유 능력 GE들을 정확히 그 핸들들로만 제거 — 같은 아이템 중복 보유가
	// 금지돼 있어 FName 하나당 핸들 목록 하나로 항상 안전하게 매칭된다.
	if (TArray<FActiveGameplayEffectHandle>* Handles = ActiveItemEffects.Find(ItemRowName))
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			for (const FActiveGameplayEffectHandle& Handle : *Handles)
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
		}
		ActiveItemEffects.Remove(ItemRowName);
	}

	// 구매 시 부여한 온-히트 아이템 어빌리티들도 정확히 그 핸들들로만 회수.
	if (TArray<FGameplayAbilitySpecHandle>* AbilityHandles = ActiveItemAbilities.Find(ItemRowName))
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			for (const FGameplayAbilitySpecHandle& Handle : *AbilityHandles)
			{
				ASC->ClearAbility(Handle);
			}
		}
		ActiveItemAbilities.Remove(ItemRowName);
	}

	static const FString ContextString(TEXT("ServerSellItem"));
	const FP1ShopItemData* ItemData = ShopItemTable->FindRow<FP1ShopItemData>(ItemRowName, ContextString, /*bWarnIfRowMissing=*/false);
	if (ItemData && ItemData->SellPrice > 0)
	{
		ApplyGoldDelta(static_cast<float>(ItemData->SellPrice));
	}

	OnInventoryChangedNative.Broadcast();

	UE_LOG(LogP1, Log, TEXT("[Shop] 판매 — %s (환불=%d, 남은 아이템=%d개)"),
		*ItemRowName.ToString(), ItemData ? ItemData->SellPrice : 0, Inventory.Num());
}
