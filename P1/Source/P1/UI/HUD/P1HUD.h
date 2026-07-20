// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/WidgetController/P1WidgetController.h"
#include "P1HUD.generated.h"

class UP1HUDWidget;
class UP1OverlayWidgetController;

// 화면스페이스 UI 컨트롤러의 팩토리·레지스트리.
// BP_P1HUD에서 OverlayWidgetClass·OverlayWidgetControllerClass를 설정한다.
UCLASS()
class P1_API AP1HUD : public AHUD
{
	GENERATED_BODY()

public:
	UP1OverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams);

	// AP1HeroCharacter::HandleAbilitySystemReady 에서 호출.
	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);

private:
	UPROPERTY()
	TObjectPtr<UP1HUDWidget> OverlayWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Overlay")
	TSubclassOf<UP1HUDWidget> OverlayWidgetClass;

	UPROPERTY()
	TObjectPtr<UP1OverlayWidgetController> OverlayWidgetController;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Overlay")
	TSubclassOf<UP1OverlayWidgetController> OverlayWidgetControllerClass;
};
