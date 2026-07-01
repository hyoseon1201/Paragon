// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1GameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_InputTag_Ability_BasicAttack, "InputTag.Ability.BasicAttack", "기본공격 입력 액션에 대응하는 입력 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_BasicAttack, "Ability.BasicAttack", "기본공격 어빌리티 식별 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Attacking, "State.Attacking", "기본공격 모션 재생 중 상태 — 재입력 차단 등에 사용")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Montage_BasicAttackHit, "Event.Montage.BasicAttackHit", "기본공격 몽타주의 타격 프레임에서 발생하는 게임플레이 이벤트")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_DamageMultiplier, "Data.DamageMultiplier", "GE 스펙에 SetByCaller로 전달되는 데미지 배율 — ExecCalc에서 읽어 클리브/스킬 계수에 사용")
