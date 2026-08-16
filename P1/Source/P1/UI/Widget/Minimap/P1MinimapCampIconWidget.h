// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1MinimapCampIconWidget.generated.h"

class UImage;
class UTextBlock;

// 미니맵 위의 정글 캠프 아이콘 — 플레이어 아이콘(UP1MinimapIconWidget)과 별개 클래스로 분리했다.
// 초상화가 필요 없고(캠프는 영웅이 아님) 대신 리스폰 카운트다운 텍스트가 필요해서, 둘을 겸용하는
// 대신 각자 필요한 것만 갖는 얇은 클래스로 나눴다. 컨트롤러/PlayerState가 필요 없는 순수 표시
// 위젯이라 UUserWidget을 직접 상속 — 색/카운트다운은 전부 UP1MinimapWidget이 직접 호출해서 채운다.
//
// 구조: IconImage(단색으로 틴트되는 다이아몬드) — 다이아몬드 모양은 C++이 아니라 WBP 디자이너에서
// UV 기반 마름모 알파 마스크 머티리얼을 Brush로 설정해서 낸다(UP1SkillIconWidget의 마름모 아이콘과
// 동일한 트릭). CountdownText는 팀이 캠프의 죽음을 확인했을 때만 남은 리스폰 시간을 보여준다.
UCLASS()
class P1_API UP1MinimapCampIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 캠프 아이콘 색(기본 상태) 또는 확인된 사망 상태의 어두운 색을 칠한다.
	void SetIconColor(FLinearColor Color);

	// 팀이 죽음을 확인했을 때 남은 리스폰 시간(초)을 표시.
	void SetCountdownSeconds(float RemainingSeconds);

	// 카운트다운 텍스트를 비운다 — 캠프가 살아있거나 아직 팀이 죽음을 확인 못 한 상태(스테일 표시).
	void ClearCountdown();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountdownText;
};
