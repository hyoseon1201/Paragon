// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1SkillIconWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;

// 다이아몬드 형태 스킬 아이콘. CooldownOverlay Image에 머터리얼(CooldownPercent 파라미터)을
// 설정하면 아래→위 방향으로 쿨타임이 채워지고, 잔여시간이 CooldownText에 표시된다.
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

protected:
	// WBP에서 동일 이름으로 배치 필수
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillIconImage;

	// CooldownPercent(0~1) 파라미터를 가진 머터리얼을 Brush로 설정할 것
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CooldownOverlay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void UpdateCooldownDisplay();
	FString FormatCooldown(float Seconds) const;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CooldownMID;

	float TotalCooldownTime = 0.f;
	float RemainingCooldown = 0.f;
	bool bOnCooldown = false;
};
