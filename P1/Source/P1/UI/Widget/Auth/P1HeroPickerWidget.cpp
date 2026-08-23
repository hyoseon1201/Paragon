// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Auth/P1HeroPickerWidget.h"
#include "UI/Widget/Auth/P1HeroSelectSlotWidget.h"
#include "Characters/P1HeroTypes.h"
#include "Online/P1BackendSubsystem.h"
#include "Components/PanelWidget.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "P1.h"

void UP1HeroPickerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!HeroTable || !HeroSlotWidgetClass || !HeroSlotContainer)
	{
		UE_LOG(LogP1, Warning, TEXT("[HeroPicker] HeroTable/HeroSlotWidgetClass/HeroSlotContainer 중 하나가 비어있음 — WBP 설정 확인 필요"));
		return;
	}

	TArray<FP1HeroDefinition*> Rows;
	HeroTable->GetAllRows<FP1HeroDefinition>(TEXT("UP1HeroPickerWidget::NativeConstruct"), Rows);

	for (const FP1HeroDefinition* Row : Rows)
	{
		if (!Row || Row->HeroId.IsNone())
		{
			continue;
		}

		UP1HeroSelectSlotWidget* SlotWidget = CreateWidget<UP1HeroSelectSlotWidget>(this, HeroSlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetSlotData(*Row);
		SlotWidget->OnSelected.AddDynamic(this, &UP1HeroPickerWidget::HandleHeroSlotSelected);

		// HorizontalBox/VerticalBox 슬롯은 기본이 Fill(컨테이너 크기를 균등 분할)이라 슬롯 개수가
		// 바뀔 때마다 칸 크기가 흔들린다 — Size to Content(Automatic)로 바꿔서 각 슬롯이 자기
		// PortraitImage 크기만큼만 차지하게 한다. WrapBox 등 다른 패널은 이미 콘텐츠 크기로 배치되므로
		// 해당 없음(캐스트 실패 시 조용히 스킵).
		UPanelSlot* NewSlot = HeroSlotContainer->AddChild(SlotWidget);
		if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(NewSlot))
		{
			HBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
		else if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(NewSlot))
		{
			VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		SlotsByHeroId.Add(Row->HeroId, SlotWidget);
	}

	const UP1BackendSubsystem* Backend = GetBackendSubsystem();
	RefreshHighlights(Backend ? Backend->GetSelectedHeroId() : NAME_None);
}

void UP1HeroPickerWidget::HandleHeroSlotSelected(FName HeroId)
{
	if (UP1BackendSubsystem* Backend = GetBackendSubsystem())
	{
		Backend->SetSelectedHeroId(HeroId);
	}

	RefreshHighlights(HeroId);
}

void UP1HeroPickerWidget::RefreshHighlights(FName SelectedHeroId)
{
	for (const TPair<FName, TObjectPtr<UP1HeroSelectSlotWidget>>& Pair : SlotsByHeroId)
	{
		if (Pair.Value)
		{
			Pair.Value->SetSelectedVisual(Pair.Key == SelectedHeroId);
		}
	}
}

void UP1HeroPickerWidget::SetLocked(bool bLocked)
{
	if (HeroSlotContainer)
	{
		HeroSlotContainer->SetIsEnabled(!bLocked);
	}
}

UP1BackendSubsystem* UP1HeroPickerWidget::GetBackendSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UP1BackendSubsystem>() : nullptr;
}
