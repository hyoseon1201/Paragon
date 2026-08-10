// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/WidgetController/P1WidgetController.h"
#include "P1HUD.generated.h"

class UP1HUDWidget;
class UP1OverlayWidgetController;
class UP1ShopWidget;
class UP1ShopWidgetController;

// 화면스페이스 UI 컨트롤러의 팩토리·레지스트리.
// BP_P1HUD에서 OverlayWidgetClass·OverlayWidgetControllerClass를 설정한다.
UCLASS()
class P1_API AP1HUD : public AHUD
{
	GENERATED_BODY()

public:
	UP1OverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams);
	UP1ShopWidgetController* GetShopWidgetController(const FWidgetControllerParams& WCParams);

	// AP1HeroCharacter::HandleAbilitySystemReady 에서 호출.
	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);

	// Overlay와 동일 시점에 호출 — 상점 위젯을 한 번만 만들어 Collapsed 상태로 뷰포트에 올려둔다.
	// AP1PlayerController가 토글 키를 누를 때마다 이 위젯을 새로 만들지 않고 Visibility만 바꾼다.
	void InitShop(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);

	UP1ShopWidget* GetShopWidget() const { return ShopWidget; }

private:
	UPROPERTY()
	TObjectPtr<UP1HUDWidget> OverlayWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Overlay")
	TSubclassOf<UP1HUDWidget> OverlayWidgetClass;

	UPROPERTY()
	TObjectPtr<UP1OverlayWidgetController> OverlayWidgetController;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Overlay")
	TSubclassOf<UP1OverlayWidgetController> OverlayWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UP1ShopWidget> ShopWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Shop")
	TSubclassOf<UP1ShopWidget> ShopWidgetClass;

	UPROPERTY()
	TObjectPtr<UP1ShopWidgetController> ShopWidgetController;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Shop")
	TSubclassOf<UP1ShopWidgetController> ShopWidgetControllerClass;
};
