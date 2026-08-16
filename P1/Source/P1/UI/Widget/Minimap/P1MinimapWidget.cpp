// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Minimap/P1MinimapWidget.h"
#include "UI/Widget/Minimap/P1MinimapIconWidget.h"
#include "UI/Widget/Minimap/P1MinimapCampIconWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "GameModes/P1GameState.h"
#include "Player/P1PlayerState.h"
#include "AI/P1JungleCampAnchor.h"
#include "Characters/P1HeroCharacter.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

void UP1MinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<AP1JungleCampAnchor> It(World); It; ++It)
		{
			CachedCampAnchors.Add(*It);
		}
	}

	if (CampIconWidgetClass && MinimapIconContainer)
	{
		for (AP1JungleCampAnchor* CampAnchor : CachedCampAnchors)
		{
			if (!IsValid(CampAnchor))
			{
				continue;
			}

			UP1MinimapCampIconWidget* CampIcon = CreateWidget<UP1MinimapCampIconWidget>(GetOwningPlayer(), CampIconWidgetClass);
			if (!CampIcon)
			{
				continue;
			}

			CampIcon->SetIconColor(CampIconColor);
			// 안개 시스템상 캠프 아이콘은 항상 보인다(생사와 무관 — 팀이 죽음을 확인 못했으면 계속
			// "살아있음"으로 스테일 표시된다, 아래 RefreshMinimap 참고) — Collapsed로 시작하지 않는다.
			CampIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			// 위치는 여기서 굳이 안 잡는다 — NativeConstruct 시점엔 MinimapIconContainer의 레이아웃이
			// 아직 계산되기 전이라(GetCachedGeometry가 크기 0을 반환) 좌표 변환이 전부 (0,0)으로
			// 나온다. 플레이어 아이콘과 동일하게 RefreshMinimap()이 매 틱 다시 계산해서 채워준다.
			if (UCanvasPanelSlot* CanvasSlot = MinimapIconContainer->AddChildToCanvas(CampIcon))
			{
				CanvasSlot->SetAutoSize(true); // 위젯 디자인 크기 그대로 사용 — Size To Content.
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // SetPosition 기준점을 좌상단이 아닌 중심으로.
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
	// 미니맵 이미지의 가로축은 월드 Y, 세로축은 월드 X에 대응한다(캡처한 Top뷰 기준 실측 보정 —
	// WorldXY.X/Y를 그대로 Local.X/Y에 매칭하면 좌우/상하가 뒤바뀐다).
	const FVector2D BoundsSize = WorldBoundsMax - WorldBoundsMin;
	const FVector2D Alpha(
		BoundsSize.Y != 0.0 ? (WorldXY.Y - WorldBoundsMin.Y) / BoundsSize.Y : 0.0,
		BoundsSize.X != 0.0 ? (WorldXY.X - WorldBoundsMin.X) / BoundsSize.X : 0.0);

	const FVector2D ContainerSize = MinimapIconContainer
		? MinimapIconContainer->GetCachedGeometry().GetLocalSize()
		: FVector2D(200.0, 200.0);

	return FVector2D(Alpha.X * ContainerSize.X, Alpha.Y * ContainerSize.Y);
}

FLinearColor UP1MinimapWidget::GetColorForTeam(uint8 TeamId) const
{
	// WBP Class Defaults의 Alpha가 에디터 표시값(1.0)과 무관하게 런타임 CDO에서 0으로 직렬화되는
	// 현상이 확인됨(TeamColors 전 항목 동일 현상) — 팀 테두리는 항상 완전 불투명이어야 하므로
	// Alpha는 배열 값을 신뢰하지 않고 여기서 강제한다.
	FLinearColor Color = TeamColors.IsValidIndex(TeamId) ? TeamColors[TeamId] : FLinearColor::White;
	Color.A = 1.0f;
	return Color;
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
			if (UCanvasPanelSlot* NewSlot = MinimapIconContainer->AddChildToCanvas(Icon))
			{
				NewSlot->SetAutoSize(true); // 위젯 디자인 크기 그대로 사용 — Size To Content.
				NewSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // SetPosition 기준점을 좌상단이 아닌 중심으로.
			}
			PlayerIconPool.Add(P1PS, Icon);
		}

		Icon->SetIconColor(GetColorForTeam(P1PS->GetGenericTeamId().GetId()));

		// 영웅 초상화는 스폰된 Pawn의 실제 클래스(CDO)에서 읽는다 — 영웅 BP마다 다른 값이라 리플리케이션이
		// 필요 없다(원격 클라이언트도 이미 표준 액터 클래스 리플리케이션으로 어떤 영웅인지 알고 있음).
		if (const AP1HeroCharacter* HeroCDO = Cast<AP1HeroCharacter>(Pawn->GetClass()->GetDefaultObject()))
		{
			Icon->SetPortraitTexture(HeroCDO->GetHeroPortrait());
		}

		Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		const FVector2D LocalPos = WorldToMinimapLocal(FVector2D(Pawn->GetActorLocation()));
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Icon->Slot))
		{
			CanvasSlot->SetPosition(LocalPos);
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

	// 3. 캠프 아이콘 — 실제 생사가 아니라 "우리 팀이 확인했는가"(팀 공유 안개)만 반영한다. 미확인
	// 상태에선 실제로 죽었어도 계속 "살아있음"으로 스테일 표시되고, 확인되면 다이아몬드를 어둡게
	// 칠하고 남은 리스폰 시간을 카운트다운으로 보여준다. 부활은 확인 여부와 무관하게 즉시 반영되는데,
	// 이는 AP1JungleCampAnchor::RespawnServerTime이 리스폰 순간 -1로 리셋되기 때문에 별도 분기 없이
	// 자연히 성립한다.
	// 캠프 위치는 고정이라 한 번만 계산하면 되지만, NativeConstruct 시점엔 컨테이너 레이아웃이 아직
	// 안 잡혀있어(GetCachedGeometry가 크기 0을 반환) 그때 바로 계산하면 전부 (0,0)으로 어긋난다 —
	// 컨테이너 크기가 유효해지는 첫 틱에 딱 한 번만 위치를 확정하고 이후로는 건너뛴다.
	if (!bCampIconsPositioned)
	{
		const FVector2D ContainerSize = MinimapIconContainer->GetCachedGeometry().GetLocalSize();
		if (!ContainerSize.IsNearlyZero())
		{
			for (int32 Index = 0; Index < CachedCampAnchors.Num(); ++Index)
			{
				AP1JungleCampAnchor* CampAnchor = CachedCampAnchors[Index];
				if (!CampIconPool.IsValidIndex(Index) || !CampIconPool[Index] || !IsValid(CampAnchor))
				{
					continue;
				}

				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CampIconPool[Index]->Slot))
				{
					CanvasSlot->SetPosition(WorldToMinimapLocal(FVector2D(CampAnchor->GetActorLocation())));
				}
			}
			bCampIconsPositioned = true;
		}
	}

	for (int32 Index = 0; Index < CachedCampAnchors.Num(); ++Index)
	{
		AP1JungleCampAnchor* CampAnchor = CachedCampAnchors[Index];
		if (!CampIconPool.IsValidIndex(Index) || !CampIconPool[Index] || !IsValid(CampAnchor))
		{
			continue;
		}

		UP1MinimapCampIconWidget* CampIcon = CampIconPool[Index];
		const float CampRespawnServerTime = CampAnchor->GetRespawnServerTime();
		const bool bTeamConfirmedDead = CampRespawnServerTime >= 0.0f && CampAnchor->HasTeamObservedDeath(LocalTeamId);

		if (bTeamConfirmedDead)
		{
			FLinearColor DimmedColor = CampIconColor;
			DimmedColor.A = 0.3f;
			CampIcon->SetIconColor(DimmedColor);
			CampIcon->SetCountdownSeconds(CampRespawnServerTime - GameState->GetServerWorldTimeSeconds());
		}
		else
		{
			CampIcon->SetIconColor(CampIconColor);
			CampIcon->ClearCountdown();
		}
	}
}
