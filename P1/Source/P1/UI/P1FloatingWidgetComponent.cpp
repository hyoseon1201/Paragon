// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/P1FloatingWidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "P1.h"

UP1FloatingWidgetComponent::UP1FloatingWidgetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UP1FloatingWidgetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bBaseDrawSizeCaptured)
	{
		// 생성자/BP 디폴트에서 이미 설정된 디자인 원본 크기를 기준값으로 캡처 — 이후 매 틱 이 값에
		// 배율만 곱해서 SetDrawSize한다(누적 곱 방지, 매번 원본 기준으로 다시 계산).
		BaseDrawSize = GetDrawSize();
		bBaseDrawSizeCaptured = true;
	}

	const APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!LocalPC || ReferenceDistance <= 0.f || BaseDrawSize.IsZero())
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	LocalPC->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const float Distance = FMath::Max(FVector::Dist(ViewLocation, GetComponentLocation()), 1.f);
	const float Scale = FMath::Clamp(ReferenceDistance / Distance, MinScale, MaxScale);
	const FVector2D NewDrawSize = BaseDrawSize * Scale;

	SetDrawSize(NewDrawSize);
}
