// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/HUD/P1HUD.h"
#include "UI/Widget/HUD/P1HUDWidget.h"
#include "UI/Widget/Shop/P1ShopWidget.h"
#include "UI/WidgetController/P1OverlayWidgetController.h"
#include "UI/WidgetController/P1ShopWidgetController.h"
#include "Components/Widget.h"
#include "P1.h"

UP1OverlayWidgetController* AP1HUD::GetOverlayWidgetController(const FWidgetControllerParams& WCParams)
{
	if (!OverlayWidgetController)
	{
		checkf(OverlayWidgetControllerClass,
			TEXT("AP1HUD: OverlayWidgetControllerClass가 설정되지 않았습니다 — BP_P1HUD의 Details를 확인하세요."));

		OverlayWidgetController = NewObject<UP1OverlayWidgetController>(this, OverlayWidgetControllerClass);
		OverlayWidgetController->SetWidgetControllerParams(WCParams);
		OverlayWidgetController->BindCallbacksToDependencies();
	}
	return OverlayWidgetController;
}

void AP1HUD::InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	// OnRep_PlayerState가 폰 생애주기 중 두 번 이상 불릴 가능성에 대비한 안전장치 — 이미 만들어진
	// 뒤 재호출되면 조용히 스킵한다(안 그러면 오버레이가 중복 생성된다).
	if (OverlayWidget)
	{
		return;
	}

	checkf(OverlayWidgetClass,
		TEXT("AP1HUD: OverlayWidgetClass가 설정되지 않았습니다 — BP_P1HUD의 Details를 확인하세요."));

	const FWidgetControllerParams WCParams(PC, PS, ASC, AS);
	UP1OverlayWidgetController* Controller = GetOverlayWidgetController(WCParams);

	OverlayWidget = CreateWidget<UP1HUDWidget>(PC, OverlayWidgetClass);
	OverlayWidget->SetWidgetController(Controller);
	OverlayWidget->AddToViewport();

	Controller->BroadcastInitialValues();

	UE_LOG(LogP1, Log, TEXT("[P1HUD] Overlay 초기화 완료 — PC=%s"), *PC->GetName());
}

UP1ShopWidgetController* AP1HUD::GetShopWidgetController(const FWidgetControllerParams& WCParams)
{
	if (!ShopWidgetController)
	{
		checkf(ShopWidgetControllerClass,
			TEXT("AP1HUD: ShopWidgetControllerClass가 설정되지 않았습니다 — BP_P1HUD의 Details를 확인하세요."));

		ShopWidgetController = NewObject<UP1ShopWidgetController>(this, ShopWidgetControllerClass);
		ShopWidgetController->SetWidgetControllerParams(WCParams);
		ShopWidgetController->BindCallbacksToDependencies();
	}
	return ShopWidgetController;
}

void AP1HUD::InitShop(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	if (ShopWidget)
	{
		return;
	}

	checkf(ShopWidgetClass,
		TEXT("AP1HUD: ShopWidgetClass가 설정되지 않았습니다 — BP_P1HUD의 Details를 확인하세요."));

	const FWidgetControllerParams WCParams(PC, PS, ASC, AS);
	UP1ShopWidgetController* Controller = GetShopWidgetController(WCParams);

	ShopWidget = CreateWidget<UP1ShopWidget>(PC, ShopWidgetClass);
	ShopWidget->SetWidgetController(Controller);
	ShopWidget->AddToViewport();
	// 토글 키를 누르기 전까지는 숨김 — Overlay와 달리 상점은 상시 노출 화면이 아니다.
	ShopWidget->SetVisibility(ESlateVisibility::Collapsed);

	Controller->BroadcastInitialValues();

	UE_LOG(LogP1, Log, TEXT("[P1HUD] Shop 초기화 완료 — PC=%s"), *PC->GetName());
}
