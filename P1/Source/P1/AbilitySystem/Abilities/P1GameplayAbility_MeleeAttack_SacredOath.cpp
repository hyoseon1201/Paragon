// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/P1GameplayAbility_MeleeAttack_SacredOath.h"
#include "P1.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "Characters/P1CharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "DrawDebugHelpers.h"

UP1GameplayAbility_MeleeAttack_SacredOath::UP1GameplayAbility_MeleeAttack_SacredOath()
{
	// 이 클래스가 이미 "Sacred Oath 시너지가 있는 영웅의 유일한 BasicAttack"이므로 별도의
	// Required/Blocked 태그 게이팅이 필요 없다 — 버프 유무 판정은 ApplyComboHitDamage에서 처리.
}

void UP1GameplayAbility_MeleeAttack_SacredOath::ApplyComboHitDamage(const TArray<AActor*>& EnemyTargets, AActor* PrimaryTarget, float CleavePct)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	const bool bBuffActive = ASC->HasMatchingGameplayTag(TAG_Buff_SacredOath_Active);
	UE_LOG(LogP1, Log, TEXT("[MeleeAttack_SacredOath] ApplyComboHitDamage 진입 — Auth=%d BuffActive=%d EnemyCount=%d"),
		CurrentActorInfo->IsNetAuthority() ? 1 : 0, bBuffActive ? 1 : 0, EnemyTargets.Num());

	// 콤보가 여러 스텝 이어지는 동안 같은 인스턴스가 재사용되므로, 첫 스윙에서 버프를 이미
	// 소모했다면 이후 스텝은 평소 로직으로 폴백한다 — "다음 1회만" 강화를 보장.
	if (!bBuffActive)
	{
		Super::ApplyComboHitDamage(EnemyTargets, PrimaryTarget, CleavePct);
		return;
	}

	AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo();

	const float ResolvedBonusFlat = BonusFlatDamage.GetValueAtLevel(GetAbilityLevel());

#if ENABLE_DRAW_DEBUG
	if (bShowDebugSacredOath)
	{
		for (AActor* Enemy : EnemyTargets)
		{
			DrawDebugSphere(GetWorld(), Enemy->GetActorLocation(), 55.0f, 16, FColor::Orange, false, 1.5f, 0, 3.0f);
			DrawDebugString(GetWorld(), Enemy->GetActorLocation() + FVector(0, 0, 100.0f),
				FString::Printf(TEXT("SacredOath +%.1f"), ResolvedBonusFlat), nullptr, FColor::Orange, 1.5f);
		}
	}
#endif

	for (AActor* Enemy : EnemyTargets)
	{
		// Cleave 무시 — 범위 내 전원에게 100% 데미지.
		UE_LOG(LogP1, Log, TEXT("[MeleeAttack_SacredOath] Damage loop: %s | IsPrimary=%d | BonusFlat=%.1f"),
			*Enemy->GetName(), (Enemy == PrimaryTarget) ? 1 : 0, ResolvedBonusFlat);

		ApplyDamageToTarget(Enemy, 1.0f, ResolvedBonusFlat, BonusPhysicalPowerCoefficient,
			0.0f, BonusTargetMaxHealthPctCoefficient, 0.0f);

		if (SlowDebuffEffectClass)
		{
			if (IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(Enemy))
			{
				if (UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent())
				{
					FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
					Ctx.AddSourceObject(SourceCharacter);
					const FGameplayEffectSpecHandle SlowSpec = ASC->MakeOutgoingSpec(SlowDebuffEffectClass, GetAbilityLevel(), Ctx);
					if (SlowSpec.IsValid())
					{
						SlowSpec.Data->SetSetByCallerMagnitude(TAG_Data_DebuffMagnitude, SlowPercent);
						ASC->ApplyGameplayEffectSpecToTarget(*SlowSpec.Data.Get(), TargetASC);
						UE_LOG(LogP1, Log, TEXT("[MeleeAttack_SacredOath] 슬로우 디버프 적용 — Target=%s SlowPercent=%.2f"),
							*Enemy->GetName(), SlowPercent);
					}
				}
			}
		}
	}

	UE_LOG(LogP1, Log, TEXT("[MeleeAttack_SacredOath] ApplyComboHitDamage 끝 — ConsumeSacredOathBuff 호출 직전"));
	ConsumeSacredOathBuff();
}

void UP1GameplayAbility_MeleeAttack_SacredOath::OnHitEventReceived(FGameplayEventData Payload)
{
	// 적중 여부와 무관하게 "스윙이 발생했다"는 시점 — 허공을 휘둘러도 버프는 소모되지 않으므로,
	// 태그가 아직 살아있는 한 매번 트레일을 재생한다. 이번 스윙이 실제로 적을 맞혀 버프를 소모하는
	// 스윙이더라도 소모 판정(Super → ApplyComboHitDamage)보다 먼저 체크하므로 트레일은 정상 재생된다.
	// Multicast는 서버에서만 호출(클라 인스턴스가 호출하면 무시/경고).
	if (CurrentActorInfo->IsNetAuthority() && TrailParticleTemplate)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			if (ASC->HasMatchingGameplayTag(TAG_Buff_SacredOath_Active))
			{
				if (AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo())
				{
					SourceCharacter->MulticastPlayParticleEffect(TrailParticleTemplate, TrailSocketName);
				}
			}
		}
	}

	Super::OnHitEventReceived(Payload);
}

void UP1GameplayAbility_MeleeAttack_SacredOath::ConsumeSacredOathBuff() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		UE_LOG(LogP1, Error, TEXT("[MeleeAttack_SacredOath] ConsumeSacredOathBuff — ASC null, 소모 실패"));
		return;
	}

	// 태그를 부여한 GE 자체를 제거 — 검 발광 원복은 AP1HeroCharacter가 이 태그의
	// 카운트 변경 이벤트(RegisterGameplayTagEvent)를 구독해 자동으로 처리한다.
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Buff_SacredOath_Active);
	const int32 RemovedCount = ASC->RemoveActiveEffectsWithGrantedTags(Tags);
	UE_LOG(LogP1, Log, TEXT("[MeleeAttack_SacredOath] RemoveActiveEffectsWithGrantedTags 결과: %d개 제거됨 (Auth=%d)"),
		RemovedCount, CurrentActorInfo->IsNetAuthority() ? 1 : 0);
}
