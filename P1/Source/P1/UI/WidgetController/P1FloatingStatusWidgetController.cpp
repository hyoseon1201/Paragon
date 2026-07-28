// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/WidgetController/P1FloatingStatusWidgetController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Player/P1PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "P1.h"

void UP1FloatingStatusWidgetController::BroadcastInitialValues()
{
	bool bFound = false;
	OnHealthChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetHealthAttribute(), bFound));
	OnMaxHealthChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetMaxHealthAttribute(), bFound));
	OnManaChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetManaAttribute(), bFound));
	OnMaxManaChanged.Broadcast(AbilitySystemComponent->GetGameplayAttributeValue(UP1AttributeSet::GetMaxManaAttribute(), bFound));

	// 컨트롤러/위젯이 생성되는 시점에 캐릭터가 이미 스턴 상태일 수도 있다(예: 늦게 접속한 관전 시점 등) —
	// 초기값도 다른 델리게이트들과 동일하게 한 번 브로드캐스트해 커버한다.
	RefreshStun();
}

void UP1FloatingStatusWidgetController::BindCallbacksToDependencies()
{
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetHealthAttribute())
		.AddUObject(this, &UP1FloatingStatusWidgetController::OnHealthAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UP1FloatingStatusWidgetController::OnMaxHealthAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetManaAttribute())
		.AddUObject(this, &UP1FloatingStatusWidgetController::OnManaAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UP1AttributeSet::GetMaxManaAttribute())
		.AddUObject(this, &UP1FloatingStatusWidgetController::OnMaxManaAttributeChanged);

	AbilitySystemComponent->RegisterGameplayTagEvent(TAG_State_Stunned, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UP1FloatingStatusWidgetController::OnStunnedTagChanged);

	// StunEndServerTime이 복제돼 들어오는 순간(또는 서버에서 세팅되는 순간)에도 스턴바를 다시 그린다 —
	// 태그와 시간값이 서로 다른 프레임에 도착할 수 있어(순서 무보장) 양쪽 다 구독해야 정확한 카운트다운으로
	// 수렴한다.
	if (AP1PlayerState* P1PS = Cast<AP1PlayerState>(PlayerState))
	{
		P1PS->OnStunTimeChangedNative.AddUObject(this, &UP1FloatingStatusWidgetController::OnStunTimeChanged);
	}
}

void UP1FloatingStatusWidgetController::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)   { OnHealthChanged.Broadcast(Data.NewValue); }
void UP1FloatingStatusWidgetController::OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data){ OnMaxHealthChanged.Broadcast(Data.NewValue); }
void UP1FloatingStatusWidgetController::OnManaAttributeChanged(const FOnAttributeChangeData& Data)     { OnManaChanged.Broadcast(Data.NewValue); }
void UP1FloatingStatusWidgetController::OnMaxManaAttributeChanged(const FOnAttributeChangeData& Data)  { OnMaxManaChanged.Broadcast(Data.NewValue); }

void UP1FloatingStatusWidgetController::OnStunnedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	RefreshStun();
}

void UP1FloatingStatusWidgetController::OnStunTimeChanged()
{
	RefreshStun();
}

void UP1FloatingStatusWidgetController::RefreshStun()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// 표시/숨김은 State.Stunned 태그 카운트로 결정한다 — 이 태그는 Mixed 모드에서도 전 클라(적을 보는
	// 프록시 포함)에 복제되므로, 어느 월드에서 보든 정확히 켜지고 꺼진다.
	if (AbilitySystemComponent->GetTagCount(TAG_State_Stunned) <= 0)
	{
		OnStunStateChanged.Broadcast(false, 0.0f);
		return;
	}

	// 남은시간은 복제된 StunEndServerTime과 동기화된 서버 시계(GameState->GetServerWorldTimeSeconds())의
	// 차이로 계산 — 서버/소유클라/프록시 전부 같은 기준이라 정확하다. StunEndServerTime이 아직 도착 전
	// (이 함수가 태그 이벤트로 먼저 불린 경우)이면 R<=0이 나올 수 있는데, 그땐 폴백 1초로 바를 채우고
	// StunEndServerTime OnRep이 도착하면 다시 이 함수가 불려 정확한 값으로 보정된다.
	float Remaining = 1.0f;
	if (const AP1PlayerState* P1PS = Cast<AP1PlayerState>(PlayerState))
	{
		if (const UWorld* World = P1PS->GetWorld())
		{
			if (const AGameStateBase* GS = World->GetGameState())
			{
				const float R = P1PS->GetStunEndServerTime() - GS->GetServerWorldTimeSeconds();
				if (R > 0.0f)
				{
					Remaining = R;
				}
			}
		}
	}
	OnStunStateChanged.Broadcast(true, Remaining);
}
