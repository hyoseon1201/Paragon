// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "P1AbilitySystemComponent.generated.h"

// 어빌리티 Spec의 DynamicAbilityTags에 입력 태그(예: InputTag.Ability.Q)를 달아두고,
// 입력 press/release가 들어오면 즉시 해당 태그를 가진 Spec을 찾아 활성화/이벤트 전달한다.
UCLASS()
class P1_API UP1AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	// AP1HeroCharacter::AddDefaultAbilities()가 서버에서 어빌리티 부여를 마치면 호출한다.
	// bAbilitiesGiven을 리플리케이트해 원격 클라이언트도 "내 어빌리티 스펙이 실제로 도착한 시점"을
	// 알 수 있게 한다 — 스킬 아이콘 UI처럼 GetActivatableAbilities()가 채워진 후에만 조립 가능한
	// 데이터가 있는데, InitAbilityActorInfo()(HUD 생성 포함)가 AddDefaultAbilities()보다 먼저
	// 호출되고, 원격 클라이언트는 스펙 리플리케이션 자체가 별도 타이밍이라 생성 시점 단순 조회로는
	// 부족하다.
	void SetAbilitiesGiven();
	bool AreAbilitiesGiven() const { return bAbilitiesGiven; }

	FSimpleMulticastDelegate AbilitiesGivenDelegate;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_AbilitiesGiven();

	UPROPERTY(ReplicatedUsing = OnRep_AbilitiesGiven)
	bool bAbilitiesGiven = false;
};
