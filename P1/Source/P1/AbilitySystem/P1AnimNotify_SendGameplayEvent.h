// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "P1AnimNotify_SendGameplayEvent.generated.h"

// 범용 GameplayEvent 발신 AnimNotify.
// 어빌리티마다 별도 Notify 클래스를 만들지 않고, 몽타주 에디터에서
// EventTag 프로퍼티만 변경해 모든 어빌리티 이벤트를 이 클래스 하나로 처리한다.
UCLASS(meta = (DisplayName = "Send Gameplay Event"))
class P1_API UP1AnimNotify_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	// 타임라인에 태그 이름을 표시해서 Notify 종류를 한눈에 구분할 수 있게 한다.
	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	FGameplayTag EventTag;
};
