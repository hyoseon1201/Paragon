// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1SkillIconWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;

// 사각형 스킬 아이콘. 쿨타임 비주얼은 2개 레이어로 구성:
// 1) SkillIconImage 자체의 Tint - 준비완료면 ReadyTint(기본 흰색), 쿨타임중이면 CooldownTint(기본 0.3 어둡게)
// 2) CooldownOverlay - CooldownTint보다 더 어두운 Fill 색을 가진 ProgressBar(FillType=BottomToTop).
//    Percent 1(방금 사용)→0(준비) 로 줄어들면서 위쪽부터 점점 Fill이 사라져 아이콘이 드러난다.
//    (머티리얼 Opacity로 시도했으나 이 프로젝트의 Substrate+UserInterface 도메인 조합에서 픽셀 단위
//    알파가 작동하지 않아 - BSDF는 Domain을 Surface로 강제 전환, 레거시 Opacity는 무시됨 - 엔진 내장
//    ProgressBar의 FillType으로 대체함)
UCLASS()
class P1_API UP1SkillIconWidget : public UP1UserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SkillIcon")
	void SetSkillIcon(UTexture2D* IconTexture);

	// 쿨타임 시작. GAS ability tag에서 cooldown duration 수신 후 호출.
	UFUNCTION(BlueprintCallable, Category = "SkillIcon")
	void StartCooldown(float TotalCooldown);

	UFUNCTION(BlueprintCallable, Category = "SkillIcon")
	void ClearCooldown();

	// 준비완료 상태의 SkillIconImage Tint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillIcon")
	FLinearColor ReadyTint = FLinearColor::White;

	// 쿨타임중 상태의 SkillIconImage Tint - CooldownOverlay 머티리얼의 색보다는 밝아야 함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillIcon")
	FLinearColor CooldownTint = FLinearColor(0.3f, 0.3f, 0.3f, 1.f);

protected:
	// WBP에서 동일 이름으로 배치 필수
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillIconImage;

	// FillType=BottomToTop, Background는 완전 투명(Alpha=0)으로 WBP에서 설정할 것
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> CooldownOverlay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void UpdateCooldownDisplay();
	FString FormatCooldown(float Seconds) const;

	float TotalCooldownTime = 0.f;
	float RemainingCooldown = 0.f;
	bool bOnCooldown = false;
};
