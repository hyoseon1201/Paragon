// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/P1WidgetController.h"
#include "GameplayEffectTypes.h"
#include "P1FloatingStatusWidgetController.generated.h"

// 캐릭터 머리 위 월드스페이스 위젯용 컨트롤러.
// AP1CharacterBase가 per-instance로 생성하며 AP1HUD와 무관하다.
UCLASS(BlueprintType)
class P1_API UP1FloatingStatusWidgetController : public UP1WidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAttributeChangedSignature OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAttributeChangedSignature OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAttributeChangedSignature OnMaxManaChanged;

	// bIsStunned=false일 땐 Duration 무의미(0). State.Stunned 태그 카운트 0→양수/양수→0 전이마다 브로드캐스트.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStunStateChangedSignature, bool, bIsStunned, float, Duration);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStunStateChangedSignature OnStunStateChanged;

private:
	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void OnManaAttributeChanged(const FOnAttributeChangeData& Data);
	void OnMaxManaAttributeChanged(const FOnAttributeChangeData& Data);

	// 스턴바 상태를 다시 계산해 브로드캐스트하는 공통 진입점. 두 트리거가 각자 시점에 이걸 호출한다:
	//  (1) State.Stunned 태그 카운트 변경(전원 복제) → OnStunnedTagChanged → 표시/숨김 결정
	//  (2) AP1PlayerState::StunEndServerTime 도착(OnStunTimeChangedNative) → OnStunTimeChanged → 정확한 남은시간
	// 태그와 시간값의 도착 순서가 무보장이라, 어느 게 먼저 와도 마지막엔 RefreshStun이 정확한 상태로
	// 수렴하게 만든다(먼저 온 쪽이 폴백/스킵, 나중 온 쪽이 보정).
	void RefreshStun();
	void OnStunnedTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnStunTimeChanged();
};
