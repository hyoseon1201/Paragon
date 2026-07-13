// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/P1PlayerState.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/CurveTable.h"

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
}
