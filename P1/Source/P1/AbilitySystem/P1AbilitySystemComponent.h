// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityRepAnimMontage.h"
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

	// 스킬 아이콘의 "포인트 투자" 버튼 클릭 → 이 InputTag를 가진 어빌리티 스펙의 Level을 1 올리고
	// PlayerState의 SkillPoints를 1 소비한다. 이미 최대 레벨(UP1GameplayAbility::MaxAbilityLevel)이거나
	// 남은 포인트가 없으면 조용히 무시(서버 권위 검증 — 버튼 Visibility는 클라이언트 UI 힌트일 뿐 신뢰 안 함).
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerInvestSkillPoint(FGameplayTag AbilityInputTag);

	// AP1HeroCharacter::AddDefaultAbilities()가 서버에서 어빌리티 부여를 마치면 호출한다.
	// bAbilitiesGiven을 리플리케이트해 원격 클라이언트도 "내 어빌리티 스펙이 실제로 도착한 시점"을
	// 알 수 있게 한다 — 스킬 아이콘 UI처럼 GetActivatableAbilities()가 채워진 후에만 조립 가능한
	// 데이터가 있는데, InitAbilityActorInfo()(HUD 생성 포함)가 AddDefaultAbilities()보다 먼저
	// 호출되고, 원격 클라이언트는 스펙 리플리케이션 자체가 별도 타이밍이라 생성 시점 단순 조회로는
	// 부족하다.
	void SetAbilitiesGiven();
	bool AreAbilitiesGiven() const { return bAbilitiesGiven; }

	FSimpleMulticastDelegate AbilitiesGivenDelegate;

	// 특정 InputTag에 바인딩된 어빌리티의 남은 쿨다운을 퍼센트만큼 줄인다("아직 쿨다운 중이 아님"이면
	// 조용히 무시). 어빌리티마다 전용 GE를 따로 준비할 필요가 없다 — GenericCooldownReductionEffectClass
	// 하나(Duration=SetByCaller Data.CooldownDuration, 고정 Granted Tags 없음)를 그 어빌리티의 실제
	// 쿨다운 태그(UGameplayAbility::GetCooldownTags())로 동적 태깅해서 재사용한다(AssaultTheGates가
	// 자기 자신에게만 하던 "남은시간 재계산 후 제거+재적용"을 여러 어빌리티에 범용으로 적용한 버전).
	// 애쉬브링어(아이템) 고유효과 "크로노 스트라이크"가 첫 사용처 — UP1GameplayAbility_Item_ChronoStrike 참고.
	void ReduceCooldownByInputTag(FGameplayTag InputTag, float Percent);

	// RepAnimMontageInfo(GAS 내장 몽타주 리플리케이션 구조체)의 대역폭 최적화 스위치.
	// GetRepAnimMontageInfo_Mutable()이 엔진 UAbilitySystemComponent에서 protected라 하위 클래스인
	// 이 컴포넌트를 거쳐야만 어빌리티 코드(예: MeleeAttack)에서 호출할 수 있다.
	void SetRepAnimPositionMethod(ERepAnimPositionMethod InMethod);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// ReduceCooldownByInputTag()가 재적용에 쓰는 공용 쿨다운 GE — Duration Policy=Has Duration,
	// Duration Magnitude=Set by Caller(Data.CooldownDuration), Granted Tags는 에셋에 고정하지 않는다
	// (런타임에 대상 어빌리티의 실제 쿨다운 태그를 DynamicGrantedTags로 실어 적용하므로).
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	TSubclassOf<class UGameplayEffect> GenericCooldownReductionEffectClass;

	UFUNCTION()
	void OnRep_AbilitiesGiven();

	UPROPERTY(ReplicatedUsing = OnRep_AbilitiesGiven)
	bool bAbilitiesGiven = false;
};
