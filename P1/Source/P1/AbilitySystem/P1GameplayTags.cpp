// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1GameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_InputTag_Ability_BasicAttack, "InputTag.Ability.BasicAttack", "기본공격 입력 액션에 대응하는 입력 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_BasicAttack, "Ability.BasicAttack", "기본공격 어빌리티 식별 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Attacking, "State.Attacking", "기본공격 모션 재생 중 상태 — 재입력 차단 등에 사용")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Montage_BasicAttackHit, "Event.Montage.BasicAttackHit", "기본공격 몽타주의 타격 프레임에서 발생하는 게임플레이 이벤트")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_DamageMultiplier, "Data.DamageMultiplier", "GE 스펙에 SetByCaller로 전달되는 데미지 배율 — ExecCalc에서 읽어 클리브/스킬 계수에 사용")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Damage_Flat, "Data.Damage.Flat", "스탯 무관 고정 데미지 채널. ExecCalc가 Raw에 그대로 더한다")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Damage_PhysicalPower, "Data.Damage.PhysicalPower", "PhysicalPower 계수 채널. ExecCalc가 Source.PhysicalPower와 곱해 Raw에 더한다")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Damage_MagicalPower, "Data.Damage.MagicalPower", "MagicalPower 계수 채널. ExecCalc가 Source.MagicalPower와 곱해 Raw에 더한다")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Damage_TargetMaxHealthPct, "Data.Damage.TargetMaxHealthPct", "Target MaxHealth 계수 채널. ExecCalc가 Target.MaxHealth와 곱해 Raw에 더한다")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Damage_SourceMaxHealthPct, "Data.Damage.SourceMaxHealthPct", "시전자(Source) MaxHealth 계수 채널. ExecCalc가 Source.MaxHealth와 곱해 Raw에 더한다")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Character_Type_Hero, "Character.Type.Hero", "플레이어가 조종하는 영웅 캐릭터")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Character_Type_Boss, "Character.Type.Boss", "보스 몬스터")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Character_Type_Minion, "Character.Type.Minion", "일반 미니언/졸개")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_InputTag_Ability_RMB, "InputTag.Ability.RMB", "우클릭 어빌리티 입력 태그 (Assault The Gates)")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_AssaultTheGates, "Ability.AssaultTheGates", "Assault The Gates 어빌리티 식별 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_TargetingAbility, "State.TargetingAbility", "지면 조준 중 상태 — ASC가 LMB=확정/RMB=취소로 리라우팅하고 나머지 어빌리티 입력 차단")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Montage_AssaultTheGates_Land, "Event.Montage.AssaultTheGates.Land", "도약 몽타주의 착지 프레임 이벤트 — 서버 범위 피해 판정 트리거")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cooldown_Ability_AssaultTheGates, "Cooldown.Ability.AssaultTheGates", "Assault The Gates 쿨다운 GE가 부여하는 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Buff_MovementSpeed, "Buff.MovementSpeed", "이동속도 증가 버프 식별 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_CooldownDuration, "Data.CooldownDuration", "쿨다운 GE의 SetByCaller 지속시간 채널 — 어빌리티가 동적으로 쿨다운 길이 지정")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_InputTag_Ability_E, "InputTag.Ability.E", "E 어빌리티 입력 태그 (Sacred Oath)")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_SacredOath, "Ability.SacredOath", "Sacred Oath 어빌리티 식별 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Buff_SacredOath_Active, "Buff.SacredOath.Active", "Sacred Oath 버프 활성 상태 — MeleeAttack이 체크 후 발동 시 제거해 다음 1회만 강화")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cooldown_Ability_SacredOath, "Cooldown.Ability.SacredOath", "Sacred Oath 쿨다운 GE가 부여하는 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Debuff_MovementSlow, "Debuff.MovementSlow", "이동속도 감소 디버프 식별 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_DebuffMagnitude, "Data.DebuffMagnitude", "범용 디버프 크기(0~1 비율) SetByCaller 채널 — 슬로우/방어감소 등 여러 디버프 GE가 공유")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_InputTag_Ability_Q, "InputTag.Ability.Q", "Q 어빌리티 입력 태그 (Make Way)")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_MakeWay, "Ability.MakeWay", "Make Way 어빌리티 식별 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cooldown_Ability_MakeWay, "Cooldown.Ability.MakeWay", "Make Way 쿨다운 GE가 부여하는 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Debuff_ArmorShred, "Debuff.ArmorShred", "물리 방어력 감소 디버프 식별 태그 (중첩 가능)")
