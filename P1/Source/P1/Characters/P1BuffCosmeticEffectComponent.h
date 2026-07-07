// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "P1BuffCosmeticEffectComponent.generated.h"

class UAbilitySystemComponent;
class UMaterialInterface;
class UParticleSystem;
class AP1CharacterBase;

// 하나의 "버프 태그 → 코스메틱 이펙트" 매핑. 태그가 부여되면 켜지고, 태그가 완전히 사라지면(카운트 0)
// 자동으로 원복된다 — 머티리얼/파티클 둘 다 선택 사항이며, 필드를 비워두면(SlotName/Template 없음)
// 그 항목은 아예 관여하지 않는다.
USTRUCT(BlueprintType)
struct FP1BuffCosmeticEffectEntry
{
	GENERATED_BODY()

	// 이 태그가 ASC에 부여되면 아래 이펙트가 켜지고, 카운트가 0으로 돌아가면 자동으로 꺼진다.
	UPROPERTY(EditDefaultsOnly, Category = "BuffCosmeticEffect")
	FGameplayTag BuffTag;

	// 머티리얼 오버라이드 (SlotName이 비어있으면 이 항목은 머티리얼을 다루지 않는다).
	UPROPERTY(EditDefaultsOnly, Category = "BuffCosmeticEffect|Material")
	FName MaterialSlotName;

	UPROPERTY(EditDefaultsOnly, Category = "BuffCosmeticEffect|Material")
	TObjectPtr<UMaterialInterface> OverrideMaterial;

	// 소켓에 계속 붙어있는 지속 파티클 (Template이 없으면 이 항목은 파티클을 다루지 않는다).
	// 주의: 캐릭터당 지속 파티클 슬롯은 하나(AP1CharacterBase::AttachedParticleEffectComponent)만
	// 추적되므로, 여러 항목이 동시에 파티클을 켜는 상황은 지금은 지원하지 않는다(현재는 항목 1개뿐이라 충분).
	UPROPERTY(EditDefaultsOnly, Category = "BuffCosmeticEffect|Particle")
	TObjectPtr<UParticleSystem> AttachedParticleTemplate;

	UPROPERTY(EditDefaultsOnly, Category = "BuffCosmeticEffect|Particle")
	FName ParticleSocketName;
};

// "버프 태그가 활성인 동안 코스메틱 이펙트를 유지하고, 사라지면 자동으로 원복"하는 범용 컴포넌트.
// 특정 스킬(Sacred Oath 등)을 전혀 몰라도 되도록 태그/머티리얼/파티클을 전부 데이터(Effects 배열)로
// 받는다 — 이 시너지가 필요한 영웅의 BP에만 이 컴포넌트를 추가하면 되고, 없는 영웅은 아무 영향도
// 받지 않는다(AP1HeroCharacter 같은 공용 베이스에 특정 스킬 이름을 하드코딩하지 않기 위함).
//
// ASC는 PlayerState에 있어서 PossessedBy/OnRep_PlayerState 시점에야 준비되므로, 소유 캐릭터가
// ASC 초기화를 마친 시점에 BindToAbilitySystemComponent()를 명시적으로 호출해줘야 한다.
UCLASS(Blueprintable, ClassGroup = (Custom), Meta = (BlueprintSpawnableComponent))
class P1_API UP1BuffCosmeticEffectComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	// 소유 캐릭터의 ASC가 준비된 시점(InitAbilityActorInfo 등)에 호출 — Effects의 각 태그를 구독한다.
	// 재호출 시(리스폰 등) 기존 구독을 정리하고 다시 등록한다.
	void BindToAbilitySystemComponent(UAbilitySystemComponent* ASC);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "BuffCosmeticEffect")
	TArray<FP1BuffCosmeticEffectEntry> Effects;

private:
	void UnbindFromAbilitySystemComponent();
	void OnBuffTagChanged(const FGameplayTag Tag, int32 NewCount, FP1BuffCosmeticEffectEntry Entry);

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	TArray<TPair<FGameplayTag, FDelegateHandle>> TagDelegateHandles;
};
