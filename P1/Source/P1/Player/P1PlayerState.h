// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "P1PlayerState.generated.h"

class UP1AbilitySystemComponent;
class UP1AttributeSet;

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

protected:
	UPROPERTY()
	TObjectPtr<UP1AbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UP1AttributeSet> AttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamId)
	FGenericTeamId MyTeamId;

	UFUNCTION()
	void OnRep_MyTeamId();

	UPROPERTY(Replicated)
	int32 CharacterLevel = 1;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
