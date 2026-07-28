// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/P1JungleMonsterCharacter.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AI/P1JungleMonsterAIController.h"
#include "Player/P1PlayerState.h"
#include "UI/P1FloatingWidgetComponent.h"
#include "UI/Widget/P1FloatingStatusWidget.h"
#include "UI/WidgetController/P1FloatingStatusWidgetController.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "P1.h"

AP1JungleMonsterCharacter::AP1JungleMonsterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// AP1JungleCampAnchor는 SpawnActor로 런타임에 몬스터를 만든다(=Spawned) — 레벨에 직접 배치해
	// 테스트하는 경우(=PlacedInWorld)도 함께 커버해둔다. 기본값(Disabled)이면 AIControllerClass가
	// 지정돼 있어도 아무도 Possess하지 않아 BT가 영원히 안 돎.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// 이 ASC엔 "소유 클라이언트"가 없다(AI 컨트롤러 소유, 사람이 조종하지 않음) — Full로 GE까지
	// 전 클라이언트에 복제한다. PlayerState가 쓰는 Mixed(소유자 전용 GE 복제)는 사람이 조종하는
	// 캐릭터 전용 최적화라 여기선 의미가 없고, 오히려 스턴바 버그와 같은 종류의 문제(프록시가 GE를
	// 못 읽어 UI가 깨지는)를 재현하게 된다 — CLAUDE.md의 스턴바 버그 기록 참고.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

	AttributeSet = CreateDefaultSubobject<UP1AttributeSet>(TEXT("AttributeSet"));

	// CachedAbilitySystemComponent(=IAbilitySystemInterface::GetAbilitySystemComponent()가 반환하는 값)를
	// 여기 생성자에서 미리 채워둔다 — BeginPlay()에서 채우면 클라이언트에서 리플리케이션 초기 프로퍼티
	// 수신(OnRep_Health 등, GAMEPLAYATTRIBUTE_REPNOTIFY가 내부적으로 GetOwningAbilitySystemComponentChecked()
	// 를 호출함)이 BeginPlay()보다 먼저 도착할 수 있어서 그 사이에 크래시가 났었다(check(Result) 실패,
	// Result=null). 히어로는 ASC/AttributeSet이 PlayerState 생성자에서 함께 만들어지고 PlayerState
	// 자신이 IAbilitySystemInterface를 구현해서 이 경쟁이 애초에 없었는데, 정글몹은 Pawn이 직접 ASC를
	// 들고 있어서 이 대입을 최대한 일찍(생성자, 리플리케이션이 시작되기 전) 해줘야 한다.
	CachedAbilitySystemComponent = AbilitySystemComponent;

	CharacterType = TAG_Character_Type_Monster;
	// TeamId는 베이스 기본값(255=NoTeam) 그대로 — IsSameTeam()이 항상 false를 반환해 모든 영웅에게
	// 자동으로 적대(공격 가능)가 된다. 별도 설정 불필요.
}

void AP1JungleMonsterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 캠프 앵커/스포너가 아직 없으므로 우선 레벨에 배치된 스폰 위치를 홈으로 사용한다 — 나중에
	// 스포너가 생기면 스폰 직후 SetHomeLocation()으로 정확한 캠프 좌표로 덮어쓰면 된다.
	HomeLocation = GetActorLocation();

	// CachedAbilitySystemComponent는 생성자에서 이미 채워져 있다(위 생성자 주석 참고).
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(TAG_Event_Character_HitReact)
		.AddUObject(this, &AP1JungleMonsterCharacter::OnHitReactEventReceived);
	AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(TAG_Event_Character_Died)
		.AddUObject(this, &AP1JungleMonsterCharacter::OnDiedEventReceived);

	if (HasAuthority())
	{
		ApplyDefaultAttributes();

		if (MeleeAttackAbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(MeleeAttackAbilityClass, 1));
		}
	}

	// 머리 위 HP 바 — 히어로(AP1HeroCharacter::HandleAbilitySystemReady)와 동일한 초기화 패턴이지만,
	// 몬스터는 "누구 화면에서도" 항상 보여야 하므로 IsLocallyControlled() 숨김 분기가 없다(애초에
	// 사람이 조종하지 않아 항상 false).
	if (IsValid(FloatingStatusComponent))
	{
		FloatingStatusComponent->InitWidget();
		if (UP1FloatingStatusWidget* FloatingWidget =
			Cast<UP1FloatingStatusWidget>(FloatingStatusComponent->GetUserWidgetObject()))
		{
			if (!FloatingStatusWidgetController)
			{
				const FWidgetControllerParams Params(nullptr, nullptr, AbilitySystemComponent, AttributeSet);
				FloatingStatusWidgetController = NewObject<UP1FloatingStatusWidgetController>(this);
				FloatingStatusWidgetController->SetWidgetControllerParams(Params);
				FloatingStatusWidgetController->BindCallbacksToDependencies();
			}
			FloatingWidget->SetWidgetController(FloatingStatusWidgetController);
			FloatingStatusWidgetController->BroadcastInitialValues();
			FloatingWidget->SetLevel(MonsterLevel);
		}
	}
}

void AP1JungleMonsterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsLeashRecovering || !HasAuthority() || !AttributeSet)
	{
		return;
	}

	const float MaxHealth = AttributeSet->GetMaxHealth();
	const float HealAmount = MaxHealth * LeashRecoveryHealPercentPerSecond * DeltaSeconds;
	AttributeSet->SetHealth(FMath::Min(AttributeSet->GetHealth() + HealAmount, MaxHealth));
}

void AP1JungleMonsterCharacter::ApplyDefaultAttributes()
{
	if (!DefaultAttributesEffectClass || !AbilitySystemComponent)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	// MonsterLevel을 스펙 레벨로 넘겨 GE의 Scalable Float 커브가 이 레벨 기준으로 평가되게 한다
	// (AP1HeroCharacter::ApplyBaseStatsForLevel과 동일한 패턴).
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributesEffectClass, static_cast<float>(MonsterLevel), EffectContext);
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

		// GE는 보통 MaxHealth만 설정하고 Health 자체는 안 건드리므로(히어로 쪽 컨벤션과 동일), 새로
		// 산출된 MaxHealth로 풀피 확정 — 안 하면 UP1AttributeSet 생성자 기본값(680)이 그대로 남아있다가
		// MaxHealth가 그보다 크면 덜 채운 채로, 작으면 클램프된 채로 스폰되는 문제가 생긴다.
		if (AttributeSet)
		{
			AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
		}
	}
}

void AP1JungleMonsterCharacter::EnterLeashRecovery()
{
	if (!HasAuthority() || bIsLeashRecovering)
	{
		return;
	}

	if (ResetInvulnerabilityEffectClass && AbilitySystemComponent)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(ResetInvulnerabilityEffectClass, 1.0f, EffectContext);
		if (SpecHandle.IsValid())
		{
			ResetInvulnerabilityEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	bIsLeashRecovering = true;
}

void AP1JungleMonsterCharacter::ExitLeashRecovery()
{
	if (!HasAuthority())
	{
		return;
	}

	if (AbilitySystemComponent && ResetInvulnerabilityEffectHandle.IsValid())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(ResetInvulnerabilityEffectHandle);
		ResetInvulnerabilityEffectHandle.Invalidate();
	}

	if (AttributeSet)
	{
		AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	}

	bIsLeashRecovering = false;
}

void AP1JungleMonsterCharacter::RequestMeleeAttack()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = TAG_Event_Monster_MeleeAttack;
	AbilitySystemComponent->HandleGameplayEvent(TAG_Event_Monster_MeleeAttack, &EventData);
}

void AP1JungleMonsterCharacter::OnHitReactEventReceived(const FGameplayEventData* Payload)
{
	if (!HasAuthority() || !Payload)
	{
		return;
	}

	// 히어로가 가한 데미지는 Instigator가 공격자의 AP1PlayerState(Pawn이 아님, ASC OwnerActor
	// 컨벤션 — P1AttributeSet.cpp의 HitReact 발신부 주석 참고)이므로 GetPawn()으로 한 번 더 풀어야
	// 한다. 몬스터끼리 싸우게 되는 경우 등 Instigator가 이미 Pawn 자신인 경우도 대비해둔다.
	AActor* InstigatorActor = const_cast<AActor*>(Payload->Instigator.Get());

	APawn* AttackerPawn = nullptr;
	if (AP1PlayerState* InstigatorPS = Cast<AP1PlayerState>(InstigatorActor))
	{
		AttackerPawn = InstigatorPS->GetPawn();
	}
	else
	{
		AttackerPawn = Cast<APawn>(InstigatorActor);
	}

	if (AttackerPawn)
	{
		if (AP1JungleMonsterAIController* AICon = Cast<AP1JungleMonsterAIController>(GetController()))
		{
			AICon->NotifyAggro(AttackerPawn);
		}
	}
}

void AP1JungleMonsterCharacter::OnDiedEventReceived(const FGameplayEventData* Payload)
{
	if (!HasAuthority())
	{
		return;
	}

	// BT/이동을 멈춰서 사망 몽타주 재생 구간 동안 시체가 계속 공격/추적하는 걸 방지.
	if (AController* MyController = GetController())
	{
		MyController->StopMovement();
		if (AAIController* AICon = Cast<AAIController>(MyController))
		{
			if (UBrainComponent* Brain = AICon->GetBrainComponent())
			{
				Brain->StopLogic(TEXT("Died"));
			}
		}
	}
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}

	// 히어로의 State.Dead 패턴(그 자리에서 GE 만료까지 대기 후 리스폰)과 달리, 정글 몬스터는 캠프
	// 스포너(AP1JungleCampAnchor)가 완전히 새 인스턴스를 다시 스폰하는 방식이라 이 액터는 그대로 정리한다.
	// 리스폰 대기시간은 죽는 순간부터 카운트되는 게 자연스러우므로 델리게이트는 즉시 브로드캐스트.
	OnMonsterDied.Broadcast(this);

	if (DeathMontage)
	{
		MulticastPlayDeathMontage();
		SetLifeSpan(DeathDestroyDelay);
	}
	else
	{
		Destroy();
	}
}

void AP1JungleMonsterCharacter::MulticastPlayDeathMontage_Implementation()
{
	if (!DeathMontage)
	{
		UE_LOG(LogP1, Warning, TEXT("[JungleMonster] MulticastPlayDeathMontage — DeathMontage가 설정되지 않음 (%s)"), *GetName());
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogP1, Warning, TEXT("[JungleMonster] MulticastPlayDeathMontage — AnimInstance가 없음(Mesh=%d, AnimBP 미설정 가능성) (%s)"),
			MeshComp != nullptr, *GetName());
		return;
	}

	const float PlayLength = AnimInstance->Montage_Play(DeathMontage);
	UE_LOG(LogP1, Log, TEXT("[JungleMonster] MulticastPlayDeathMontage — Montage_Play 결과=%.2f(0이면 재생 실패) (%s)"), PlayLength, *GetName());
}
