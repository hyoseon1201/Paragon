// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/P1DamageGameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "ScalableFloat.h"
#include "P1GameplayAbility_IonStrike.generated.h"

class UGameplayEffect;
class UParticleSystem;
class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class AGameplayAbilityTargetActor;

// R(궁극기) — Ion Strike (Dekker). 원본(Predecessor) 툴팁의 "짧은 딜레이 후 단발 블래스트"는
// 이 프로젝트가 쓰는 구버전 Paragon 에셋에 대응 연출이 없어 폐기 — 대신 실제로 갖고 있는
// "하늘에서 화살비가 떨어지는" 연출에 맞춰 지속 데미지 + 마법방어력 감소(누적)로 재설계.
//
// ContainmentFence/AssaultTheGates와 동일한 지면 조준(WaitTargetData+GroundDecal, 미확정 시
// 미커밋) 후, 확정 위치에 화살비 연출을 재생하고 MakeWay(Q)와 완전히 동일한 패턴으로 어빌리티가
// 직접 소유한 반복 타이머를 시작한다 — 다만 MakeWay는 매 틱 "캐릭터 현재 위치"를 재스캔하는 반면,
// 여기는 "확정된 고정 지면 좌표"를 매 틱 재스캔한다는 점만 다르다(고정 타겟 기반 Periodic
// GameplayEffect로는 여전히 "매 틱 그 자리 기준으로 새로 스캔"을 표현할 수 없어 타이머가 필요).
// 쿨다운은 지속시간이 끝나야 시작되므로(MakeWay와 동일 이유), 확정 시점엔 CommitAbilityCost()만
// 커밋하고 마지막 틱 직후 쿨다운을 따로 적용한다.
UCLASS()
class P1_API UP1GameplayAbility_IonStrike : public UP1DamageGameplayAbility
{
	GENERATED_BODY()

public:
	UP1GameplayAbility_IonStrike();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 조준 확정 — 타겟 데이터(화살비 중심점) 수신.
	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data);

	// 조준 취소 — 무소모 종료.
	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& Data);

	// 0.5초 간격 반복 틱 — 확정 위치 기준 재스캔하여 데미지+마법방어력 감소 적용.
	void OnRainTick();

	// 조준 중(WaitTargetData 활성) 재생되는 포즈 — 하늘을 올려다보며 낙하지점을 겨냥하는 대기 자세.
	// 몽타주 에셋 자체를 Loop로 설정해야 하며(StasisBomb의 AimMontage와 동일 컨벤션), 확정/취소 시
	// StopAimMontage()로 명시적으로 정지한다. 미설정 시 재생 생략.
	UPROPERTY(EditDefaultsOnly, Category = "IonStrike|Montage")
	TObjectPtr<UAnimMontage> AimMontage;

	// 확정 순간 1회 재생되는 소환 캐스팅 연출(fire-and-forget) — 지속 판정(OnRainTick)과는 별개로
	// 재생만 하고 끝, 어빌리티 종료를 기다리지 않는다. 미설정 시 재생 생략.
	UPROPERTY(EditDefaultsOnly, Category = "IonStrike|Montage")
	TObjectPtr<UAnimMontage> FireMontage;

	// --- 조준 ---
	UPROPERTY(EditDefaultsOnly, Category = "IonStrike|Targeting")
	TSubclassOf<AGameplayAbilityTargetActor> TargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "IonStrike|Targeting")
	float MaxRange = 1500.0f;

	// --- 지속 판정 ---
	UPROPERTY(EditDefaultsOnly, Category = "IonStrike")
	float EffectRadius = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "IonStrike")
	float HalfHeight = 250.0f;

	// 틱 간격(초) — 고정값.
	UPROPERTY(EditDefaultsOnly, Category = "IonStrike")
	float TickPeriod = 0.5f;

	// 지속시간(초) — R 컨벤션대로 3랭크(MaxAbilityLevel=3, RequiredCharacterLevelPerRank={6,11,15}).
	// 레벨별 실제 값은 BP/커브테이블에서 지정(미지정 시 3.0 고정값). TotalTicks는 ActivateAbility
	// 시점에 Duration.GetValueAtLevel() / TickPeriod로 매 캐스트마다 계산한다(레벨마다 틱 수가 다름).
	UPROPERTY(EditDefaultsOnly, Category = "IonStrike")
	FScalableFloat Duration = FScalableFloat(3.0f);

	// 틱마다 누적 적용되는 마법방어력 감소량(%) — 3랭크 값은 BP/커브테이블에서 지정.
	// GE 쪽 Modifier가 Data.DebuffMagnitude(SetByCaller)를 음수 계수로 받아 MagicalArmor를 깎고,
	// Stacking Type=AggregateByTarget(또는 Source)로 틱마다 누적되도록 GE 에셋에서 구성한다.
	UPROPERTY(EditDefaultsOnly, Category = "IonStrike")
	FScalableFloat MagicalArmorShredPerTick = FScalableFloat(1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "IonStrike")
	TSubclassOf<UGameplayEffect> MagicalArmorShredDebuffEffectClass;

	// 화살비 연출 — 확정 위치에서 지속시간 내내 유지되다 마지막 틱(또는 조기 중단) 시점에 명시적으로
	// 정지된다(MulticastSetPersistentParticleEffectAtLocation/Stop 페어, bAutoDestroy=false). 에셋 자체가
	// 무한 루프여도 상관없다 — 지속시간에 자동으로 맞춰지길 기대하지 않는다.
	UPROPERTY(EditDefaultsOnly, Category = "IonStrike|VFX")
	TObjectPtr<UParticleSystem> ArrowRainEffectTemplate;

	// ArrowRainEffectTemplate 스폰 시 적용할 회전값 — 에셋의 파티클 스폰/속도 방향이 로컬 스페이스
	// 기준이면 여기서 조정만으로 방향을 바꿀 수 있다(월드 스페이스로 박혀있으면 여기서 조정해도
	// 효과가 없고 에셋 자체를 고쳐야 함 — 자세한 진단 방법은 GA BP 설정 노트 참고).
	UPROPERTY(EditDefaultsOnly, Category = "IonStrike|VFX")
	FRotator ArrowRainEffectRotation = FRotator::ZeroRotator;

	// ArrowRainEffectTemplate 크기 배율 — 에셋 자체를 수정하지 않고 여기서 줄이거나 키울 수 있다
	// (PhotonDisruptor::GroundBlastEffectScale과 동일한 이유). 기본값 (1,1,1).
	UPROPERTY(EditDefaultsOnly, Category = "IonStrike|VFX")
	FVector ArrowRainEffectScale = FVector(1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "IonStrike|Debug")
	bool bShowDebug = false;

private:
	void BeginTargeting();

	// 조준 태그 부여/해제 — AssaultTheGates::SetTargetingState()와 동일한 패턴.
	void SetTargetingState(bool bEnable);

	// AimMontage 정지 — EndTask만으로는 루프 몽타주가 확실히 안 멎을 수 있어 Montage_Stop도 함께 호출한다
	// (StasisBomb::StopAimMontage()와 동일 패턴).
	void StopAimMontage();

	bool bTargetingActive = false;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> AimMontageTask;

	// 확정된 지면 중심점 — 매 틱 이 좌표 기준으로 재스캔한다(캐릭터가 움직여도 고정).
	FVector ConfirmedLocation = FVector::ZeroVector;

	FTimerHandle TickTimerHandle;
	int32 CurrentTick = 0;
	int32 TotalTicks = 0;

	// 조준 취소(OnTargetCancelled)와 구분하기 위한 플래그 — 코스트가 실제로 나갔는지(=OnTargetDataReady
	// 진입 성공) 여부. EndAbility의 쿨다운 폴백이 조준만 하고 취소한 경우까지 쿨다운을 걸어버리는
	// 것을 막는다(PhotonDisruptor에서 겪은 것과 동일한 버그 패턴).
	bool bCommitted = false;

	// EndAbility가 여러 경로(정상 종료/중단)로 여러 번 불려도 쿨다운이 중복 적용되지 않도록 하는 가드.
	bool bCooldownApplied = false;
};
