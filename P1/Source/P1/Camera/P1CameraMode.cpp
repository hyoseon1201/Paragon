// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/P1CameraMode.h"
#include "Camera/P1CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

// ─── FP1CameraModeView ────────────────────────────────────────────────────────

FP1CameraModeView::FP1CameraModeView()
	: Location(ForceInit)
	, Rotation(ForceInit)
	, ControlRotation(ForceInit)
	, FieldOfView(80.f)
{
}

void FP1CameraModeView::Blend(const FP1CameraModeView& Other, float OtherWeight)
{
	if (OtherWeight <= 0.f) return;
	if (OtherWeight >= 1.f) { *this = Other; return; }

	Location = FMath::Lerp(Location, Other.Location, OtherWeight);
	Rotation = FQuat::Slerp(FQuat(Rotation), FQuat(Other.Rotation), OtherWeight).Rotator();
	ControlRotation = FQuat::Slerp(FQuat(ControlRotation), FQuat(Other.ControlRotation), OtherWeight).Rotator();
	FieldOfView = FMath::Lerp(FieldOfView, Other.FieldOfView, OtherWeight);
}

// ─── UP1CameraMode ────────────────────────────────────────────────────────────

UP1CameraMode::UP1CameraMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UP1CameraComponent* UP1CameraMode::GetP1CameraComponent() const
{
	return CastChecked<UP1CameraComponent>(GetOuter());
}

AActor* UP1CameraMode::GetTargetActor() const
{
	const UP1CameraComponent* CameraComponent = GetP1CameraComponent();
	return CameraComponent ? CameraComponent->GetOwner() : nullptr;
}

FVector UP1CameraMode::GetPivotLocation() const
{
	const AActor* TargetActor = GetTargetActor();
	if (!TargetActor) return FVector::ZeroVector;

	if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		return TargetCharacter->GetActorLocation() + FVector(0.f, 0.f, TargetCharacter->BaseEyeHeight);
	}
	return TargetActor->GetActorLocation();
}

FRotator UP1CameraMode::GetPivotRotation() const
{
	const AActor* TargetActor = GetTargetActor();
	if (!TargetActor) return FRotator::ZeroRotator;

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		if (const AController* Controller = TargetPawn->GetController())
		{
			return Controller->GetControlRotation();
		}
	}
	return TargetActor->GetActorRotation();
}

void UP1CameraMode::UpdateView(float DeltaTime)
{
	const FVector  PivotLocation = GetPivotLocation();
	const FRotator PivotRotation = GetPivotRotation();

	View.Location       = PivotLocation + PivotRotation.RotateVector(ViewOffset);
	View.Rotation       = PivotRotation;
	View.ControlRotation = PivotRotation;
	View.FieldOfView    = FieldOfView;
}

void UP1CameraMode::UpdateBlending(float DeltaTime)
{
	BlendAlpha = (BlendTime > 0.f)
		? FMath::Min(BlendAlpha + DeltaTime / BlendTime, 1.f)
		: 1.f;

	const float Exp = FMath::Max(BlendExponent, 1.f);
	switch (BlendFunction)
	{
	case EP1CameraModeBlendFunction::Linear:
		BlendWeight = BlendAlpha; break;
	case EP1CameraModeBlendFunction::EaseIn:
		BlendWeight = FMath::InterpEaseIn(0.f, 1.f, BlendAlpha, Exp); break;
	case EP1CameraModeBlendFunction::EaseOut:
		BlendWeight = FMath::InterpEaseOut(0.f, 1.f, BlendAlpha, Exp); break;
	case EP1CameraModeBlendFunction::EaseInOut:
		BlendWeight = FMath::InterpEaseInOut(0.f, 1.f, BlendAlpha, Exp); break;
	default:
		BlendWeight = BlendAlpha; break;
	}
}

// ─── UP1CameraModeStack ───────────────────────────────────────────────────────

UP1CameraModeStack::UP1CameraModeStack(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UP1CameraModeStack::PushCameraMode(TSubclassOf<UP1CameraMode> CameraModeClass)
{
	if (!CameraModeClass) return;

	// Already the top active mode — nothing to do
	if (ActiveModes.Num() > 0 && ActiveModes.Last()->GetClass() == CameraModeClass) return;

	UP1CameraMode* NewMode = GetCameraModeInstance(CameraModeClass);
	if (!NewMode) return;

	// Move to top of stack and reset blend state
	ActiveModes.Remove(NewMode);
	NewMode->BlendAlpha  = (NewMode->BlendTime <= 0.f) ? 1.f : 0.f;
	NewMode->BlendWeight = NewMode->BlendAlpha;
	ActiveModes.Add(NewMode);
}

bool UP1CameraModeStack::EvaluateStack(float DeltaTime, FP1CameraModeView& OutView)
{
	if (ActiveModes.Num() == 0) return false;

	for (UP1CameraMode* Mode : ActiveModes)
	{
		Mode->UpdateView(DeltaTime);
		Mode->UpdateBlending(DeltaTime);
	}

	// Prune: if top mode is fully blended in, discard everything below it
	while (ActiveModes.Num() > 1 && ActiveModes.Last()->BlendWeight >= 1.f)
	{
		ActiveModes.RemoveAt(0);
	}

	// Accumulate from bottom (oldest/base) upward
	OutView = ActiveModes[0]->View;
	for (int32 i = 1; i < ActiveModes.Num(); ++i)
	{
		OutView.Blend(ActiveModes[i]->View, ActiveModes[i]->BlendWeight);
	}

	return true;
}

UP1CameraMode* UP1CameraModeStack::GetCameraModeInstance(TSubclassOf<UP1CameraMode> CameraModeClass)
{
	for (TObjectPtr<UP1CameraMode>& Existing : CameraModeInstances)
	{
		if (Existing && Existing->GetClass() == CameraModeClass)
		{
			return Existing.Get();
		}
	}

	// Outer is CameraComponent (the stack's outer), so mode can call GetP1CameraComponent()
	UP1CameraMode* NewMode = NewObject<UP1CameraMode>(GetOuter(), CameraModeClass);
	if (NewMode)
	{
		CameraModeInstances.Add(NewMode);
	}
	return NewMode;
}
