// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1HeroTypes.generated.h"

class AP1HeroCharacter;

// DT_Heroes의 Row(RowName=HeroId, 예: "Greystone"/"Dekker") — 픽 화면(클라)과 AP1ArenaGameMode(서버)
// 양쪽이 각자 독립된 UDataTable* 참조로 같은 에셋을 가리킨다(DT_ShopItems와 동일한 이중 참조 패턴).
// 표시용 이름/포트레이트는 여기 중복 저장하지 않고 HeroClass의 CDO(AP1HeroCharacter::GetHeroDisplayName/
// GetHeroPortrait)에서 그대로 읽는다. 서버 쪽에서는 이 테이블에 실제로 존재하는 HeroId만 유효한 값으로
// 인정되므로, 테이블 자체가 클라이언트가 보낸 URL 옵션에 대한 화이트리스트 역할을 겸한다.
USTRUCT(BlueprintType)
struct P1_API FP1HeroDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	FName HeroId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	TSubclassOf<AP1HeroCharacter> HeroClass;
};
