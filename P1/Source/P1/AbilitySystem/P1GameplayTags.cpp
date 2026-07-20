// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1GameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_InputTag_Ability_BasicAttack, "InputTag.Ability.BasicAttack", "기본공격 입력 액션에 대응하는 입력 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_BasicAttack, "Ability.BasicAttack", "기본공격 어빌리티 식별 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cooldown_Ability_BasicAttack, "Cooldown.Ability.BasicAttack", "기본공격 스킬 아이콘 UI 전용 타이머 태그 — CooldownGameplayEffectClass가 아니므로 발동 차단에 관여하지 않음")
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

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Invulnerable, "State.Invulnerable", "무적 상태 — UP1AttributeSet이 Damage 처리 시 이 태그가 있으면 데미지를 무시(0으로 처리)한다")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Stunned, "State.Stunned", "기절 상태 — 베이스 어빌리티 ActivationBlockedTags에 포함되어 대부분의 어빌리티 발동을 막고, AP1PlayerController::HandleMove()가 직접 체크해 이동도 막는다")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_StunDuration, "Data.StunDuration", "기절 GE의 SetByCaller 지속시간 채널 — 소스마다 다른 기절 시간을 공유 GE 하나로 재사용")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Dead, "State.Dead", "사망 상태 GE가 부여하는 태그 — 전 어빌리티 ActivationBlockedTags에 포함, GE 자연 만료 시점이 리스폰 시점")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Character_Died, "Event.Character.Died", "AttributeSet이 Health<=0을 감지하면 보내는 이벤트 — 캐릭터가 받아 State.Dead GE를 자신에게 적용")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_RespawnDelay, "Data.RespawnDelay", "사망 GE의 Duration(리스폰 대기시간) SetByCaller 채널")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Montage_Death_Impact, "Event.Montage.Death.Impact", "DeathMontage의 착지 프레임 이벤트 — 이 시점에 래그돌로 전환")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Heal_Flat, "Data.Heal.Flat", "즉시 회복 GE의 SetByCaller 채널 — 계수 적용까지 끝난 최종 회복량")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Mana_Flat, "Data.Mana.Flat", "즉시 마나 회복 GE의 SetByCaller 채널")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Gold_Flat, "Data.Gold.Flat", "골드 보상 GE의 SetByCaller 채널")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Experience_Flat, "Data.Experience.Flat", "경험치 보상 GE의 SetByCaller 채널")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_KillStreak, "Data.KillStreak", "킬 보상 MMC가 읽는 피해자의 연속킬 수 — 현상금 보정에 사용")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_TimeSinceLastDeath, "Data.TimeSinceLastDeath", "킬 보상 MMC가 읽는 피해자의 마지막 사망 후 경과 시간(초) — 현상금 보정에 사용")

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

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_InputTag_Ability_R, "InputTag.Ability.R", "R 어빌리티 입력 태그 (Stone Forged Soul)")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_StoneForgedSoul, "Ability.StoneForgedSoul", "Stone Forged Soul 어빌리티 식별 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Montage_StoneForgedSoul_Crash, "Event.Montage.StoneForgedSoul.Crash", "착지 몽타주 프레임 이벤트 — 서버 범위 피해 판정 + 비행 모드 해제 트리거")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cooldown_Ability_StoneForgedSoul, "Cooldown.Ability.StoneForgedSoul", "Stone Forged Soul 쿨다운 GE가 부여하는 태그")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_InputTag_Ability_Passive, "InputTag.Ability.Passive", "패시브 스킬아이콘 UI 슬롯 식별용 — 실제 입력 액션에는 바인딩하지 않는다")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_StoicismDeflect, "Ability.StoicismDeflect", "Stoicism 패시브의 디플렉트 어빌리티 식별 태그 — OnGiveAbility에서 루즈 태그로도 부여되어 AttributeSet의 존재 판별에 쓰임")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cooldown_Ability_StoicismDeflect, "Cooldown.Ability.StoicismDeflect", "Stoicism 디플렉트 쿨다운 GE가 부여하는 태그 — 부재 시 디플렉트 사용 가능")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_StoicismDeflect_Consumed, "Event.StoicismDeflect.Consumed", "AttributeSet이 디플렉트 발동을 감지했을 때 보내는 이벤트 — 어빌리티가 자기 쿨다운을 커밋하도록 트리거")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_StoicismVitality, "Ability.StoicismVitality", "Stoicism 패시브의 재생+방어력 어빌리티 식별 태그")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_ArmorBonus_Flat, "Data.ArmorBonus.Flat", "방어력 보너스 GE의 SetByCaller 채널 — 잃은 체력 비례 증폭까지 반영한 최종값")
