// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/P1GameplayAbility_JungleMonsterMeleeAttack.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/P1CharacterBase.h"
#include "Animation/AnimMontage.h"
#include "Engine/OverlapResult.h"
#include "P1.h"

UP1GameplayAbility_JungleMonsterMeleeAttack::UP1GameplayAbility_JungleMonsterMeleeAttack()
{
	// Ability.BasicAttack 재사용 — MeleeAttack/RangedAttack과 동일하게, 이 태그가 있어야
	// UP1AttributeSet::PostGameplayEffectExecute의 Stoicism 디플렉트 판정("기본공격에서 왔는지")이
	// 몬스터의 근접 공격도 인식한다(디플렉트가 몬스터 공격도 막을 수 있어야 하므로 의도적인 재사용).
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_Ability_BasicAttack);
	SetAssetTags(Tags);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = TAG_Event_Monster_MeleeAttack;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// AI 전용, 예측할 사람이 없다 — 서버에서만 실행.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UP1GameplayAbility_JungleMonsterMeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		// 쿨다운 중이면 CommitAbility가 실패한다(CooldownGameplayEffectClass 미설정이면 항상 성공) —
		// 이걸로 재공격 간격이 자동 보장된다.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AP1CharacterBase* SourceCharacter = GetP1CharacterFromActorInfo())
	{
		const TArray<AActor*> Enemies = GetEnemiesInRadius(SourceCharacter->GetActorLocation(), AttackRadius, AttackHalfHeight);
		for (AActor* Enemy : Enemies)
		{
			ApplyDamageToTarget(Enemy);
		}

		UE_LOG(LogP1, Log, TEXT("[JungleMonster] MeleeAttack 발동 — %d명 적중 (%s)"), Enemies.Num(), *SourceCharacter->GetName());

		// 진단용 — 0명 적중이 반복되는 원인(수평 사거리 밖인지, 팀 판정인지, 캡슐 높이차로 인한
		// HalfHeight 필터인지)을 바로 확인할 수 있게 반경을 3배 넓혀 재검색해서 후보들의 수치를 찍는다.
		// GetEnemiesInRadius가 이미 필터링해버린 이유(팀/높이)는 이 로그로만 알 수 있다.
		if (Enemies.Num() == 0)
		{
			TArray<FOverlapResult> DebugOverlaps;
			FCollisionQueryParams DebugParams;
			DebugParams.AddIgnoredActor(SourceCharacter);
			GetWorld()->OverlapMultiByChannel(DebugOverlaps, SourceCharacter->GetActorLocation(), FQuat::Identity,
				ECC_Pawn, FCollisionShape::MakeSphere(AttackRadius * 3.0f), DebugParams);

			if (DebugOverlaps.Num() == 0)
			{
				UE_LOG(LogP1, Warning, TEXT("[JungleMonster] 0명 적중 진단 — 반경 %.0f(사거리의 3배) 안에 Pawn이 아예 없음. 사거리가 너무 좁거나 BT의 MoveTo Acceptance Radius가 너무 큰 것으로 보임"), AttackRadius * 3.0f);
			}
			for (const FOverlapResult& R : DebugOverlaps)
			{
				if (AP1CharacterBase* Candidate = Cast<AP1CharacterBase>(R.GetActor()))
				{
					const float HorizDist = FVector::Dist2D(Candidate->GetActorLocation(), SourceCharacter->GetActorLocation());
					const float ZDiff = FMath::Abs(Candidate->GetActorLocation().Z - SourceCharacter->GetActorLocation().Z);
					UE_LOG(LogP1, Warning, TEXT("[JungleMonster] 0명 적중 진단 — 후보=%s 수평거리=%.0f(사거리=%.0f) Z차이=%.0f(HalfHeight=%.0f) 같은팀=%d"),
						*Candidate->GetName(), HorizDist, AttackRadius, ZDiff, AttackHalfHeight,
						AP1CharacterBase::IsSameTeam(SourceCharacter, Candidate) ? 1 : 0);
				}
			}
		}

		// ASC->PlayMontage()를 써야 서버에서 재생해도 리모트 클라이언트까지 전 클라이언트에 복제된다
		// (raw AnimInstance->Montage_Play는 서버 화면에만 보임 — NetExecutionPolicy=ServerOnly라 클라
		// 예측 경로가 없으므로 몽타주 복제는 전적으로 ASC의 RepAnimMontageInfo에 의존).
		if (AttackMontage)
		{
			if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
			{
				const float PlayLength = ASC->PlayMontage(this, ActivationInfo, AttackMontage, 1.0f);
				UE_LOG(LogP1, Log, TEXT("[JungleMonster] MeleeAttack PlayMontage 결과=%.2f(0 이하면 재생 실패, AnimInstance 미설정 가능성) (%s)"), PlayLength, *SourceCharacter->GetName());
			}
			else
			{
				UE_LOG(LogP1, Warning, TEXT("[JungleMonster] MeleeAttack — GetAbilitySystemComponentFromActorInfo()가 null (%s)"), *SourceCharacter->GetName());
			}
		}
		else
		{
			UE_LOG(LogP1, Warning, TEXT("[JungleMonster] MeleeAttack — AttackMontage가 설정되지 않음 (%s)"), *SourceCharacter->GetName());
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
