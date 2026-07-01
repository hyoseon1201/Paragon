// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1UserWidget.generated.h"

class UP1WidgetController;

// 모든 P1 위젯의 공통 베이스.
// SetWidgetController → OnWidgetControllerSet() 순서로 초기화된다.
UCLASS(Abstract)
class P1_API UP1UserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Widget")
	void SetWidgetController(UP1WidgetController* InWidgetController);

protected:
	// 서브클래스에서 override — 여기서 컨트롤러 델리게이트를 바인딩한다.
	virtual void OnWidgetControllerSet() {}

	UPROPERTY(BlueprintReadOnly, Category = "Widget")
	TObjectPtr<UP1WidgetController> WidgetController;
};
