// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1MinimapIconWidget.generated.h"

class UImage;
class UTexture2D;

// 미니맵 위의 플레이어 아이콘 하나 — 정글 캠프 아이콘은 별도 클래스(UP1MinimapCampIconWidget)로
// 분리되어 있다(초상화/카운트다운 등 서로 필요한 게 달라서 하나로 겸용하지 않음). 컨트롤러/PlayerState가
// 필요 없는 순수 표시 위젯이라 UP1UserWidget이 아니라 UUserWidget을 직접 상속(UP1ShopItemSlotWidget과
// 같은 이유) — 위치/색상/텍스처는 전부 UP1MinimapWidget이 직접 호출해서 채운다.
//
// 구조: BorderImage(뒤, 팀 색으로 틴트된 원형 링) + PortraitImage(앞, 영웅 초상화가 그 위에 원형으로
// 얹힘 — PortraitImage가 BorderImage보다 살짝 작아야 테두리가 링 형태로 보인다). 원형으로 잘리는 건
// C++ 코드가 아니라 WBP 디자이너에서 각 Image의 Brush > Draw As = Rounded Box, Corner Radii를
// 이미지 절반 크기로 설정해서 처리한다(UE5 Slate 내장 기능이라 별도 마스크 머티리얼 불필요).
UCLASS()
class P1_API UP1MinimapIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 팀 색으로 테두리 링을 칠한다.
	void SetIconColor(FLinearColor Color);

	// 영웅 초상화 설정.
	void SetPortraitTexture(UTexture2D* Texture);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BorderImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PortraitImage;
};
