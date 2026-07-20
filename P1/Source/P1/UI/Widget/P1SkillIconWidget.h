// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/P1UserWidget.h"
#include "P1SkillIconWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
class UWidget;

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

	// 포인트 투자 가능 표시(InvestButton, WBP에 배치 안 하면 그냥 무시됨) 노출 여부.
	// 투자는 클릭이 아니라 Ctrl+스킬키로 이뤄지므로(AP1PlayerController 참고) 이건 순수 시각적
	// 인디케이터일 뿐 — 클릭 이벤트를 받지 않는다. 투자 가능한 4개 슬롯(RMB/Q/E/R)에만 WBP에서 배치.
	UFUNCTION(BlueprintCallable, Category = "SkillIcon")
	void SetInvestButtonVisible(bool bVisible);

	// 어빌리티 레벨이 0(아직 포인트 미투자)이면 SkillIconImage에 CooldownTint를 적용해 쿨다운처럼
	// 어둡게 표시한다 — 잠긴 상태라는 걸 시각적으로 알려준다. 잠금 해제(투자)되면 ReadyTint로 복귀
	// (단, 마침 쿨다운 중이면 그쪽이 우선 — 잠긴 어빌리티는 애초에 쿨다운을 탈 수 없으니 실전에서 겹칠 일은 없음).
	UFUNCTION(BlueprintCallable, Category = "SkillIcon")
	void SetLocked(bool bLocked);

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

	// 스킬 아이콘 위쪽(y좌표가 더 작은 쪽)에 배치하는 순수 시각적 표시기 — 어떤 UWidget 타입이든
	// 무방(UButton으로 만들었어도 클릭은 안 씀, 그냥 Visibility만 켜고 끔). 투자 가능한 4개 슬롯에만
	// WBP에 배치할 것 — 없는 슬롯(LMB/Passive)은 SetInvestButtonVisible이 그냥 조용히 무시된다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> InvestButton;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void UpdateCooldownDisplay();
	FString FormatCooldown(float Seconds) const;

	float TotalCooldownTime = 0.f;
	float RemainingCooldown = 0.f;
	bool bOnCooldown = false;
	bool bIsLocked = false;
};
