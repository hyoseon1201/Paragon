// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "P1GameplayCueNotify_MaterialOverride.generated.h"

class UMaterialInterface;

// GE(버프/디버프)의 활성 상태(적용~제거)에 정확히 대응해 특정 머티리얼 슬롯을 교체하는 GameplayCue.
// AnimNotifyState_MaterialOverride(P1AnimNotifyState_MaterialOverride)와 로직은 동일하지만,
// 그건 몽타주 타임라인(고정 길이)에 종속되는 반면 이건 GE의 실제 지속시간(조기 제거 시 그만큼 단축)에
// 정확히 대응하고, 시뮬레이티드 프록시(다른 클라이언트)에도 자동 복제된다.
// 사용법: 이 클래스를 상속한 GameplayCueNotify Blueprint를 만들어 GameplayCueTag를 지정하고,
// 해당 태그를 대상 GameplayEffect의 GameplayCues 목록에 등록한다.
UCLASS(meta = (DisplayName = "P1 Material Override Cue"))
class P1_API UP1GameplayCueNotify_MaterialOverride : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

	// 교체할 머티리얼 슬롯 이름 (Skeletal Mesh 에디터의 Material Slots에서 확인).
	UPROPERTY(EditAnywhere, Category = "GameplayCue")
	FName MaterialSlotName;

	// GE 활성 동안 적용할 머티리얼.
	UPROPERTY(EditAnywhere, Category = "GameplayCue")
	TObjectPtr<UMaterialInterface> OverrideMaterial;
};
