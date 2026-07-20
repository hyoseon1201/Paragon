// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "P1GameplayAbility.generated.h"

class AP1CharacterBase;
class AP1PlayerController;
class UGameplayEffect;
class UTexture2D;

// 프로젝트의 모든 GameplayAbility가 상속하는 베이스 클래스.
UCLASS(Abstract)
class P1_API UP1GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility();

	// BP 서브클래스에서 설정. GiveAbility 시 이 태그를 Spec.DynamicAbilityTags에 추가해
	// AbilityInputTagPressed/Released 라우팅에 사용한다.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTag InputTag;

	// 스킬 아이콘 UI에 표시할 텍스처 — UP1OverlayWidgetController가 InputTag와 함께 HUD에 전달한다.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UTexture2D> AbilityIcon;

	// 스킬 포인트로 투자 가능한 최대 레벨(FGameplayAbilitySpec::Level 기준). 기본값 1 = 투자 불가
	// (기본공격/패시브 등 — 시작부터 이미 최대 레벨이라 투자 버튼이 뜰 일이 없다). 투자 가능한
	// 어빌리티(RMB/Q/E는 5, R은 3)는 BP에서 이 값을 설정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Leveling")
	int32 MaxAbilityLevel = 1;

	// 인덱스 i = "스펙 레벨을 (i+1)로 올리는 데 필요한 최소 캐릭터 레벨". 예: RMB/Q/E처럼 캐릭터 레벨
	// 제한이 아예 없는 어빌리티는 빈 배열로 두면 된다(범위를 벗어난 인덱스는 항상 1=제한없음 취급).
	// R(궁극기)처럼 6/11/15 레벨에 한 랭크씩 풀리는 경우 {6, 11, 15}로 BP에서 설정한다.
	// MaxAbilityLevel<=1(투자 불가 어빌리티)에는 아무 의미가 없다.
	UPROPERTY(EditDefaultsOnly, Category = "Leveling")
	TArray<int32> RequiredCharacterLevelPerRank;

	// CurrentSpecLevel(0=미투자)에서 다음 랭크로 올리는 데 필요한 최소 캐릭터 레벨.
	// RequiredCharacterLevelPerRank가 비어있거나 해당 인덱스가 없으면 1(제한 없음)을 반환.
	int32 GetRequiredCharacterLevelForNextRank(int32 CurrentSpecLevel) const;

	// true면 AP1HeroCharacter::AddDefaultAbilities()가 GiveAbility 대신 GiveAbilityAndActivateOnce로
	// 부여해 그 즉시 1회 활성화한다 — 입력도 트리거 이벤트도 없이 부여되자마자 스스로 계속 돌아야 하는
	// 상시 패시브(Stoicism Vitality 등)용. 입력 바인딩이 있거나 이벤트로 트리거되는 어빌리티는 false(기본값).
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	bool bActivateOnGranted = false;

	// 스킬 아이콘 UI가 "쿨다운처럼" 구독할 태그. 기본값은 진짜 GAS 쿨다운(GetCooldownTags()의 첫 태그) —
	// CooldownGameplayEffectClass가 없는 어빌리티는 무효 태그를 반환해 UI에서 자동으로 스킵된다.
	// 기본공격처럼 "진짜 쿨다운은 없지만 UI에는 타이머를 보여주고 싶은" 경우 오버라이드해서 별도 태그를
	// 반환하면 된다 — CooldownGameplayEffectClass를 건드리지 않으므로 발동 차단에는 영향이 없다.
	virtual FGameplayTag GetUICooldownTag() const;

	// 투자 가능한(MaxAbilityLevel>1) 어빌리티는 스펙 Level이 0(=아직 포인트 미투자)이면 발동 자체를
	// 막는다 — 1레벨에 좌클릭/패시브만 쓸 수 있고 나머지는 첫 포인트를 투자해야 열리는 요구사항.
	// 투자 불가 어빌리티(MaxAbilityLevel<=1, 좌클릭/패시브 등)는 이 체크를 완전히 건너뛴다.
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// 베이스 UGameplayAbility::ActivationOwnedTags는 protected라 외부(캐릭터 클래스 등)에서 "이 어빌리티가
	// 활성 상태일 때 특정 태그를 소유하는지"를 확인할 방법이 없다 — 스턴 등으로 진행 중인 어빌리티를 선별
	// 취소해야 하는 곳(AP1HeroCharacter::CancelActiveAbilitiesOnStun)에 필요해 얇은 접근자로 노출한다.
	bool OwnsActivationTag(const FGameplayTag& Tag) const { return ActivationOwnedTags.HasTag(Tag); }

protected:
	AP1CharacterBase* GetP1CharacterFromActorInfo() const;
	AP1PlayerController* GetP1PlayerControllerFromActorInfo() const;

	// 캐릭터(영웅) 레벨 — GetAbilityLevel()(스킬 랭크, FGameplayAbilitySpec::Level)과는 다른 축.
	// "스킬 랭크가 아니라 캐릭터 레벨에 따라 값이 달라져야 하는" 수치(Stoicism 쿨다운 등)를 GE 자체의
	// Scalable Float 커브(그건 GetAbilityLevel()로 평가됨) 대신 SetByCaller로 직접 주입할 때 이 값을 쓴다.
	// PlayerState를 못 찾으면(예: ASC 소유자가 PlayerState가 아닌 경우) 1을 반환.
	int32 GetP1CharacterLevel() const;

	// 자신(어빌리티 소유자)에게 GE를 적용하는 공통 헬퍼 — 버프/디버프/쿨다운 등
	// "어빌리티가 자기 자신에게 이펙트를 건다" 패턴 전반에서 재사용한다.
	// SetByCallerTag가 유효하면 SetByCallerMagnitude를 함께 실어 보낸다 (지속시간, 세기 등).
	// MakeOutgoingGameplayEffectSpec/ApplyGameplayEffectSpecToOwner를 사용해 어빌리티의
	// 예측 키(prediction key) 컨텍스트를 올바르게 태운다 (raw ASC->ApplyGameplayEffectSpecToSelf보다 정석적).
	FActiveGameplayEffectHandle ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass,
		FGameplayTag SetByCallerTag = FGameplayTag(), float SetByCallerMagnitude = 0.0f) const;
};
