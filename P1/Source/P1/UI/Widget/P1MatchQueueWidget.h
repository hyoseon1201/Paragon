// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1MatchQueueWidget.generated.h"

class UButton;
class UTextBlock;
class UP1BackendSubsystem;

// PreGame 매칭 대기 화면 전용 위젯. QueueButton 하나로 "매칭 시작"/"취소" 두 상태를 겸한다 —
// 대기 중이 아니면 클릭 시 JoinQueue(), 대기 중이면 클릭 시 LeaveQueue(). 대기 상태 표시(공용 상태
// 텍스트)나 매칭 성사 시 ClientTravel은 UP1PreGameHUDWidget이 백엔드 델리게이트를 직접 구독해서
// 처리하므로 이 위젯은 그 결과를 몰라도 되지만, "자기 버튼이 지금 큐에 들어가 있는 상태인지"는
// 스스로 알아야 하므로 OnQueueJoined/OnQueueLeft만 별도로 구독한다.
UCLASS()
class P1_API UP1MatchQueueWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QueueButton;

	// UButton 자체엔 SetText가 없어, WBP에서 버튼 안에 넣은 TextBlock을 별도로 바인딩해야
	// "매칭 시작"/"취소" 라벨 전환이 가능하다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QueueButtonLabel;

private:
	UFUNCTION()
	void HandleQueueClicked();
	UFUNCTION()
	void HandleQueueJoined(bool bSuccess, FString ErrorMessage);
	UFUNCTION()
	void HandleQueueLeft(bool bSuccess, FString ErrorMessage);

	UP1BackendSubsystem* GetBackendSubsystem() const;
	void SetQueuedState(bool bInIsQueued);

	bool bIsQueued = false;
};
