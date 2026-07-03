// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/P1CharacterBase.h"
#include "Components/WidgetComponent.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "P1.h"

AP1CharacterBase::AP1CharacterBase()
{
	FloatingStatusComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("FloatingStatusComponent"));
	FloatingStatusComponent->SetupAttachment(GetRootComponent());
	FloatingStatusComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	FloatingStatusComponent->SetWidgetSpace(EWidgetSpace::World);
	FloatingStatusComponent->SetDrawSize(FVector2D(180.f, 50.f));
	FloatingStatusComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 기본 유형은 영웅. 미니언/보스 서브클래스 BP에서 CharacterType을 재설정한다.
	CharacterType = TAG_Character_Type_Hero;
}

bool AP1CharacterBase::IsHeroOrBoss() const
{
	return CharacterType == TAG_Character_Type_Hero || CharacterType == TAG_Character_Type_Boss;
}

bool AP1CharacterBase::IsSameTeam(const AActor* A, const AActor* B)
{
	const IGenericTeamAgentInterface* TeamA = Cast<IGenericTeamAgentInterface>(A);
	const IGenericTeamAgentInterface* TeamB = Cast<IGenericTeamAgentInterface>(B);

	if (!TeamA || !TeamB)
	{
		return false;
	}

	const uint8 IdA = TeamA->GetGenericTeamId().GetId();
	const uint8 IdB = TeamB->GetGenericTeamId().GetId();

	UE_LOG(LogP1, Log, TEXT("[IsSameTeam] %s(Team=%d) vs %s(Team=%d)"),
		A ? *A->GetName() : TEXT("null"), IdA,
		B ? *B->GetName() : TEXT("null"), IdB);

	// 255(NoTeam)은 팀 미설정 상태 — 같은 팀으로 취급하지 않아 공격 허용.
	// 프로덕션에서는 GameMode가 모든 캐릭터에 팀을 할당하므로 이 경로에 걸리지 않아야 한다.
	if (IdA == 255 || IdB == 255)
	{
		return false;
	}

	return IdA == IdB;
}
