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
		OnCharacterLevelChangedNative.Broadcast(CharacterLevel);
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

	DOREPLIFETIME(AP1PlayerState, MyTeamId);
	DOREPLIFETIME(AP1PlayerState, CharacterLevel);
	DOREPLIFETIME(AP1PlayerState, SkillPoints);
	DOREPLIFETIME(AP1PlayerState, Kills);
	DOREPLIFETIME(AP1PlayerState, Deaths);
	DOREPLIFETIME(AP1PlayerState, Assists);
	DOREPLIFETIME(AP1PlayerState, KillStreak);
	DOREPLIFETIME(AP1PlayerState, Inventory);
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
	// 들고 있는 상태 자체가 존재하지 않는다(중복 구매 금지).
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
	OnInventoryChangedNative.Broadcast();

	// 깡스탯 GE 적용 — Infinite Duration이라 핸들을 들고 있다가 판매 시 그 핸들로만 정확히 제거한다.
	if (ItemData->StatEffectClass)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
			EffectContext.AddSourceObject(this);
			const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(
				ItemData->StatEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, EffectContext);
			ActiveItemStatEffects.Add(ItemRowName, Handle);
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

	// 구매 시 걸어둔 깡스탯 GE를 정확히 그 핸들로만 제거 — 같은 아이템 중복 보유가 금지돼 있어
	// FName 하나당 핸들 하나로 항상 안전하게 매칭된다.
	if (FActiveGameplayEffectHandle* Handle = ActiveItemStatEffects.Find(ItemRowName))
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			ASC->RemoveActiveGameplayEffect(*Handle);
		}
		ActiveItemStatEffects.Remove(ItemRowName);
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
