// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "P1AnimInstance.generated.h"

class AP1CharacterBase;

UCLASS()
class P1_API UP1AnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE float GetSpeed() const { return Speed; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE bool IsMoving() const { return Speed > KINDA_SMALL_NUMBER; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE bool IsNotMoving() const { return !IsMoving(); }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE bool IsFalling() const { return bIsFalling; }

protected:
	// 로코모션 블렌드스페이스 Speed 축 입력값. 절대 속도(cm/s)가 아니라 현재 MaxWalkSpeed 대비 비율(0~100)로
	// 정규화한다 — MovementSpeed 어트리뷰트가 버프/디버프로 바뀌어도 블렌드스페이스 샘플 배치를 그대로 쓸 수 있다.
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Speed = 0.0f;

	// 로코모션 블렌드스페이스 Direction 축 입력값(-180~180). 액터 정면 기준 이동 방향각.
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling = false;

private:
	UPROPERTY()
	TObjectPtr<AP1CharacterBase> CachedCharacter;
};
