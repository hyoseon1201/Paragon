// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Minimap/P1MinimapWidget.h"
#include "UI/Widget/Minimap/P1MinimapIconWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "GameModes/P1GameState.h"
#include "Player/P1PlayerState.h"
#include "AI/P1JungleCampAnchor.h"
#include "Characters/P1JungleMonsterCharacter.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "P1.h"

void UP1MinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<AP1JungleCampAnchor> It(World); It; ++It)
		{
			CachedCampLocations.Add(It->GetActorLocation());
		}
	}

	if (!IconWidgetClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[Minimap] IconWidgetClass 미설정 — WBP_Minimap Class Defaults에서 지정하세요."));
	}
	else if (MinimapIconContainer)
	{
		for (const FVector& CampLocation : CachedCampLocations)
		{
			UP1MinimapIconWidget* CampIcon = CreateWidget<UP1MinimapIconWidget>(GetOwningPlayer(), IconWidgetClass);
			if (!CampIcon)
			{
				continue;
			}

			CampIcon->SetIconColor(CampIconColor);
			CampIcon->SetVisibility(ESlateVisibility::Collapsed);

			if (UCanvasPanelSlot* CanvasSlot = MinimapIconContainer->AddChildToCanvas(CampIcon))
			{
				CanvasSlot->SetPosition(WorldToMinimapLocal(FVector2D(CampLocation)));
			}

			CampIconPool.Add(CampIcon);
		}
	}

	if (World)
	{
		World->GetTimerManager().SetTimer(RefreshTimerHandle, this, &UP1MinimapWidget::RefreshMinimap, RefreshIntervalSeconds, true);
	}
}

void UP1MinimapWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	Super::NativeDestruct();
}

FVector2D UP1MinimapWidget::WorldToMinimapLocal(const FVector2D& WorldXY) const
{
	const FVector2D BoundsSize = WorldBoundsMax - WorldBoundsMin;
	const FVector2D Alpha(
		BoundsSize.X != 0.0 ? (WorldXY.X - WorldBoundsMin.X) / BoundsSize.X : 0.0,
		BoundsSize.Y != 0.0 ? (WorldXY.Y - WorldBoundsMin.Y) / BoundsSize.Y : 0.0);

	const FVector2D ContainerSize = MinimapIconContainer
		? MinimapIconContainer->GetCachedGeometry().GetLocalSize()
		: FVector2D(200.0, 200.0);

	return FVector2D(Alpha.X * ContainerSize.X, Alpha.Y * ContainerSize.Y);
}

FLinearColor UP1MinimapWidget::GetColorForTeam(uint8 TeamId) const
{
	return TeamColors.IsValidIndex(TeamId) ? TeamColors[TeamId] : FLinearColor::White;
}

void UP1MinimapWidget::RefreshMinimap()
{
	if (!MinimapIconContainer || !IconWidgetClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	AP1GameState* GameState = World ? World->GetGameState<AP1GameState>() : nullptr;
	APlayerController* OwningPC = GetOwningPlayer();
	AP1PlayerState* LocalPS = OwningPC ? Cast<AP1PlayerState>(OwningPC->PlayerState) : nullptr;
	if (!GameState || !LocalPS)
	{
		return;
	}

	const uint8 LocalTeamId = LocalPS->GetGenericTeamId().GetId();

	// 1. 살아있는 아군 Pawn 위치 수집 — 시야 제공용(죽은 아군은 시야를 안 준다).
	TArray<FVector> AllyLocations;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		AP1PlayerState* P1PS = Cast<AP1PlayerState>(PS);
		APawn* AllyPawn = P1PS ? P1PS->GetPawn() : nullptr;
		if (!P1PS || !AllyPawn || P1PS->GetGenericTeamId().GetId() != LocalTeamId)
		{
			continue;
		}

		const UAbilitySystemComponent* ASC = P1PS->GetAbilitySystemComponent();
		if (ASC && ASC->HasMatchingGameplayTag(TAG_State_Dead))
		{
			continue;
		}

		AllyLocations.Add(AllyPawn->GetActorLocation());
	}

	// 2. 플레이어 아이콘 — 아군은 항상, 적은 아군 시야 반경 이내일 때만 표시.
	const float VisionRadiusSq = FMath::Square(VisionRadius);
	TSet<AP1PlayerState*> VisiblePlayers;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		AP1PlayerState* P1PS = Cast<AP1PlayerState>(PS);
		APawn* Pawn = P1PS ? P1PS->GetPawn() : nullptr;
		if (!P1PS || !Pawn)
		{
			continue;
		}

		const bool bIsAlly = P1PS->GetGenericTeamId().GetId() == LocalTeamId;
		bool bVisible = bIsAlly;
		if (!bIsAlly)
		{
			const FVector EnemyLocation = Pawn->GetActorLocation();
			for (const FVector& AllyLocation : AllyLocations)
			{
				if (FVector::DistSquared(EnemyLocation, AllyLocation) <= VisionRadiusSq)
				{
					bVisible = true;
					break;
				}
			}
		}

		if (!bVisible)
		{
			continue;
		}

		VisiblePlayers.Add(P1PS);

		UP1MinimapIconWidget* Icon = PlayerIconPool.FindRef(P1PS);
		if (!Icon)
		{
			Icon = CreateWidget<UP1MinimapIconWidget>(OwningPC, IconWidgetClass);
			if (!Icon)
			{
				continue;
			}
			MinimapIconContainer->AddChildToCanvas(Icon);
			PlayerIconPool.Add(P1PS, Icon);
		}

		Icon->SetIconColor(GetColorForTeam(P1PS->GetGenericTeamId().GetId()));
		Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Icon->Slot))
		{
			CanvasSlot->SetPosition(WorldToMinimapLocal(FVector2D(Pawn->GetActorLocation())));
		}
	}

	// 더 이상 안 보이는 플레이어의 아이콘은 완전히 제거(풀에서도 지움 — Collapsed로 숨기고 끝내지
	// 않는 이유: 죽은 PlayerState를 계속 들고 있을 이유가 없고, 다음에 다시 보일 때 새로 만들면 됨).
	for (auto It = PlayerIconPool.CreateIterator(); It; ++It)
	{
		if (!VisiblePlayers.Contains(It->Key))
		{
			if (It->Value)
			{
				It->Value->RemoveFromParent();
			}
			It.RemoveCurrent();
		}
	}

	// 3. 캠프 아이콘 — 캠프 위치 CampDetectionRadius 안에 살아있는 몬스터가 있으면 표시.
	const float CampDetectionRadiusSq = FMath::Square(CampDetectionRadius);
	for (int32 Index = 0; Index < CachedCampLocations.Num(); ++Index)
	{
		if (!CampIconPool.IsValidIndex(Index) || !CampIconPool[Index])
		{
			continue;
		}

		bool bCampAlive = false;
		for (TActorIterator<AP1JungleMonsterCharacter> It(World); It; ++It)
		{
			if (FVector::DistSquared(It->GetActorLocation(), CachedCampLocations[Index]) <= CampDetectionRadiusSq)
			{
				bCampAlive = true;
				break;
			}
		}

		CampIconPool[Index]->SetVisibility(bCampAlive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
