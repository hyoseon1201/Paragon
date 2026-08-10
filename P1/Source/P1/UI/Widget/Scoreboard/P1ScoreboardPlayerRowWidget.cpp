// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Scoreboard/P1ScoreboardPlayerRowWidget.h"
#include "Components/TextBlock.h"
#include "Player/P1PlayerState.h"

namespace
{
	// 한글/영어 대문자는 폭 2, 영어 소문자/숫자(+그 외)는 폭 1로 쳐서 최대 MaxWidth까지만 자른다 —
	// "한글/대문자 8자" == "소문자/숫자 16자"와 같은 폭(16)이 되게 하는 요청이라, 문자 개수가 아니라
	// 이 폭 합계 기준으로 자르는 게 맞다. 한글 음절(가~힣, U+AC00~U+D7A3) 범위만 폭 2로 취급 —
	// 자모/특수 한글 블록까지는 스코어보드 닉네임에 실제로 안 쓰일 값이라 다루지 않는다.
	FString TruncateByDisplayWidth(const FString& Source, int32 MaxWidth)
	{
		FString Result;
		int32 CurrentWidth = 0;

		for (const TCHAR Ch : Source)
		{
			const bool bIsWide = (Ch >= TEXT('A') && Ch <= TEXT('Z')) || (Ch >= 0xAC00 && Ch <= 0xD7A3);
			const int32 CharWidth = bIsWide ? 2 : 1;

			if (CurrentWidth + CharWidth > MaxWidth)
			{
				break;
			}

			Result.AppendChar(Ch);
			CurrentWidth += CharWidth;
		}

		return Result;
	}
}

void UP1ScoreboardPlayerRowWidget::SetPlayerData(AP1PlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	if (NicknameText)
	{
		// 8(한글/대문자) == 16(소문자/숫자)와 같은 폭이 되도록 최대 폭 16 기준으로 자른다.
		NicknameText->SetText(FText::FromString(TruncateByDisplayWidth(PlayerState->GetPlayerName(), 16)));
	}

	if (CharacterText)
	{
		// PlayerState->GetPawn()을 거치지 않는다 — 그 역방향 링크는 소유 클라이언트 로컬에서만 채워지고
		// 다른 클라이언트에는 복제되지 않아서(자세한 배경은 AP1PlayerState::GetHeroDisplayName() 주석
		// 참고), 남의 정보를 봐야 하는 스코어보드에서 자기 자신 행만 뜨고 나머지는 비는 버그가 있었다.
		CharacterText->SetText(PlayerState->GetHeroDisplayName());
	}

	if (KDAText)
	{
		KDAText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d / %d"),
			PlayerState->GetKills(), PlayerState->GetDeaths(), PlayerState->GetAssists())));
	}
}
