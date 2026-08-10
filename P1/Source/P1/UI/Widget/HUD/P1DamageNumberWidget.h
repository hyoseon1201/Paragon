// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1DamageNumberWidget.generated.h"

class UTextBlock;

// 데미지 팝업 텍스트 1회성 위젯 — 위젯 컨트롤러/ASC 연결이 필요 없는 단순 값 표시라
// UP1UserWidget이 아니라 UUserWidget을 직접 상속(UP1PreGameHUDWidget과 같은 이유).
// AP1DamageNumberActor가 스폰 직후 SetDamageAmount()를 한 번 호출하고 끝 — 색상/폰트/등장·소멸
// 애니메이션은 전부 WBP 디자이너가 담당(UMG Animation을 NativeConstruct에서 자동 재생하도록 구성해도 됨).
UCLASS()
class P1_API UP1DamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// bIsMagicalDamage로 PhysicalColor/MagicalColor 중 하나를 골라 텍스트 색상까지 함께 적용한다.
	UFUNCTION(BlueprintCallable, Category = "DamageNumber")
	void SetDamageAmount(float Amount, bool bIsMagicalDamage);

	// 물리 데미지 색상(기본 빨강) — WBP에서 자유롭게 조정 가능(UP1SkillIconWidget::ReadyTint와 동일한 패턴).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber")
	FLinearColor PhysicalColor = FLinearColor(0.85f, 0.1f, 0.1f, 1.0f);

	// 마법 데미지 색상(기본 파랑).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber")
	FLinearColor MagicalColor = FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageText;
};
