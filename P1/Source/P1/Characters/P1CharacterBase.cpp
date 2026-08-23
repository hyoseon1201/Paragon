// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/P1CharacterBase.h"
#include "UI/P1FloatingWidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "VFX/P1MovingParticleEffectActor.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Misc/App.h"
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

bool AP1CharacterBase::IsHero() const
{
	return CharacterType == TAG_Character_Type_Hero;
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

void AP1CharacterBase::MulticastPlayNiagaraEffect_Implementation(UNiagaraSystem* NiagaraTemplate, FName SocketName)
{
	if (!NiagaraTemplate)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (!SocketName.IsNone() && MeshComp->DoesSocketExist(SocketName))
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraTemplate, MeshComp, SocketName,
				FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
			return;
		}
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NiagaraTemplate, GetActorLocation());
}

void AP1CharacterBase::MulticastSetAttachedNiagaraEffect_Implementation(UNiagaraSystem* NiagaraTemplate, FName SocketName)
{
	// 같은 소켓에 이미 재생 중인 지속 이펙트가 있다면 먼저 정리 — 중첩 재생 방지.
	MulticastStopAttachedNiagaraEffect_Implementation(SocketName);

	if (!NiagaraTemplate)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// bAutoDestroy=false — MulticastPlayNiagaraEffect(1회성)와 달리 명시적으로 멈출 때까지 유지.
	// 이펙트 에셋이 Looping으로 만들어져 있어도(지속 버프용 연출은 보통 루프) 자동으로 "완료" 상태에
	// 도달하지 않으므로 반드시 여기서 명시적으로 파괴해야 사라진다.
	if (!SocketName.IsNone() && MeshComp->DoesSocketExist(SocketName))
	{
		UNiagaraComponent* NewComp = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraTemplate, MeshComp, SocketName,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
		AttachedNiagaraEffectComponents.Add(SocketName, NewComp);
	}
}

void AP1CharacterBase::MulticastStopAttachedNiagaraEffect_Implementation(FName SocketName)
{
	if (TObjectPtr<UNiagaraComponent>* ExistingComp = AttachedNiagaraEffectComponents.Find(SocketName))
	{
		if (IsValid(*ExistingComp))
		{
			(*ExistingComp)->DeactivateImmediate();
			(*ExistingComp)->DestroyComponent();
		}
		AttachedNiagaraEffectComponents.Remove(SocketName);
	}
}

void AP1CharacterBase::MulticastPlayParticleEffectAtLocation_Implementation(UParticleSystem* ParticleTemplate, FVector Location, FRotator Rotation, FVector Scale)
{
	if (!ParticleTemplate)
	{
		return;
	}

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleTemplate, Location, Rotation, Scale);
}

void AP1CharacterBase::MulticastPlayMovingParticleEffect_Implementation(UParticleSystem* ParticleTemplate, FVector StartLocation, FVector EndLocation, float Duration)
{
	if (!ParticleTemplate || Duration <= 0.0f)
	{
		return;
	}

	// 이 함수는 NetMulticast라 서버 자신에게도 로컬로 실행된다 — 헤드리스 데디케이티드 서버와
	// -nullrhi 헤드리스 봇 클라이언트에서는 렌더링할 화면 자체가 없으므로 애초에 스폰하지 않는다.
	if (!FApp::CanEverRender() || (GetWorld() && GetWorld()->IsNetMode(NM_DedicatedServer)))
	{
		return;
	}

	// TWeakObjectPtr+FTimerHandle+수동 Destroy() 조합으로 직접 구현했던 이전 버전은 세 차례
	// 시도에도 정확한 원인 불명의 EXCEPTION_ACCESS_VIOLATION이 반복 재현됐다(오너 액터가
	// IsValid() 체크는 통과하는데 몇 줄 뒤 Destroy() 호출 시점엔 죽어있는 패턴). 대신 이미
	// 검증된 패턴(AP1DamageNumberActor와 동일한 Tick()+SetLifeSpan() — 커스텀 위크포인터/
	// 타이머 관리가 전혀 없이 엔진이 전적으로 액터 수명을 관리)으로 갈아엎어 문제 자체를
	// 없앴다 — 자세한 내용은 AP1MovingParticleEffectActor 헤더 주석 참고.
	if (AP1MovingParticleEffectActor* Effect = GetWorld()->SpawnActor<AP1MovingParticleEffectActor>(StartLocation, (EndLocation - StartLocation).Rotation()))
	{
		Effect->InitializeMovingEffect(ParticleTemplate, StartLocation, EndLocation, Duration);
	}
}

void AP1CharacterBase::MulticastDrawDebugSphere_Implementation(FVector Location, float Radius, FColor Color, float Duration)
{
#if ENABLE_DRAW_DEBUG
	DrawDebugSphere(GetWorld(), Location, Radius, 24, Color, false, Duration, 0, 2.0f);
#endif
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

void AP1CharacterBase::MulticastSetPersistentParticleEffectAtLocation_Implementation(UParticleSystem* ParticleTemplate, FVector Location, FRotator Rotation, FVector Scale)
{
	// 이미 재생 중인 지속 이펙트가 있다면 먼저 정리 — 중첩 재생 방지.
	MulticastStopPersistentParticleEffectAtLocation_Implementation();

	if (!ParticleTemplate)
	{
		return;
	}

	// bAutoDestroy=false — 이펙트 에셋 자체가 무한 루프여도(Duration=0/Looping) 명시적으로 멈출 때까지 유지.
	PersistentLocationParticleEffectComponent = UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(), ParticleTemplate, Location, Rotation, Scale, false);
}

void AP1CharacterBase::MulticastStopPersistentParticleEffectAtLocation_Implementation()
{
	if (IsValid(PersistentLocationParticleEffectComponent))
	{
		PersistentLocationParticleEffectComponent->DeactivateSystem();
		PersistentLocationParticleEffectComponent->DestroyComponent();
	}
	PersistentLocationParticleEffectComponent = nullptr;
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
