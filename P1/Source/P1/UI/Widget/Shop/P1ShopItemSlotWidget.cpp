// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Shop/P1ShopItemSlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Player/P1PlayerState.h"
#include "Player/P1ShopTypes.h"
#include "Engine/Texture2D.h"

void UP1ShopItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddDynamic(this, &UP1ShopItemSlotWidget::HandleSelectClicked);
	}
}

void UP1ShopItemSlotWidget::SetSlotData(AP1PlayerState* InPlayerState, FName InItemRowName, const FP1ShopItemData& ItemData, bool bInOwned)
{
	OwningPlayerState = InPlayerState;
	ItemRowName = InItemRowName;

	if (NameText)
	{
		NameText->SetText(ItemData.DisplayName);
	}

	if (IconImage && ItemData.Icon)
	{
		IconImage->SetBrushFromTexture(ItemData.Icon);
	}

	if (OwnedBadge)
	{
		OwnedBadge->SetVisibility(bInOwned ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UP1ShopItemSlotWidget::HandleSelectClicked()
{
	if (ItemRowName.IsNone())
	{
		return;
	}

	OnSelected.Broadcast(ItemRowName);

	// 더블클릭 판정 — SelectButton이 칸 전체를 덮어 마우스 입력을 먼저 가로채므로
	// NativeOnMouseButtonDoubleClick은 쓸 수 없다(위 헤더 주석 참고). 대신 OnClicked가
	// DoubleClickThresholdSeconds 안에 두 번 들어오면 구매로 취급한다.
	constexpr double DoubleClickThresholdSeconds = 0.35;
	const double CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const bool bIsDoubleClick = LastClickTimeSeconds >= 0.0 && (CurrentTimeSeconds - LastClickTimeSeconds) <= DoubleClickThresholdSeconds;
	LastClickTimeSeconds = bIsDoubleClick ? -1.0 : CurrentTimeSeconds;

	if (!bIsDoubleClick)
	{
		return;
	}

	if (AP1PlayerState* PS = OwningPlayerState.Get())
	{
		// 서버도 어차피 중복 보유를 거부하지만(ServerBuyItem), 이미 보유 중인 아이템은 애초에 RPC를
		// 보내지 않고 클라이언트에서 조용히 막는다.
		if (!PS->GetInventory().Contains(ItemRowName))
		{
			PS->ServerBuyItem(ItemRowName);
		}
	}
}
