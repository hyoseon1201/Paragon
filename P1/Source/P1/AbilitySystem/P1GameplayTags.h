// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_InputTag_Ability_BasicAttack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_BasicAttack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Attacking)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Montage_BasicAttackHit)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_DamageMultiplier)

// ExecCalc_Damage 데미지 계수 채널. 어빌리티는 자기가 쓰는 채널만 채운다.
// Raw = (Flat + PhysicalPower계수 * Source.PhysicalPower) * DamageMultiplier
// 미래 확장: Data.Damage.MagicalPower, Data.Damage.TargetMaxHealthPct 등.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Damage_Flat)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Damage_PhysicalPower)

// 캐릭터 유형 — RMB 쿨감/버프 조건("영웅이나 보스 적중 시") 등에서 대상 유형 판별에 사용.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Type_Hero)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Type_Boss)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Type_Minion)

// --- RMB: Assault The Gates (지면 조준 도약 스킬) ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_InputTag_Ability_RMB)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_AssaultTheGates)
// 지면 조준 중 상태. 이 태그가 있으면 ASC 입력 라우팅이 LMB=확정, RMB=취소로 리라우팅하고 나머지 어빌리티는 차단.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_TargetingAbility)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Montage_AssaultTheGates_Land)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_AssaultTheGates)
// 이동속도 버프 식별 태그 (RMB 영웅/보스 적중 보상 등).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Buff_MovementSpeed)
// 쿨다운 GE의 SetByCaller 지속시간. 어빌리티가 값을 넣어 쿨다운 길이를 동적으로 지정(감소 재적용 등).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_CooldownDuration)
