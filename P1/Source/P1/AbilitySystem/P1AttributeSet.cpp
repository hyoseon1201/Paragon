// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"
#include "Player/P1PlayerState.h"
#include "Player/P1PlayerController.h"
#include "Characters/P1HeroCharacter.h"
#include "Characters/P1JungleMonsterCharacter.h"
#include "GameModes/P1ArenaGameMode.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "P1.h"

UP1AttributeSet::UP1AttributeSet()
{
	InitHealth(680.0f);
	InitMaxHealth(680.0f);
	InitMana(280.0f);
	InitMaxMana(280.0f);
	InitHealthRegen(1.7f);
	InitManaRegen(1.2f);

	InitPhysicalPower(68.0f);
	InitMagicalPower(0.0f);
	// 퍼센트 스케일(100=기준/100%) — P1GameplayAbility_MeleeAttack/RangedAttack의 PlayRate 계산이
	// AttackSpeed/100.0f로 직접 나눠 쓰기 때문에 여기도 그 스케일을 따라야 한다(다른 대부분의 %
	// 스탯처럼 1.0=100%인 분수 스케일이 아님 — 상점 UI 표시할 때 이 차이를 반드시 반영할 것,
	// UP1ShopStatsWidget::BindStat()의 bAlreadyPercent 참고).
	InitAttackSpeed(100.0f);
	InitBasicAttackTime(1.1f);
	InitAttackRange(275.0f);
	InitCleave(0.2f);
	InitPhysicalArmor(36.0f);
	InitMagicalArmor(30.0f);
	InitPhysicalPenetration(0.0f);
	InitMagicalPenetration(0.0f);
	InitLifeSteal(0.0f);
	InitTenacity(0.0f);
	InitDamageReduction(0.0f);
	InitAbilityHaste(0.0f);
	InitUltimateHaste(0.0f);
	InitUltimateDamagePercent(0.0f);
	InitCriticalChance(0.0f);
	InitCriticalDamage(1.5f);

	InitMovementSpeed(720.0f);

	InitGold(3000.0f);
	InitExperience(0.0f);
}

void UP1AttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// GAS 자체(FGameplayAttribute::SetNumericValueChecked 등, GAMEPLAYATTRIBUTE_* 매크로 경로)가 어트리뷰트
	// 값을 바꿀 때마다 이미 내부적으로 MARK_PROPERTY_DIRTY를 호출해준다 — 여기서는 등록만 push-based로
	// 바꾸면 되고, 우리 쪽에서 별도로 마킹 코드를 추가할 필요가 없다.
	FDoRepLifetimeParams SharedParams;
	SharedParams.Condition = COND_None;
	SharedParams.RepNotifyCondition = REPNOTIFY_Always;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, Health, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, MaxHealth, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, Mana, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, MaxMana, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, HealthRegen, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, ManaRegen, SharedParams);

	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, PhysicalPower, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, MagicalPower, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, AttackSpeed, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, BasicAttackTime, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, AttackRange, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, Cleave, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, PhysicalArmor, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, MagicalArmor, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, PhysicalPenetration, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, MagicalPenetration, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, LifeSteal, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, Tenacity, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, DamageReduction, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, AbilityHaste, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, UltimateHaste, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, UltimateDamagePercent, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, CriticalChance, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, CriticalDamage, SharedParams);

	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, MovementSpeed, SharedParams);

	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, Gold, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UP1AttributeSet, Experience, SharedParams);
}

void UP1AttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetMovementSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetGoldAttribute() || Attribute == GetExperienceAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetUltimateHasteAttribute() || Attribute == GetUltimateDamagePercentAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetDamageReductionAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
	}
	else if (Attribute == GetCriticalChanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
	}
	else if (Attribute == GetCriticalDamageAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}

void UP1AttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UP1AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& Attr = Data.EvaluatedData.Attribute;
	const float Magnitude = Data.EvaluatedData.Magnitude;

	if (Attr == GetDamageAttribute())
	{
		// 메타 Damage → Health 변환. ExecCalc가 방어/관통까지 반영한 최종값을 누적해둔다.
		const float LocalDamage = GetDamage();
		SetDamage(0.0f);

		// 메타 CriticalHitFlag → 이번 히트가 크리티컬이었는지(Damage와 동일한 "즉시 읽고 리셋" 패턴).
		// ExecCalc 내부 RNG 결과라 스펙 생성 시점엔 알 수 없어 실행 결과로만 전달 가능 — 더스트 데빌의
		// "매너스"처럼 크리티컬 적중에 반응해야 하는 온-히트 아이템이 이 값을 읽는다.
		const bool bWasCritical = GetCriticalHitFlag() > 0.5f;
		SetCriticalHitFlag(0.0f);

		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();

		// 무적 상태(State.Invulnerable)면 데미지를 완전히 무시 — 스테이시스류 스킬이 공용으로 사용.
		const bool bInvulnerable = ASC && ASC->HasMatchingGameplayTag(TAG_State_Invulnerable);

		// Stoicism 디플렉트: 이 데미지가 기본공격에서 왔고(발생시킨 어빌리티의 Asset Tag가 스펙에 실려있음,
		// UP1DamageGameplayAbility::ApplyDamageToTarget 참고), 이 캐릭터가 디플렉트를 갖고 있으며
		// (루즈 태그로 존재 여부 판별) 쿨다운 중이 아니면(네이티브 쿨다운 태그 부재=사용 가능) 무효화.
		// 실제 쿨다운 커밋은 어빌리티에게 이벤트로 위임 — AttributeSet은 어빌리티 클래스를 몰라도 된다.
		bool bDeflected = false;
		if (ASC && !bInvulnerable)
		{
			const FGameplayTagContainer* SourceTags = Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
			const bool bFromBasicAttack = SourceTags && SourceTags->HasTag(TAG_Ability_BasicAttack);
			const bool bHasStoicismDeflect = ASC->HasMatchingGameplayTag(TAG_Ability_StoicismDeflect);
			const bool bDeflectOnCooldown = ASC->HasMatchingGameplayTag(TAG_Cooldown_Ability_StoicismDeflect);
			if (bFromBasicAttack && bHasStoicismDeflect && !bDeflectOnCooldown)
			{
				bDeflected = true;
				UE_LOG(LogP1, Log, TEXT("[AttributeSet] Stoicism 디플렉트 발동 — 기본공격 %.2f 무효화 (Owner=%s)"),
					LocalDamage, GetOwningActor() ? *GetOwningActor()->GetName() : TEXT("null"));

				FGameplayEventData EventData;
				EventData.EventTag = TAG_Event_StoicismDeflect_Consumed;
				ASC->HandleGameplayEvent(TAG_Event_StoicismDeflect_Consumed, &EventData);
			}
		}

		if (LocalDamage > 0.0f && !bInvulnerable && !bDeflected)
		{
			const float NewHealth = FMath::Clamp(GetHealth() - LocalDamage, 0.0f, GetMaxHealth());
			UE_LOG(LogP1, Log, TEXT("[AttributeSet] Damage %.2f applied: Health %.2f → %.2f"),
				LocalDamage, GetHealth(), NewHealth);
			SetHealth(NewHealth);
			NotifyDamageDealt(Data, LocalDamage);

			// 어시스트 판정용 — 실제로 관통된 데미지만 "기여"로 기록(무적/디플렉트는 애초에 이 분기 진입 안 함).
			RecordDamageContribution(Data);

			// 온-히트 아이템 어빌리티 범용 디스패치 — 생존/사망 여부와 무관하게 적중 자체로 발동(막타여도 발동).
			DispatchBasicAttackHitDealt(Data, bWasCritical);

			// 사망 감지 — State.Dead가 이미 있으면(중복 판정 등) 재발신하지 않는다.
			// GE 적용 자체는 캐릭터 클래스에 위임한다(AttributeSet은 캐릭터 타입을 몰라야 함).
			if (NewHealth <= 0.0f && ASC && !ASC->HasMatchingGameplayTag(TAG_State_Dead))
			{
				UE_LOG(LogP1, Log, TEXT("[AttributeSet] Health<=0 감지 — Event.Character.Died 발신 (Owner=%s)"),
					GetOwningActor() ? *GetOwningActor()->GetName() : TEXT("null"));

				// 킬/어시스트 보상은 사망 이벤트 발신 전에 처리 — HandleKillRewards가 피해자의
				// 죽기 직전 KillStreak/생존시간을 먼저 캡처한 뒤 Deaths/KillStreak를 리셋하기 때문에 순서가 중요.
				HandleKillRewards(Data);

				FGameplayEventData DeathEventData;
				DeathEventData.EventTag = TAG_Event_Character_Died;
				ASC->HandleGameplayEvent(TAG_Event_Character_Died, &DeathEventData);
			}
			else if (ASC)
			{
				// 데미지를 받았지만 생존 — 피격 리액션 신호를 보낸다. GE 적용 자체는 캐릭터 클래스에 위임.
				// Instigator를 실어 보내 정글 몬스터 AI가 "누가 나를 때렸는지"를 알 수 있게 한다(어그로 감지) —
				// 히어로가 가한 데미지는 ApplyDamageToTarget이 SourceASC->MakeEffectContext()로 컨텍스트를
				// 만들었으므로 여기서 GetInstigator()는 공격자의 AP1PlayerState를 반환한다(Pawn이 아님,
				// ASC OwnerActor 컨벤션 — AP1CharacterBase.h 주석 참고).
				FGameplayEventData HitReactEventData;
				HitReactEventData.EventTag = TAG_Event_Character_HitReact;
				HitReactEventData.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
				ASC->HandleGameplayEvent(TAG_Event_Character_HitReact, &HitReactEventData);
			}
		}
	}
	else if (Attr == GetHealthAttribute())
	{
		const float OldHealth = GetHealth();
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Attr == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
	}
	else if (Attr == GetExperienceAttribute())
	{
		UE_LOG(LogP1, Log, TEXT("[AttributeSet] Experience 획득 → 현재 %.0f (Owner=%s)"),
			GetExperience(), GetOwningActor() ? *GetOwningActor()->GetName() : TEXT("null"));

		// 레벨업 임계치 초과 여부는 캐릭터 클래스에 위임(AttributeSet은 레벨 커브 테이블을 몰라도 됨) —
		// 어빌리티 부여 로직과 마찬가지로 실제 레벨업 처리는 AP1HeroCharacter::CheckLevelUp()이 담당.
		// GetOwningActor()는 ASC의 OwnerActor(=PlayerState)를 반환하므로, 실제 Pawn은 PlayerState::GetPawn()으로 얻는다.
		if (const AP1PlayerState* PS = Cast<AP1PlayerState>(GetOwningActor()))
		{
			if (AP1HeroCharacter* Hero = Cast<AP1HeroCharacter>(PS->GetPawn()))
			{
				Hero->CheckLevelUp();
			}
		}
	}
}

void UP1AttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, Health, OldValue);
}

void UP1AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MaxHealth, OldValue);
}

void UP1AttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, Mana, OldValue);
}

void UP1AttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MaxMana, OldValue);
}

void UP1AttributeSet::OnRep_HealthRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, HealthRegen, OldValue);
}

void UP1AttributeSet::OnRep_ManaRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, ManaRegen, OldValue);
}

void UP1AttributeSet::OnRep_PhysicalPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, PhysicalPower, OldValue);
}

void UP1AttributeSet::OnRep_MagicalPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MagicalPower, OldValue);
}

void UP1AttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, AttackSpeed, OldValue);
}

void UP1AttributeSet::OnRep_BasicAttackTime(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, BasicAttackTime, OldValue);
}

void UP1AttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, AttackRange, OldValue);
}

void UP1AttributeSet::OnRep_Cleave(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, Cleave, OldValue);
}

void UP1AttributeSet::OnRep_PhysicalArmor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, PhysicalArmor, OldValue);
}

void UP1AttributeSet::OnRep_MagicalArmor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MagicalArmor, OldValue);
}

void UP1AttributeSet::OnRep_PhysicalPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, PhysicalPenetration, OldValue);
}

void UP1AttributeSet::OnRep_MagicalPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MagicalPenetration, OldValue);
}

void UP1AttributeSet::OnRep_LifeSteal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, LifeSteal, OldValue);
}

void UP1AttributeSet::OnRep_Tenacity(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, Tenacity, OldValue);
}

void UP1AttributeSet::OnRep_DamageReduction(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, DamageReduction, OldValue);
}

void UP1AttributeSet::OnRep_AbilityHaste(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, AbilityHaste, OldValue);
}

void UP1AttributeSet::OnRep_UltimateHaste(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, UltimateHaste, OldValue);
}

void UP1AttributeSet::OnRep_UltimateDamagePercent(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, UltimateDamagePercent, OldValue);
}

void UP1AttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, CriticalChance, OldValue);
}

void UP1AttributeSet::OnRep_CriticalDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, CriticalDamage, OldValue);
}

void UP1AttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, MovementSpeed, OldValue);
}

void UP1AttributeSet::OnRep_Gold(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, Gold, OldValue);
}

void UP1AttributeSet::OnRep_Experience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UP1AttributeSet, Experience, OldValue);
}

void UP1AttributeSet::NotifyDamageDealt(const FGameplayEffectModCallbackData& Data, float DamageAmount)
{
	AP1PlayerState* InstigatorPS = Cast<AP1PlayerState>(Data.EffectSpec.GetEffectContext().GetInstigator());
	if (!InstigatorPS)
	{
		return;
	}

	// PlayerState의 Owner는 AController::InitPlayerState()가 설정 — 표준 UE 소유권 규약.
	AP1PlayerController* InstigatorPC = Cast<AP1PlayerController>(InstigatorPS->GetOwner());
	if (!InstigatorPC)
	{
		return;
	}

	// GetOwningActor()는 히어로면 AP1PlayerState를 반환한다(Pawn이 아님 — HandleKillRewards의
	// VictimPS->GetPawn() 패턴과 동일한 이유. PlayerState는 실제 위치를 추적하지 않아 그대로 쓰면
	// 데미지 숫자가 캐릭터와 무관한 엉뚱한 곳에서 스폰된다). 반면 정글 몬스터는 ASC/AttributeSet을
	// PlayerState가 아니라 Pawn 자신이 직접 들고 있어서(AP1JungleMonsterCharacter 설계) GetOwningActor()가
	// 이미 Pawn 자신이다 — 몬스터를 때렸을 때 이 Cast<AP1PlayerState>가 항상 실패해 TargetPawn이 null이
	// 되고 데미지 숫자가 월드 원점(0,0,0)에서 스폰되던 버그의 원인이었다. AP1JungleMonsterCharacter::
	// OnHitReactEventReceived()의 Instigator 이중 해석과 동일한 패턴으로 양쪽 다 대응한다.
	const APawn* TargetPawn = nullptr;
	if (const AP1PlayerState* TargetPS = Cast<AP1PlayerState>(GetOwningActor()))
	{
		TargetPawn = TargetPS->GetPawn();
	}
	else
	{
		TargetPawn = Cast<APawn>(GetOwningActor());
	}

	// ApplyDamageToTarget()이 실어 보낸 색상 구분 태그 — UP1DamageGameplayAbility::ApplyDamageToTarget() 참고.
	const FGameplayTagContainer* SourceTags = Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
	const bool bIsMagicalDamage = SourceTags && SourceTags->HasTag(TAG_Data_DamageType_Magical);

	InstigatorPC->ClientShowDamageNumber(TargetPawn ? TargetPawn->GetActorLocation() : FVector::ZeroVector, DamageAmount, bIsMagicalDamage);
}

void UP1AttributeSet::RecordDamageContribution(const FGameplayEffectModCallbackData& Data)
{
	AP1PlayerState* InstigatorPS = Cast<AP1PlayerState>(Data.EffectSpec.GetEffectContext().GetInstigator());
	if (!InstigatorPS)
	{
		return;
	}

	const UWorld* World = GetWorld();
	RecentDamageContributors.Add(InstigatorPS, World ? World->GetTimeSeconds() : 0.0f);
}

void UP1AttributeSet::DispatchBasicAttackHitDealt(const FGameplayEffectModCallbackData& Data, bool bWasCritical)
{
	const FGameplayTagContainer* SourceTags = Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
	if (!SourceTags || !SourceTags->HasTag(TAG_Ability_BasicAttack))
	{
		return;
	}

	AP1PlayerState* InstigatorPS = Cast<AP1PlayerState>(Data.EffectSpec.GetEffectContext().GetInstigator());
	UAbilitySystemComponent* InstigatorASC = InstigatorPS ? InstigatorPS->GetAbilitySystemComponent() : nullptr;
	if (!InstigatorASC)
	{
		return;
	}

	// 어떤 아이템이 이 이벤트를 구독하는지 여기서는 전혀 모른다 — 그건 각 온-히트 아이템 어빌리티
	// (구매 시 GiveAbility로 부여됨)의 AbilityTriggers 몫이다. 이 함수는 "때렸다"는 사실과
	// 피격자/크리티컬 여부만 실어 보낸다.
	FGameplayEventData EventData;
	EventData.EventTag = TAG_Event_Character_BasicAttackHitDealt;
	EventData.Target = GetOwningActor();
	EventData.EventMagnitude = bWasCritical ? 1.0f : 0.0f;
	InstigatorASC->HandleGameplayEvent(TAG_Event_Character_BasicAttackHitDealt, &EventData);
}

void UP1AttributeSet::HandleKillRewards(const FGameplayEffectModCallbackData& Data)
{
	constexpr float AssistWindowSeconds = 10.0f;

	UAbilitySystemComponent* VictimASC = GetOwningAbilitySystemComponent();
	AActor* VictimOwner = VictimASC ? VictimASC->GetOwnerActor() : nullptr;
	AP1PlayerState* VictimPS = Cast<AP1PlayerState>(VictimOwner);
	if (!VictimPS)
	{
		// 정글 몬스터는 ASC를 PlayerState가 아니라 Pawn 자신이 들고 있어 여기로 빠진다(팀 킬스코어/KDA는
		// 원래도 안 섞여야 하는 게 맞지만, 골드/경험치까지 같이 누락되고 있었던 게 실제 버그였음).
		HandleMonsterKillRewards(Data, VictimOwner);
		return;
	}

	AP1PlayerState* KillerPS = Cast<AP1PlayerState>(Data.EffectSpec.GetEffectContext().GetInstigator());
	const bool bValidKiller = KillerPS && KillerPS != VictimPS;

	// 죽기 직전 피해자의 연속킬/생존시간을 먼저 캡처(현상금 계산용) — AddDeath()가 곧 이 값들을 리셋한다.
	const int32 VictimKillStreak = VictimPS->GetKillStreak();
	const int32 VictimLevel = VictimPS->GetCharacterLevel();
	const UWorld* World = VictimPS->GetWorld();
	const float VictimTimeSinceLastDeath = World ? (World->GetTimeSeconds() - VictimPS->GetLastDeathTime()) : 0.0f;

	UE_LOG(LogP1, Log, TEXT("[AttributeSet][Kill] 피해자=%s 킬러=%s(유효=%d) 피해자연속킬=%d 피해자생존시간=%.1f 최근기여자수=%d"),
		*VictimPS->GetName(), KillerPS ? *KillerPS->GetName() : TEXT("NULL"), bValidKiller ? 1 : 0,
		VictimKillStreak, VictimTimeSinceLastDeath, RecentDamageContributors.Num());

	VictimPS->AddDeath();

	if (bValidKiller)
	{
		KillerPS->AddKill();
		if (AP1HeroCharacter* KillerHero = Cast<AP1HeroCharacter>(KillerPS->GetPawn()))
		{
			KillerHero->GrantKillReward(VictimKillStreak, VictimTimeSinceLastDeath, VictimLevel);
		}

		// 팀 킬스코어 집계 — 승리 임계치 판정/매치 종료 오케스트레이션은 GameMode가 전담(AttributeSet은
		// "킬이 났다"는 사실과 팀 ID만 전달). Instant GE 실행 컨텍스트라 이 함수 전체가 서버 전용이다
		// (다른 킬 보상 처리와 동일한 암묵적 전제).
		if (AP1ArenaGameMode* ArenaGM = World ? World->GetAuthGameMode<AP1ArenaGameMode>() : nullptr)
		{
			ArenaGM->OnTeamKillScored(KillerPS->GetGenericTeamId().GetId());
		}
	}

	const float AssistWindowStart = World ? World->GetTimeSeconds() - AssistWindowSeconds : 0.0f;
	for (const TPair<TWeakObjectPtr<AP1PlayerState>, float>& Pair : RecentDamageContributors)
	{
		AP1PlayerState* AssisterPS = Pair.Key.Get();
		if (!AssisterPS || AssisterPS == KillerPS || AssisterPS == VictimPS || Pair.Value < AssistWindowStart)
		{
			continue;
		}

		AssisterPS->AddAssist();
		if (AP1HeroCharacter* AssisterHero = Cast<AP1HeroCharacter>(AssisterPS->GetPawn()))
		{
			AssisterHero->GrantAssistReward(VictimLevel);
		}
	}

	// 다음 생애주기는 기여 기록 없이 새로 시작 — 방금 죽은 시점 이전 기록이 다음 죽음에 영향 주면 안 된다.
	RecentDamageContributors.Empty();
}

void UP1AttributeSet::HandleMonsterKillRewards(const FGameplayEffectModCallbackData& Data, AActor* VictimOwner)
{
	AP1JungleMonsterCharacter* VictimMonster = Cast<AP1JungleMonsterCharacter>(VictimOwner);
	if (!VictimMonster)
	{
		return;
	}

	// 정글 몬스터를 죽였을 때의 Instigator도 히어로 데미지와 동일한 컨벤션(ASC OwnerActor=AP1PlayerState)을 따른다.
	AP1PlayerState* KillerPS = Cast<AP1PlayerState>(Data.EffectSpec.GetEffectContext().GetInstigator());
	AP1HeroCharacter* KillerHero = KillerPS ? Cast<AP1HeroCharacter>(KillerPS->GetPawn()) : nullptr;
	if (!KillerHero)
	{
		return;
	}

	int32 GoldAmount = 0;
	float ExperienceAmount = 0.0f;
	VictimMonster->GetKillReward(GoldAmount, ExperienceAmount);
	KillerHero->GrantMonsterKillReward(GoldAmount, ExperienceAmount);

	UE_LOG(LogP1, Log, TEXT("[AttributeSet][Kill] 정글 몬스터 처치 — %s → 킬러=%s (Gold=%d XP=%.0f)"),
		*VictimMonster->GetName(), *KillerPS->GetName(), GoldAmount, ExperienceAmount);
}
