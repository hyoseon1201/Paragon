// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/P1PlayerState.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

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
	}
}

void AP1PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AP1PlayerState, MyTeamId);
	DOREPLIFETIME(AP1PlayerState, CharacterLevel);
}
