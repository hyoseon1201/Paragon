// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_InputTag_Ability_BasicAttack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_BasicAttack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Attacking)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Montage_BasicAttackHit)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_DamageMultiplier)

// ExecCalc_Damage 데미지 계수 채널. 어빌리티는 자기가 쓰는 채널만 채운다.
// Raw = (Flat + PhysicalPower계수*Source.PhysicalPower + MagicalPower계수*Source.MagicalPower
//        + TargetMaxHealthPct계수*Target.MaxHealth + SourceMaxHealthPct계수*Source.MaxHealth) * DamageMultiplier
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Damage_Flat)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Damage_PhysicalPower)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Damage_MagicalPower)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Damage_TargetMaxHealthPct)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Damage_SourceMaxHealthPct)

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

// --- E: Sacred Oath (다음 기본공격 강화 버프) ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_InputTag_Ability_E)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_SacredOath)
// 버프 지속(기본 5초) 동안 부여. MeleeAttack이 이 태그로 "강화된 다음 공격"인지 판단하고, 발동 즉시 제거해 1회만 소모한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Buff_SacredOath_Active)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_SacredOath)
// 이동속도 감소 디버프 식별 태그 (Sacred Oath 강화 공격 등).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Debuff_MovementSlow)
// 범용 디버프 크기 채널 (0.0~1.0 비율). 슬로우%, 방어력 감소% 등 GE마다 재사용 — Data.CooldownDuration과 같은 패턴.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_DebuffMagnitude)

// --- Q: Make Way (지속 화염 회오리) ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_InputTag_Ability_Q)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_MakeWay)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_MakeWay)
// 방어력 감소 디버프 식별 태그 (Make Way 등, 중첩 적용).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Debuff_ArmorShred)
