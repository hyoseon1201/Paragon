// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/P1CharacterBase.h"
#include "UI/P1FloatingWidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "P1.h"

AP1CharacterBase::AP1CharacterBase()
{
	FloatingStatusComponent = CreateDefaultSubobject<UP1FloatingWidgetComponent>(TEXT("FloatingStatusComponent"));
	FloatingStatusComponent->SetupAttachment(GetRootComponent());
	FloatingStatusComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	FloatingStatusComponent->SetWidgetSpace(EWidgetSpace::Screen);
	FloatingStatusComponent->SetDrawSize(FVector2D(180.f, 50.f));
	FloatingStatusComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 기본 유형은 영웅. 미니언/보스 서브클래스 BP에서 CharacterType을 재설정한다.
	CharacterType = TAG_Character_Type_Hero;
}

bool AP1CharacterBase::IsHeroOrBoss() const
{
	return CharacterType == TAG_Character_Type_Hero || CharacterType == TAG_Character_Type_Boss;
}

void AP1CharacterBase::MulticastSetMaterialOverride_Implementation(FName SlotName, UMaterialInterface* OverrideMaterial)
{
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		const int32 SlotIndex = MeshComp->GetMaterialIndex(SlotName);
		if (SlotIndex != INDEX_NONE)
		{
			// OverrideMaterial이 nullptr이면 오버라이드가 해제되고 스켈레탈메시 기본 머티리얼로 되돌아간다.
			MeshComp->SetMaterial(SlotIndex, OverrideMaterial);
		}
	}
}

void AP1CharacterBase::MulticastPlayParticleEffect_Implementation(UParticleSystem* ParticleTemplate, FName SocketName)
{
	if (!ParticleTemplate)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (!SocketName.IsNone() && MeshComp->DoesSocketExist(SocketName))
		{
			UGameplayStatics::SpawnEmitterAttached(ParticleTemplate, MeshComp, SocketName,
				FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
			return;
		}
	}

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleTemplate, GetActorLocation());
}

void AP1CharacterBase::MulticastSetAttachedParticleEffect_Implementation(UParticleSystem* ParticleTemplate, FName SocketName)
{
	// 이미 재생 중인 지속 이펙트가 있다면 먼저 정리 — 중첩 재생 방지.
	MulticastStopAttachedParticleEffect_Implementation();

	if (!ParticleTemplate)
	{
		return;
	}

	// bAutoDestroy=false — MulticastPlayParticleEffect(1회성)와 달리 명시적으로 멈출 때까지 유지.
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp && !SocketName.IsNone() && MeshComp->DoesSocketExist(SocketName))
	{
		AttachedParticleEffectComponent = UGameplayStatics::SpawnEmitterAttached(ParticleTemplate, MeshComp, SocketName,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
		return;
	}

	// 소켓을 지정하지 않았거나 존재하지 않으면 루트 컴포넌트에 붙인다 — 특정 본이 아니라
	// 캐릭터 전체를 중심으로 도는 이펙트(예: Make Way 회오리)에 적합한 폴백.
	AttachedParticleEffectComponent = UGameplayStatics::SpawnEmitterAttached(ParticleTemplate, GetRootComponent(), NAME_None,
		FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
}

void AP1CharacterBase::MulticastStopAttachedParticleEffect_Implementation()
{
	if (IsValid(AttachedParticleEffectComponent))
	{
		AttachedParticleEffectComponent->DeactivateSystem();
		AttachedParticleEffectComponent->DestroyComponent();
	}
	AttachedParticleEffectComponent = nullptr;
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
