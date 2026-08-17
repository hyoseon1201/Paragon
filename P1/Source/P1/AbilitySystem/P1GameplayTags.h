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

// 데미지 팝업 텍스트 색상 구분용 — ApplyDamageToTarget()이 유효 계수(클래스+Bonus)를 비교해 우세한
// 쪽 태그 하나를 CapturedSourceTags에 실어 보낸다(Ability.BasicAttack과 동일한 전달 경로).
// 어트리뷰트/방어력 계산 자체에는 관여하지 않는 순수 UI용 분류 — ExecCalc_Damage는 이 태그를 모른다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_DamageType_Physical)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_DamageType_Magical)
// **위 둘과 달리 UI 전용이 아니다** — `UP1DamageGameplayAbility::bIsTrueDamage=true`인 어빌리티가
// Physical/Magical 대신 이 태그를 붙이면(둘 중 하나만 배타적으로 부여됨), `P1ExecCalc_Damage`가 방어력
// 감산(Armor/(Armor+100))을 완전히 건너뛴다 — 관통(Penetration)으로 무효화되는 게 아니라 애초에 방어력
// 계산 자체가 없는 별개 축(단, DamageReduction 같은 % 감쇄 효과는 그대로 적용됨 — "고정 피해"는 방어력만
// 무시하지 전체 방어 시스템을 무시하지 않는 일반적인 MOBA 컨벤션). 증강(아이템) "진실의 일격"이 첫 사용처.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_DamageType_True)

// 캐릭터 유형 — RMB 쿨감/버프 조건("영웅 적중 시") 등에서 대상 유형 판별에 사용.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Type_Hero)
// 정글 몬스터(중립 캠프) — TeamId를 기본값(255=NoTeam)으로 두면 IsSameTeam()이 항상 false를 반환해
// 별도 팀 설정 없이도 모든 영웅에게 자동으로 적대(공격 가능)가 된다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Type_Monster)

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

// "이동 불가"류 CC(스턴/에어본/속박 등) 전체를 아우르는 범용 카테고리 태그 — 구체적인 CC 태그
// (State.Stunned 등)와 별개로, 그 CC GE의 Granted Tags에 이 태그도 함께 추가해두면 "이동 불가에
// 걸리면 반응"하는 아이템/스킬(예: 사면(아이템) "용기")이 CC 종류를 하나하나 몰라도 이 태그 하나만
// 구독해서 전부 감지할 수 있다. 지금은 스턴 GE만 이 태그를 부여하지만, 나중에 에어본/속박이 추가돼도
// 그 GE의 Granted Tags에 이 태그만 얹으면 되고 아이템/스킬 쪽 코드는 전혀 안 바뀐다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Immobilized)

// 캐스팅/채널링 중 이동을 막고 싶은 어빌리티가 자기 ActivationOwnedTags에 추가하는 범용 태그(전 어빌리티
// 공용, State.Stunned와 동일한 패턴) — AP1PlayerController::HandleMove()가 이 태그도 함께 체크한다.
// 어빌리티가 활성 중인 동안만 자동으로 부여/해제되므로(GAS 표준 동작) 별도 부여/해제 코드가 필요 없다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Rooted)

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

// --- 피격 리액션 (전 캐릭터 공용) ---
// AttributeSet이 데미지를 받고도 생존(Health>0)했음을 감지하면 보내는 GameplayEvent — 이 이벤트 자체는
// 서버에서만 발화된다(Death와 동일한 이유). AP1HeroCharacter가 받아 HitReactEffectClass(매우 짧은
// Duration GE)를 자신에게 적용 — 그 GE가 부여하는 State.HitReacting 태그가 실제로 리플리케이트되므로,
// 원격 클라이언트를 포함한 전 클라이언트가 태그 변경 이벤트로 몽타주를 재생할 수 있다(순수 GameplayEvent를
// 직접 재생 트리거로 쓰면 서버/호스트에서만 보이고 다른 플레이어 화면엔 안 보임 — 코스메틱 리플리케이션 컨벤션 참고).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Character_HitReact)
// HitReactEffectClass가 부여하는 신호용 태그 — 값 자체엔 의미 없고, 카운트가 0→양수로 바뀌는 순간만
// 감지해 리액션 몽타주를 재생한다(제거 시점엔 아무것도 안 함, 몽타주는 짧은 원샷이라 스스로 끝남).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_HitReacting)

// AP1HeroCharacter가 State.Immobilized 태그 카운트 0→양수 전이(서버)를 감지하면 자신의 ASC로 보내는
// 이벤트 — "이동 불가에 걸리면 반응"하는 아이템 어빌리티(사면(아이템) "용기"가 첫 사용처)가
// AbilityTriggers로 구독한다. Event.Character.BasicAttackHitDealt와 동일한 범용 디스패치 원칙 —
// 발신자(캐릭터 클래스)는 어떤 아이템이 반응하는지 전혀 모른다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Character_Immobilized)

// 사면(아이템) 고유효과 "용기"(UP1GameplayAbility_Item_Bravery)의 쿨다운 태그 — GE_Absolution_Cooldown의
// Granted Tags에 이 태그를 지정해야 한다(다른 어빌리티들의 Cooldown.Ability.X와 동일 컨벤션).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_Bravery)

// 증강(아이템) 고유효과 "진실의 일격"(UP1GameplayAbility_Item_TrueStrike) — Q/E/RMB/R 중 하나를 사용하면
// 이 태그가 부여되는 4초짜리 버프가 걸리고("다음 기본공격 강화" 대기 상태), 그 안에 기본공격이 적중하면
// 소모되며 고정 피해를 추가로 입힌다. GE_Item_TrueStrike_EmpowerBuff의 Granted Tags에 지정.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Buff_TrueStrike_Empowered)
// "진실의 일격" 자체의 쿨다운(1.5초, 스킬 사용→버프 부여 사이의 재발동 간격) — GE_Item_TrueStrike_Cooldown의
// Granted Tags에 지정.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_TrueStrike)

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
// 지면 조준 중 상태. 이 태그가 있으면 ASC 입력 라우팅이 LMB=확정, InputTag.Cancel(F)=취소로 리라우팅하고
// 나머지 어빌리티는 차단. 취소를 "같은 스킬키 재입력"이 아니라 전용 키로 분리한 이유 — 홀드해서 조준하고
// 키를 떼야 발사하는 스킬은 "떼기"가 이미 확정 동작이라 같은 키로 취소를 표현할 방법이 없기 때문(상세
// 배경은 UP1AbilitySystemComponent::AbilityInputTagPressed 주석 참고).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_TargetingAbility)
// 지면조준/타게팅 어빌리티 전용 취소 입력 태그 — 실제 입력 액션(F 키 등)은 AbilityInputActions TMap에서
// 이 태그와 매핑한다. 어떤 스킬 키로 조준을 시작했든(RMB/Q/E 등) 항상 이 하나로 취소한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_InputTag_Ability_Cancel)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Montage_AssaultTheGates_Land)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_AssaultTheGates)
// 이동속도 버프 식별 태그 (RMB 영웅 적중 보상 등).
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

// --- Dekker Passive: Rocket Boots (공중에서 점프 입력 시 상승 추진, 다른 스킬 사용 시 쿨다운 감소) ---
// InputTag.Ability.Passive를 그대로 재사용(위 Stoicism 섹션 참고, UI 슬롯 공유 목적) — Dekker는 이
// 태그를 가진 유일한 어빌리티가 Rocket Boots라, AP1PlayerController가 공중 점프 입력 시
// ASC->AbilityInputTagPressed(InputTag.Ability.Passive)를 호출하는 것이 실제 발동 트리거로도 그대로
// 작동한다(다른 영웅의 패시브는 이 태그를 InputTag로 설정하지 않으므로 충돌 없음).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_RocketBoots)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_RocketBoots)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_StoicismVitality)
// 방어력 보너스 GE의 SetByCaller 채널 — 잃은 체력 비례 증폭까지 반영한 최종값을 어빌리티가 계산해 넣는다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_ArmorBonus_Flat)

// --- Dekker Q: Photon Disruptor (관통 드론 빔) ---
// InputTag.Ability.Q는 그레이스톤의 Make Way와 공유(영웅마다 이 슬롯엔 자신의 어빌리티 하나만 등록,
// 같은 입력 태그를 두 어빌리티가 동시에 두고 경쟁하지 않음 — MeleeAttack/RangedAttack과 동일한 패턴).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_PhotonDisruptor)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_PhotonDisruptor)

// --- Dekker RMB: Stasis Bomb (홀드 조준 → 튕기는 폭탄) ---
// InputTag.Ability.RMB는 그레이스톤의 Assault The Gates와 공유(위와 동일한 이유 — 영웅당 이 슬롯에
// 자신의 어빌리티 하나만 등록). Assault The Gates와 달리 지면 장판이 아니라 "홀드 조준 → 릴리즈 발사"
// 방식이라, State.TargetingAbility 태그는 그대로 재사용하되 확정 트리거가 LMB가 아니라 조준을 시작한
// 스킬 자신(RMB)의 release다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_StasisBomb)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_StasisBomb)

// --- Dekker E: Containment Fence (지면 장판 조준 → 원형 이동 차단 장판) ---
// InputTag.Ability.E는 그레이스톤의 Sacred Oath와 공유(위와 동일한 이유 — 영웅당 이 슬롯에 자신의
// 어빌리티 하나만 등록). 데미지가 없는 순수 CC라 별도 데미지 SetByCaller 채널은 필요 없다 — 이동 차단
// 자체는 GE/태그가 아니라 AP1ContainmentFence(스폰된 액터)가 매 틱 직접 위치를 clamp하는 방식으로 구현.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_ContainmentFence)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_ContainmentFence)

// --- Dekker R: Ion Strike (지면 장판 조준 → 짧은 딜레이 후 원형 폭발, 이중 반경 CC) ---
// InputTag.Ability.R는 그레이스톤의 Stone Forged Soul과 공유(위와 동일한 이유).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_IonStrike)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Ability_IonStrike)

// --- 정글 몬스터 AI(BT 기반 리시 FSM) ---
// AIController가 BT 태스크에서 이 이벤트로 근접 공격 어빌리티를 트리거한다 — 실제 조준/판정은
// 어빌리티가 UP1DamageGameplayAbility::GetEnemiesInRadius로 직접 재조회(EventData에 타겟을 안 실음).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Monster_MeleeAttack)

// --- 온-히트 아이템 어빌리티 범용 디스패치 ---
// UP1AttributeSet::PostGameplayEffectExecute가 기본 공격 데미지 적용을 감지하면 가해자(공격자)의 ASC에
// 보내는 이벤트 — 페이로드는 EventData.Target(피격자 액터)+EventData.EventMagnitude(1.0=크리티컬,
// 0.0=평타). 아이템 구매 시 부여되는 각 아이템 전용 어빌리티(UP1GameplayAbility_OnHitItemAbility 파생)가
// AbilityTriggers로 이 이벤트 하나를 공통 구독한다 — AttributeSet은 어떤 아이템이 존재하는지, 뭘 하는지
// 전혀 몰라도 되고(어빌리티 클래스 참조 금지 원칙 준수), 새 온-히트 아이템이 추가돼도 AttributeSet
// 쪽 코드는 전혀 안 바뀐다(스토이시즘 디플렉트가 확립한 "감지는 AttributeSet, 반응은 어빌리티" 분리를
// 아이템 전체로 일반화한 것 — 원래 애쉬브링어/더스트 데빌을 AttributeSet 전용 메서드로 만들었다가
// 클린코드 관점에서 아이템 수만큼 AttributeSet이 계속 커지는 문제가 지적돼 2026-08-17에 리팩터링함).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Character_BasicAttackHitDealt)
