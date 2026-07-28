// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1FloatingStatusWidget.generated.h"

class UTextBlock;
class UWidget;
class UProgressBar;
class UP1SegmentedBarWidget;
class UP1FloatingStatusWidgetController;

// 캐릭터 머리 위 월드스페이스 위젯. 레벨 · HP · MP 바를 표시.
// HP/MP 바는 WBP_P1Overlay의 HealthBar/ManaBar와 동일한 UP1SegmentedBarWidget을 재사용한다 —
// 같은 WBP 에셋을 여기 배치할 때 인스턴스 프로퍼티에서 bShowLabels=false로 두면 숫자 텍스트만 숨겨진다.
UCLASS()
class P1_API UP1FloatingStatusWidget : public UP1UserWidget
{
	GENERATED_BODY()

public:
	// 캐릭터 이름(예: "Greystone") 표시 — Health/Mana처럼 계속 바뀌는 값이 아니라 한 번만 세팅하면
	// 되므로 위젯 컨트롤러 델리게이트를 거치지 않고 캐릭터 쪽에서 직접 호출한다.
	UFUNCTION(BlueprintCallable, Category = "FloatingStatus")
	void SetCharacterName(const FText& NewName);

	// 캐릭터 이름과 동일한 이유로 위젯 컨트롤러 델리게이트를 거치지 않고 직접 호출 — 정글 몬스터는
	// 스폰 시점 레벨이 고정이라 한 번만 세팅하면 된다(BeginPlay에서 SetCharacterName과 함께 호출).
	UFUNCTION(BlueprintCallable, Category = "FloatingStatus")
	void SetLevel(int32 NewLevel);

protected:
	virtual void OnWidgetControllerSet() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1SegmentedBarWidget> HPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UP1SegmentedBarWidget> MPBar;

	// 남은 시간 비율(1.0=방금 스턴 시작 → 0.0=곧 해제)로 채워지는 바 — UP1SkillIconWidget::CooldownOverlay와
	// 동일한 용법(Percent를 매 틱 갱신). FillType은 WBP에서 설정(LeftToRight 추천). 스턴이 아닐 땐
	// Percent=0으로 비워 숨김을 표현하므로 WBP에서 Background 브러시는 투명(알파 0)으로 둔다 —
	// Visibility/RenderOpacity 토글은 이 WidgetComponent에서 렌더타겟 재갱신을 못 일으켜 화면에 반영이
	// 안 되므로(HP/MP 바처럼 콘텐츠 변경만 반영됨), 표시/숨김을 전부 콘텐츠(퍼센트)로만 처리한다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> StunProgressBar;

	// "Stun" 라벨 — 스턴 중엔 "Stun", 아닐 땐 빈 문자열로 바꿔 숨김을 표현한다. Visibility가 아니라
	// 텍스트 내용(SetText)으로 껐다 켜는 이유는 위 StunProgressBar 주석과 동일(WidgetComponent 렌더타겟
	// 갱신은 콘텐츠 변경에만 반응). WBP에 이 이름의 TextBlock을 배치하면 되고, 없어도 크래시는 안 남.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StunLabelText;

private:
	UFUNCTION()
	void OnHealthChanged(float NewValue);
	UFUNCTION()
	void OnMaxHealthChanged(float NewValue);
	UFUNCTION()
	void OnManaChanged(float NewValue);
	UFUNCTION()
	void OnMaxManaChanged(float NewValue);
	UFUNCTION()
	void OnStunStateChanged(bool bIsStunned, float Duration);

	void RefreshBars();
	void UpdateStunDisplay();

	float CachedHealth = 1.f;
	float CachedMaxHealth = 1.f;
	float CachedMana = 0.f;
	float CachedMaxMana = 1.f;

	bool bIsStunned = false;
	float RemainingStunDuration = 0.f;
	float TotalStunDuration = 0.f;
};
