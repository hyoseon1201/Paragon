// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1HeroSelectSlotWidget.generated.h"

class UImage;
class UButton;
struct FP1HeroDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeroSlotSelected, FName, HeroId);

// PreGame 매칭 화면에 내장된 히어로 픽커(UP1HeroPickerWidget)의 한 칸 — 클릭하면 선택만 하고
// (FOnHeroSlotSelected 브로드캐스트) 선택 표시는 부모가 SetSelectedVisual()로 갱신해준다. 별도
// 하이라이트/이름 위젯 없이 PortraitImage 하나의 명암만으로 선택 여부를 표현한다(선택=밝게,
// 비선택=어둡게) — 위젯을 딱 둘(SelectButton+PortraitImage)만 필요로 하도록 최소화한 구성.
// ASC/위젯 컨트롤러가 필요 없는 순수 표시+클릭 위젯이라 UP1ShopItemSlotWidget과 동일하게
// UUserWidget을 직접 상속한다.
UCLASS()
class P1_API UP1HeroSelectSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 픽커가 슬롯을 채울 때마다 호출 — 표시용 포트레이트는 Row.HeroClass의 CDO에서 읽는다.
	void SetSlotData(const FP1HeroDefinition& Row);

	// 이 슬롯이 현재 선택된 히어로인지에 따라 PortraitImage의 명암을 바꾼다(밝게/어둡게) — 부모
	// (UP1HeroPickerWidget)가 전체 슬롯을 순회하며 호출.
	void SetSelectedVisual(bool bSelected);

	UPROPERTY(BlueprintAssignable, Category = "Hero")
	FOnHeroSlotSelected OnSelected;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PortraitImage;

	// 이 칸을 선택하는 버튼 — 배경 전체를 덮는 투명 버튼으로 만들면 자연스럽다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SelectButton;

	// 비선택 상태일 때 PortraitImage에 곱해줄 어두운 틴트 — 선택 상태는 항상 FLinearColor::White.
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	FLinearColor UnselectedTint = FLinearColor(0.35f, 0.35f, 0.35f, 1.0f);

private:
	UFUNCTION()
	void HandleSelectClicked();

	FName HeroId;
};
