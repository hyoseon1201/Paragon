// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1HeroPickerWidget.generated.h"

class UDataTable;
class UPanelWidget;
class UP1HeroSelectSlotWidget;
class UP1BackendSubsystem;

// PreGame 매칭 대기 화면(UP1MatchQueueWidget) 안에 내장되는 히어로 픽커. 별도 화면 전환 없이
// 항상 함께 보이며, 슬롯 클릭 즉시 선택이 확정된다(별도 확인 버튼 없음 — UP1BackendSubsystem의
// SelectedHeroId가 기본 Greystone이라 "미선택" 상태 자체가 없으므로 즉시 커밋해도 안전).
// ASC/위젯 컨트롤러가 필요 없는 순수 표시+클릭 위젯이라 UUserWidget을 직접 상속.
UCLASS()
class P1_API UP1HeroPickerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 매칭 대기 중엔 UP1MatchQueueWidget::SetQueuedState()가 호출 — 컨테이너 전체를 비활성화해
	// 대기 중 히어로 변경을 막는다(UMG SetIsEnabled는 자식 위젯에 전파됨).
	void SetLocked(bool bLocked);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	TObjectPtr<UDataTable> HeroTable;

	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	TSubclassOf<UP1HeroSelectSlotWidget> HeroSlotWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> HeroSlotContainer;

private:
	UFUNCTION()
	void HandleHeroSlotSelected(FName HeroId);

	void RefreshHighlights(FName SelectedHeroId);

	UP1BackendSubsystem* GetBackendSubsystem() const;

	UPROPERTY()
	TMap<FName, TObjectPtr<UP1HeroSelectSlotWidget>> SlotsByHeroId;
};
