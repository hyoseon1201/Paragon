// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayTagContainer.h"
#include "P1CharacterBase.generated.h"

class UAbilitySystemComponent;
class UWidgetComponent;
class UP1FloatingStatusWidget;
class UP1FloatingStatusWidgetController;

UCLASS(Abstract)
class P1_API AP1CharacterBase : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AP1CharacterBase();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return CachedAbilitySystemComponent; }

	static bool IsSameTeam(const AActor* A, const AActor* B);

	// 캐릭터 유형 태그 반환 (Character.Type.*).
	FGameplayTag GetCharacterType() const { return CharacterType; }

	// 영웅 또는 보스인지 — RMB "영웅/보스 적중 시" 조건 등에서 사용.
	bool IsHeroOrBoss() const;

	// IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(TeamId); }

protected:
	// 팀 식별자. 0=팀1, 1=팀2, 255=NoTeam. 각 캐릭터 BP에서 설정.
	// 추후 GameMode가 서버에서 할당하는 방식으로 교체 예정.
	UPROPERTY(EditAnywhere, Category = "Team")
	uint8 TeamId = 255;

	// 캐릭터 유형. 생성자에서 Character.Type.Hero로 기본 설정.
	// 미니언/보스 BP에서 각각 Minion/Boss로 변경.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	FGameplayTag CharacterType;

	// ASC의 원본은 AP1HeroCharacter는 PlayerState, AP1MinionCharacter는 Pawn 자신에 있음.
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;

	// 머리 위 월드스페이스 HP/MP 바. BP에서 WidgetClass를 WBP_FloatingStatus로 설정.
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> FloatingStatusComponent;

	// FloatingStatusWidget에 데이터를 공급하는 per-character 컨트롤러.
	UPROPERTY()
	TObjectPtr<UP1FloatingStatusWidgetController> FloatingStatusWidgetController;
};
