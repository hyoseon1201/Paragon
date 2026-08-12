// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1MinimapIconWidget.generated.h"

class UImage;

// 미니맵 위의 점 하나 — 플레이어 아이콘과 정글 캠프 아이콘 둘 다 이 클래스를 재사용한다(색만 다름).
// 컨트롤러/PlayerState가 필요 없는 순수 표시 위젯이라 UP1UserWidget이 아니라 UUserWidget을 직접
// 상속(UP1ShopItemSlotWidget과 같은 이유) — 위치/색상은 전부 UP1MinimapWidget이 직접 호출해서 채운다.
UCLASS()
class P1_API UP1MinimapIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetIconColor(FLinearColor Color);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;
};
