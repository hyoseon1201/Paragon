// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1MinimapWidget.generated.h"

class UImage;
class UCanvasPanel;
class UP1MinimapIconWidget;
class UP1MinimapCampIconWidget;
class AP1PlayerState;
class AP1JungleCampAnchor;

// 우측상단(등) 미니맵 — ASC/위젯 컨트롤러 없이 UP1HUDWidget::RefreshMatchTime()과 동일하게
// GetWorld()->GetGameState<AP1GameState>()를 타이머로 직접 폴링한다(계속 흐르는 스냅샷 UI라 델리게이트
// 보다 폴링이 더 단순하고 정확히 맞는 패턴).
//
// 플레이어 시야는 서버 권위 안티치트 시스템이 아니라 전부 클라이언트 로컬 계산이다 — 캐릭터 위치는
// 이미 표준 Character 무브먼트 리플리케이션으로 모든 클라이언트에 도달해 있으므로(3D 렌더링을 위해
// 애초에 숨길 수 없음), "적이 아군 시야 안에 있는지"는 미니맵 표시 여부만 클라이언트에서 걸러내는
// 순수 연출이다.
//
// 정글 캠프는 다르다 — "팀이 직접 확인해야 죽음을 안다"는 진짜 정보 은닉(안개)이 필요해서, 실제
// 생사 판정과 팀별 확인 여부를 AP1JungleCampAnchor가 서버 권위로 계산해 리플리케이트한다
// (RespawnServerTime/TeamHasObservedDeath). 이 위젯은 그 값을 그대로 읽어 표시만 한다 — 미확인
// 상태면 실제로 죽었어도 계속 "살아있음"으로 스테일 표시하고, 확인되면 카운트다운을 보여준다.
// 부활만은 확인 여부와 무관하게 항상 즉시 반영된다.
UCLASS()
class P1_API UP1MinimapWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> MinimapBackground;

	// 아이콘들을 얹을 대상 — 배경 위에 겹쳐서 배치. AddChild 후 CanvasPanelSlot::SetPosition으로
	// 위치를 잡으므로 반드시 CanvasPanel이어야 한다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> MinimapIconContainer;

	// 플레이어 아이콘 클래스 — WBP_MinimapIcon(parent=UP1MinimapIconWidget, 초상화+원형 테두리) 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	TSubclassOf<UP1MinimapIconWidget> IconWidgetClass;

	// 정글 캠프 아이콘 클래스 — 초상화/카운트다운 등 플레이어 아이콘과 필요한 게 달라 완전히 별개
	// C++ 클래스(UP1MinimapCampIconWidget)로 분리했다. WBP_MinimapCampIcon(parent=
	// UP1MinimapCampIconWidget, 다이아몬드 마스크 머티리얼 + CountdownText 배치) 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	TSubclassOf<UP1MinimapCampIconWidget> CampIconWidgetClass;

	// 아레나 월드 X/Y 범위(cm) — 미니맵 배경 이미지의 좌상단/우하단에 대응하는 월드 좌표. 에디터에서
	// 실측해서 지정해야 한다(배경 캡처 카메라가 정확히 수직/무회전이어야 이 매핑이 정확함).
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	FVector2D WorldBoundsMin = FVector2D(-5000.0, -5000.0);

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	FVector2D WorldBoundsMax = FVector2D(5000.0, 5000.0);

	// 아군 기준 이 반경(cm) 안에 있는 적만 미니맵에 표시.
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float VisionRadius = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float RefreshIntervalSeconds = 0.2f;

	// 인덱스=TeamId. 이 프로젝트 어디에도 팀→색상 매핑이 없어서 미니맵 전용으로 새로 둔다.
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	TArray<FLinearColor> TeamColors;

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	FLinearColor CampIconColor = FLinearColor::Yellow;

private:
	void RefreshMinimap();

	// 아레나 월드 좌표를 MinimapIconContainer 로컬 좌표로 선형 변환.
	FVector2D WorldToMinimapLocal(const FVector2D& WorldXY) const;

	FLinearColor GetColorForTeam(uint8 TeamId) const;

	// 살아있는 플레이어 아이콘 풀 — 매 리프레시마다 전부 새로 만들지 않고 생성/갱신/제거만 한다.
	UPROPERTY()
	TMap<TObjectPtr<AP1PlayerState>, TObjectPtr<UP1MinimapIconWidget>> PlayerIconPool;

	// 캠프는 위치가 고정이라 NativeConstruct에서 한 번만 캐싱 — 인덱스가 CampIconPool과 대응. 위치뿐
	// 아니라 앵커 자체를 들고 있어야 RespawnServerTime/TeamHasObservedDeath(팀 공유 안개 상태)를 매
	// 리프레시마다 읽을 수 있다.
	UPROPERTY()
	TArray<TObjectPtr<AP1JungleCampAnchor>> CachedCampAnchors;

	UPROPERTY()
	TArray<TObjectPtr<UP1MinimapCampIconWidget>> CampIconPool;

	// 캠프 위치는 매치 내내 고정이라 한 번만 계산하면 충분 — NativeConstruct 시점엔 컨테이너 지오메트리가
	// 아직 유효하지 않을 수 있어(레이아웃 계산 전), RefreshMinimap()이 매 틱 컨테이너 크기가 유효해졌는지
	// 확인하다가 처음 성공하는 순간 위치를 확정하고 이 플래그를 true로 잠근다(이후 틱은 재계산 스킵).
	bool bCampIconsPositioned = false;

	FTimerHandle RefreshTimerHandle;
};
