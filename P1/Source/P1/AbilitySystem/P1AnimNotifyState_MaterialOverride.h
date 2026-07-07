// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "P1AnimNotifyState_MaterialOverride.generated.h"

class UMaterialInterface;

// 범용 머티리얼 오버라이드 AnimNotifyState.
// 노티파이 구간 동안 지정한 머티리얼 슬롯을 OverrideMaterial로 교체하고, 끝나면 원래대로 되돌린다.
// 검 화염 이펙트처럼 "스킬 사용 중에만 무기가 특정 룩으로 바뀌는" 연출에 사용.
// AnimNotifyState는 몽타주를 재생하는 모든 인스턴스(다른 클라이언트 포함)에서 발동하므로,
// GAS 어빌리티 코드(로컬 클라이언트+서버에서만 실행)로 SetMaterial하는 것과 달리
// 다른 플레이어 화면에도 정상적으로 보인다.
UCLASS(meta = (DisplayName = "Material Override"))
class P1_API UP1AnimNotifyState_MaterialOverride : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	// 교체할 머티리얼 슬롯 이름 (Skeletal Mesh 에디터의 Material Slots에서 확인).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	FName MaterialSlotName;

	// 이 구간 동안 적용할 머티리얼.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	TObjectPtr<UMaterialInterface> OverrideMaterial;
};
