// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_InputTag_Ability_BasicAttack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_BasicAttack)
// 기본공격은 CooldownGameplayEffectClass를 쓰지 않는다(콤보를 GAS 쿨다운으로 막으면 안 되므로) —
// 이 태그는 순수 UI용(스킬 아이콘의 "다음 공격까지 남은 시간" 표시)이며 발동 차단에는 전혀 관여하지 않는다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_BasicAttack)
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

// 무적 상태 — 전 어빌리티 공용(스테이시스/무적기 등). UP1AttributeSet이 Damage 처리 시 이 태그가 있으면 데미지를 무시한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Invulnerable)

// --- CC(기절) — 전 캐릭터/전 어빌리티 공용 ---
// 기절 GE(GE_Debuff_Stun 등)가 부여하는 태그. 베이스 UP1GameplayAbility의 ActivationBlockedTags에
// 포함되어 있어 기절 중엔 대부분의 어빌리티가 발동 차단된다(기절 해제용 스킬만 자기 생성자에서
// ActivationBlockedTags.RemoveTag로 예외 처리). 이동(WASD)은 어빌리티가 아니라 AP1PlayerController::
// HandleMove()가 직접 이 태그를 체크해서 막는다(Enhanced Input이라 ActivationBlockedTags를 안 거침).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Stunned)
// 기절 GE의 Duration SetByCaller 채널 — 소스마다 기절 시간이 다르므로(예: 거리 비례 스케일) 공유 GE
// 하나를 여러 어빌리티가 값만 다르게 넣어 재사용한다(Data.CooldownDuration과 동일한 패턴).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_StunDuration)

// --- 사망/리스폰 (전 캐릭터 공용) ---
// Health<=0 감지 시 UP1AttributeSet이 부여하는 Duration GE의 태그 — 전 어빌리티 공통 ActivationBlockedTags에도
// 걸려있어 사망 중엔 어떤 어빌리티도 발동 불가. GE 자체가 자연 만료되는 시점이 곧 리스폰 시점이다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead)
// AttributeSet이 Health<=0을 감지하면 보내는 GameplayEvent — AP1HeroCharacter가 이 이벤트를 받아
// State.Dead GE를 자신에게 적용한다(AttributeSet은 캐릭터 클래스를 몰라야 하므로 GE 적용 자체는 위임).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Character_Died)
// 사망 GE의 Duration(리스폰 대기시간) SetByCaller 채널.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_RespawnDelay)
// DeathMontage의 "착지/충격" 프레임에 UP1AnimNotify_SendGameplayEvent로 배치 — 이 시점에
// AP1HeroCharacter가 스켈레탈 메시를 물리 시뮬레이션(래그돌)으로 전환한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Montage_Death_Impact)

// 즉시 회복 GE의 SetByCaller 채널 — 최종 회복량(계수 적용까지 끝난 값)을 어빌리티가 C++에서 계산해 넣는다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Heal_Flat)
// 즉시 마나 회복 GE의 SetByCaller 채널 — Data.Heal.Flat과 동일한 패턴, Mana 대상.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Mana_Flat)

// --- 성장/보상 (레벨업, 킬/어시스트) ---
// 골드/경험치 보상 GE의 SetByCaller 채널 — 킬/어시스트 보상 지급 시 AttributeSet이 계산해 넣는다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Gold_Flat)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Experience_Flat)
// 킬 보상 GE 전용 — UP1MMC_GoldKillBounty가 이 두 채널을 SetByCaller로 읽어 현상금(연속킬/생존시간 보정)을 계산한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_KillStreak)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_TimeSinceLastDeath)

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

// --- R: Stone Forged Soul (스테이시스 상승 → 회복+슬로우 → 착지 범위 데미지 궁극기) ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_InputTag_Ability_R)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_StoneForgedSoul)
// 착지 몽타주 프레임 이벤트 — 서버 범위 피해 판정 + 비행 모드 해제 트리거 (RMB의 Land 이벤트와 동일한 패턴).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Montage_StoneForgedSoul_Crash)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_StoneForgedSoul)

// --- Passive: Stoicism (다음 기본공격 무효화 + 잃은 체력 비례 재생/방어력) ---
// 실제 입력 액션에는 바인딩하지 않는 "UI 슬롯 식별용" InputTag. 패시브는 입력으로 발동하지 않지만,
// UP1OverlayWidgetController::BroadcastAbilityInfo()가 어빌리티→스킬아이콘 매핑에 InputTag를 재사용하므로
// SkillIcon_Passive에 아이콘/쿨다운을 연결하려면 이 태그가 필요하다 — AbilityInputActions TMap에는 매핑하지 않는다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_InputTag_Ability_Passive)
// 어빌리티 자신의 Asset Tag를 OnGiveAbility에서 루즈 태그로도 ASC에 심어둔다 — AttributeSet이
// "이 캐릭터가 애초에 Stoicism 디플렉트를 갖고 있는지"를 어빌리티 클래스 참조 없이(태그만으로) 판별하기 위함.
// 실제 "지금 사용 가능한지"는 네이티브 쿨다운 태그(Cooldown.Ability.StoicismDeflect) 부재로 판단 — 새로 부여된
// 어빌리티는 쿨다운 태그가 없는 상태로 시작하므로 별도 "준비완료" 태그 없이 자연스럽게 "처음엔 사용 가능"이 성립한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_StoicismDeflect)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_StoicismDeflect)
// AttributeSet이 "디플렉트 발동됨"을 감지하면 이 이벤트를 보내 어빌리티가 자기 쿨다운을 커밋하게 한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_StoicismDeflect_Consumed)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_StoicismVitality)
// 방어력 보너스 GE의 SetByCaller 채널 — 잃은 체력 비례 증폭까지 반영한 최종값을 어빌리티가 계산해 넣는다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_ArmorBonus_Flat)
