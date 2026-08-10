// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/P1DamageNumberActor.h"
#include "UI/P1FloatingWidgetComponent.h"
#include "UI/Widget/HUD/P1DamageNumberWidget.h"
#include "Blueprint/UserWidget.h"
#include "P1.h"

AP1DamageNumberActor::AP1DamageNumberActor()
{
	PrimaryActorTick.bCanEverTick = true;
	// 로컬 전용 코스메틱 액터 — 서버가 존재조차 모르므로 리플리케이트할 이유가 없다.
	bReplicates = false;

	WidgetComponent = CreateDefaultSubobject<UP1FloatingWidgetComponent>(TEXT("WidgetComponent"));
	RootComponent = WidgetComponent;
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComponent->SetDrawSize(FVector2D(150.0f, 60.0f));
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AP1DamageNumberActor::InitializeDamageNumber(float DamageAmount, bool bIsMagicalDamage)
{
	if (UP1DamageNumberWidget* Widget = GetDamageNumberWidget())
	{
		Widget->SetDamageAmount(DamageAmount, bIsMagicalDamage);
	}
	else
	{
		// 여기 찍히면 화면에 아무것도 안 뜨는 게 당연함 — 원인은 둘 중 하나:
		// (1) BP_P1DamageNumberActor의 WidgetComponent->Widget Class가 비어있음
		// (2) 그 Widget Class가 WBP_DamageNumber(부모=UP1DamageNumberWidget)가 아닌 다른 걸 가리킴(Cast 실패)
		const UUserWidget* RawWidget = WidgetComponent ? WidgetComponent->GetUserWidgetObject() : nullptr;
		UE_LOG(LogP1, Warning, TEXT("[DamageNumber] Widget 캐스트 실패 — WidgetComponent=%s RawWidget=%s(%s)"),
			WidgetComponent ? TEXT("O") : TEXT("X(널)"),
			RawWidget ? TEXT("O") : TEXT("X(널 — WidgetClass 미설정 가능성 높음)"),
			RawWidget ? *RawWidget->GetClass()->GetName() : TEXT("N/A"));
	}

	SetLifeSpan(LifeSpanSeconds);
}

void AP1DamageNumberActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AddActorWorldOffset(FVector(0.0f, 0.0f, RiseSpeed * DeltaSeconds));
}

UP1DamageNumberWidget* AP1DamageNumberActor::GetDamageNumberWidget() const
{
	return WidgetComponent ? Cast<UP1DamageNumberWidget>(WidgetComponent->GetUserWidgetObject()) : nullptr;
}
