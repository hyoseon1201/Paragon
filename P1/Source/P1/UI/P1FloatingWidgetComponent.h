// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "P1FloatingWidgetComponent.generated.h"

// Space=Screen 월드앵커 위젯은 항상 고정 픽셀 크기를 유지한다(회전/오클루전 문제는 없지만) — 그래서
// 캐릭터가 카메라에 가까워지면 캐릭터 자체(3D 메시)는 원근 때문에 화면에서 커지는데 UI는 그대로라
// 상대적으로 작아 보이는 문제가 있다. 이 컴포넌트는 매 틱 로컬 카메라와의 거리를 계산해 DrawSize를
// 함께 스케일링해서 "카메라에 가까울수록 UI도 같이 커지는" World space 원근 느낌을 흉내낸다 — 회전/
// 오클루전은 여전히 Screen space 기본 동작(항상 정면, 벽에 안 가려짐)을 그대로 따른다.
UCLASS(ClassGroup = (UserInterface), meta = (BlueprintSpawnableComponent))
class P1_API UP1FloatingWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UP1FloatingWidgetComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 이 거리일 때 디자인 원본 크기(DrawSize)를 그대로 사용한다 — 기준 거리.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
	float ReferenceDistance = 800.f;

	// 최종 배율 = Clamp(ReferenceDistance / 카메라거리, MinScale, MaxScale).
	// 카메라가 ReferenceDistance보다 가까우면 1보다 커지고(최대 MaxScale), 멀면 1보다 작아진다(최소 MinScale).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
	float MinScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
	float MaxScale = 1.5f;

private:
	// 디자인 시점(BP/생성자에서 설정한) 원본 DrawSize — 첫 틱에 지연 캡처해 스케일 계산의 기준으로 쓴다.
	FVector2D BaseDrawSize = FVector2D::ZeroVector;
	bool bBaseDrawSizeCaptured = false;

	float DebugLogAccum = 0.f;
};
