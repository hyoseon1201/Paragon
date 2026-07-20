// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/P1HeroCharacter.h"
#include "Player/P1PlayerState.h"
#include "Player/P1PlayerController.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "AbilitySystem/P1GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayEffect.h"
#include "Engine/CurveTable.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/P1CameraComponent.h"
#include "Characters/P1HeroComponent.h"
#include "Characters/P1BuffCosmeticEffectComponent.h"
#include "MotionWarpingComponent.h"
#include "UI/P1FloatingWidgetComponent.h"
#include "UI/Widget/P1FloatingStatusWidget.h"
#include "UI/HUD/P1HUD.h"
#include "UI/WidgetController/P1FloatingStatusWidgetController.h"
#include "P1.h"

AP1HeroCharacter::AP1HeroCharacter()
{
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;

	CameraComponent = CreateDefaultSubobject<UP1CameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(GetRootComponent());

	HeroComponent = CreateDefaultSubobject<UP1HeroComponent>(TEXT("HeroComponent"));

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
}

FGenericTeamId AP1HeroCharacter::GetGenericTeamId() const
{
	const AP1PlayerState* P1PS = GetPlayerState<AP1PlayerState>();
	return P1PS ? P1PS->GetGenericTeamId() : FGenericTeamId::NoTeam;
}

void AP1HeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	HandleAbilitySystemReady();
	AddDefaultAbilities();
}

void AP1HeroCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	HandleAbilitySystemReady();
}

void AP1HeroCharacter::HandleAbilitySystemReady()
{
	AP1PlayerState* P1PS = GetPlayerState<AP1PlayerState>();
	if (!IsValid(P1PS))
	{
		UE_LOG(LogP1, Warning, TEXT("[GAS] HandleAbilitySystemReady: PlayerState is null on %s"), *GetName());
		return;
	}

	UAbilitySystemComponent* ASC = P1PS->GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		UE_LOG(LogP1, Warning, TEXT("[GAS] HandleAbilitySystemReady: ASC is null on %s"), *GetName());
		return;
	}

	InitAbilityActorInfo(P1PS, ASC);

	// 사망/리스폰: AttributeSet이 보내는 이벤트(Died) 및 State.Dead 태그 변경(전 클라이언트, 태그 복제로 자동 전파)을 구독.
	ASC->GenericGameplayEventCallbacks.FindOrAdd(TAG_Event_Character_Died).AddUObject(this, &AP1HeroCharacter::OnDiedEventReceived);
	ASC->GenericGameplayEventCallbacks.FindOrAdd(TAG_Event_Montage_Death_Impact).AddUObject(this, &AP1HeroCharacter::OnDeathImpactEventReceived);
	ASC->RegisterGameplayTagEvent(TAG_State_Dead, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AP1HeroCharacter::OnDeadTagChanged);

	// 기절 — 각 클라이언트가 독립적으로 몽타주 재생/정지(태그 복제로 자동 전파). 이동/어빌리티 입력
	// 차단은 여기서 안 하고 AP1PlayerController::HandleMove()/베이스 어빌리티 ActivationBlockedTags가
	// 각자 담당(실제 입력 바인딩이 캐릭터가 아니라 PlayerController의 InputComponent에 있어서, 여기서
	// DisableInput을 불러도 그 바인딩엔 영향이 없기 때문 — OnDeadTagChanged가 쓰는 DisableInput 패턴은
	// 같은 이유로 재검토가 필요할 수 있음).
	ASC->RegisterGameplayTagEvent(TAG_State_Stunned, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AP1HeroCharacter::OnStunTagChanged);

	// 버프 태그에 반응하는 코스메틱 이펙트(검 발광 등)가 있는 영웅이면(컴포넌트가 붙어있으면) ASC를 넘겨준다.
	// 어떤 태그/이펙트인지는 전부 컴포넌트 쪽 데이터가 갖고 있어 이 클래스는 특정 스킬을 몰라도 된다.
	if (UP1BuffCosmeticEffectComponent* CosmeticComp = FindComponentByClass<UP1BuffCosmeticEffectComponent>())
	{
		CosmeticComp->BindToAbilitySystemComponent(ASC);
	}

	if (HasAuthority())
	{
		ApplyBaseStatsForLevel(P1PS->GetCharacterLevel(), /*bFullHeal=*/true);
	}

	BindMoveSpeedAttribute();

	// 머리 위 위젯 — per-character FloatingStatusWidgetController 생성·바인딩.
	if (IsValid(FloatingStatusComponent))
	{
		// 자기 자신의 머리 위 바는 자기 화면엔 안 보여야 한다 — 아래 HUD 생성과 동일하게
		// IsLocallyControlled()로 판단. 다른 클라이언트가 이 폰을 볼 때는 그쪽에서 false이므로
		// 정상 표시된다. Space=Screen이라 회전/빌보드 처리는 필요 없다.
		FloatingStatusComponent->SetVisibility(!IsLocallyControlled());

		// WidgetComponent는 UserWidget 인스턴스를 지연 생성한다 — 원격 클라이언트에서 방금 막
		// 리플리케이트되어 들어온(다른 플레이어의) 폰의 경우, 이 함수가 이 시점에 불릴 때
		// 아직 위젯이 생성 전이라 GetUserWidgetObject()가 null을 반환할 수 있다. 그러면 이 블록 전체가
		// 스킵되고, OnRep_PlayerState는 보통 폰 생애주기 중 다시 안 불리므로 그 폰의 머리 위 바가
		// 영원히 바인딩 안 된 채(0/1 표시) 남는다 — InitWidget()으로 강제 즉시 생성해 이 경쟁을 없앤다.
		FloatingStatusComponent->InitWidget();
		if (UP1FloatingStatusWidget* FloatingWidget =
			Cast<UP1FloatingStatusWidget>(FloatingStatusComponent->GetUserWidgetObject()))
		{
			if (!FloatingStatusWidgetController)
			{
				const FWidgetControllerParams Params(nullptr, P1PS, ASC, P1PS->GetAttributeSet());
				FloatingStatusWidgetController = NewObject<UP1FloatingStatusWidgetController>(this);
				FloatingStatusWidgetController->SetWidgetControllerParams(Params);
				FloatingStatusWidgetController->BindCallbacksToDependencies();
			}
			FloatingWidget->SetWidgetController(FloatingStatusWidgetController);
			FloatingStatusWidgetController->BroadcastInitialValues();
			FloatingWidget->SetCharacterName(HeroDisplayName);
		}
	}

	// 소유 클라이언트(리슨서버 포함)에서만 메인 HUD 생성.
	if (IsLocallyControlled())
	{
		AP1PlayerController* PC = Cast<AP1PlayerController>(GetController());
		if (!PC)
		{
			UE_LOG(LogP1, Warning, TEXT("[HUD] HandleAbilitySystemReady: IsLocallyControlled=true인데 GetController()가 AP1PlayerController로 캐스트 실패 (%s)"), *GetName());
			return;
		}

		if (AP1HUD* P1HUD = PC->GetHUD<AP1HUD>())
		{
			P1HUD->InitOverlay(PC, P1PS, ASC, P1PS->GetAttributeSet());
		}
		else
		{
			UE_LOG(LogP1, Warning, TEXT("[HUD] HandleAbilitySystemReady: GetHUD<AP1HUD>()가 null (%s) — ArenaGameMode의 HUDClass가 BP_P1HUD로 설정됐는지 확인"), *GetName());
		}
	}
}

void AP1HeroCharacter::InitAbilityActorInfo(AP1PlayerState* P1PS, UAbilitySystemComponent* ASC)
{
	ASC->InitAbilityActorInfo(P1PS, this);
	CachedAbilitySystemComponent = ASC;
}

void AP1HeroCharacter::AddDefaultAbilities()
{
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	if (!IsValid(ASC))
	{
		return;
	}

	// 리스폰 시 RestartPlayer()가 PossessedBy()를 다시 호출한다 — ASC는 PlayerState에 남아있어
	// 어빌리티가 이미 부여된 상태이므로, 여기서 막지 않으면 매 리스폰마다 전부 중복 부여된다.
	if (UP1AbilitySystemComponent* P1ASC = Cast<UP1AbilitySystemComponent>(ASC))
	{
		if (P1ASC->AreAbilitiesGiven())
		{
			UE_LOG(LogP1, Log, TEXT("[GAS] AddDefaultAbilities: 이미 부여됨(리스폰) — 스킵 (%s)"), *GetName());
			return;
		}
	}

	UE_LOG(LogP1, Log, TEXT("[GAS] Granting %d abilities on %s"), DefaultAbilities.Num(), *GetName());

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass)
		{
			UE_LOG(LogP1, Warning, TEXT("[GAS] Null ability class in DefaultAbilities on %s"), *GetName());
			continue;
		}

		FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);
		bool bActivateOnGranted = false;

		if (const UP1GameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UP1GameplayAbility>())
		{
			UE_LOG(LogP1, Log, TEXT("[GAS] Granting %s | InputTag=%s | ActivateOnGranted=%d"),
				*AbilityClass->GetName(), *AbilityCDO->InputTag.ToString(), AbilityCDO->bActivateOnGranted ? 1 : 0);

			if (AbilityCDO->InputTag.IsValid())
			{
				AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityCDO->InputTag);
			}
			bActivateOnGranted = AbilityCDO->bActivateOnGranted;

			// 스킬 포인트로 투자 가능한 어빌리티(RMB/Q/E/R)는 Level 0(미투자)으로 부여한다 — 좌클릭/
			// 패시브처럼 항상 켜져 있는 어빌리티만 기본 Level 1을 유지. Level 0이면 UP1GameplayAbility::
			// CanActivateAbility가 발동을 막으므로, 1레벨에 아무것도 안 눌려있는 상태로 시작한다.
			if (AbilityCDO->MaxAbilityLevel > 1)
			{
				AbilitySpec.Level = 0;
			}
		}
		else
		{
			UE_LOG(LogP1, Warning, TEXT("[GAS] %s is not a UP1GameplayAbility subclass"), *AbilityClass->GetName());
		}

		// 입력도 트리거 이벤트도 없이 부여되자마자 스스로 계속 돌아야 하는 상시 패시브는
		// GiveAbilityAndActivateOnce로 그 자리에서 1회 활성화한다 — 그 안에서 반복 타이머를
		// 시작해 캐릭터 생존 내내 도는 방식(UP1GameplayAbility_StoicismVitality 등).
		if (bActivateOnGranted)
		{
			ASC->GiveAbilityAndActivateOnce(AbilitySpec);
		}
		else
		{
			ASC->GiveAbility(AbilitySpec);
		}
	}

	if (UP1AbilitySystemComponent* P1ASC = Cast<UP1AbilitySystemComponent>(ASC))
	{
		P1ASC->SetAbilitiesGiven();
	}
}

void AP1HeroCharacter::BindMoveSpeedAttribute()
{
	AP1PlayerState* P1PS = GetPlayerState<AP1PlayerState>();
	if (!IsValid(P1PS))
	{
		return;
	}

	UAbilitySystemComponent* ASC = P1PS->GetAbilitySystemComponent();
	const UP1AttributeSet* AttributeSet = P1PS->GetAttributeSet();
	if (!IsValid(ASC) || !IsValid(AttributeSet))
	{
		return;
	}

	if (MoveSpeedChangedDelegateHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMovementSpeedAttribute()).Remove(MoveSpeedChangedDelegateHandle);
	}

	MoveSpeedChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMovementSpeedAttribute())
		.AddUObject(this, &AP1HeroCharacter::OnMoveSpeedChanged);

	GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMovementSpeed();
}

void AP1HeroCharacter::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}

void AP1HeroCharacter::OnDiedEventReceived(const FGameplayEventData* EventData)
{
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	if (!IsValid(ASC) || !DeathEffectClass)
	{
		UE_LOG(LogP1, Warning, TEXT("[GAS] OnDiedEventReceived: ASC 또는 DeathEffectClass 없음 (%s)"), *GetName());
		return;
	}

	UE_LOG(LogP1, Log, TEXT("[GAS] OnDiedEventReceived — DeathEffectClass 적용, RespawnDelay=%.2f (%s)"), RespawnDelay, *GetName());

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DeathEffectClass, 1.0f, EffectContext);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_RespawnDelay, RespawnDelay);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AP1HeroCharacter::OnDeadTagChanged(FGameplayTag Tag, int32 NewCount)
{
	UE_LOG(LogP1, Log, TEXT("[GAS] OnDeadTagChanged — NewCount=%d (%s)"), NewCount, *GetName());

	if (NewCount > 0)
	{
		if (IsLocallyControlled())
		{
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				DisableInput(PC);
			}
		}

		if (DeathMontage)
		{
			if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
			{
				AnimInstance->Montage_Play(DeathMontage);
			}
		}
		else
		{
			// 몽타주가 없으면 착지 노티파이를 기다릴 수 없으므로 즉시 래그돌로 전환.
			StartRagdoll();
		}
		return;
	}

	// State.Dead 제거 = GE 자연 만료 = 리스폰 시점. 서버에서만 실제 리스폰을 수행한다.
	if (!HasAuthority())
	{
		return;
	}

	AController* OwningController = GetController();
	if (!OwningController)
	{
		UE_LOG(LogP1, Warning, TEXT("[GAS] OnDeadTagChanged: Controller 없음 — 리스폰 불가 (%s)"), *GetName());
		return;
	}

	// RestartPlayerAtPlayerStart()는 Controller->GetPawn()이 null일 때만 새 Pawn을 스폰한다 —
	// corpse(this)가 아직 Controller에 살아있는 상태로 RestartPlayer를 먼저 부르면 엔진이 새로
	// 스폰하지 않고 기존 corpse를 그대로 재빙의(Possess)했다가, 바로 뒤이은 Destroy()가 그 corpse를
	// 지워버려 결과적으로 Controller->Pawn이 null로 남는다(입력 시 "Pawn is null"로 나타남).
	// 반드시 corpse를 먼저 Destroy()해 Controller->Pawn을 비운 뒤에 RestartPlayer를 호출해야 한다.
	UWorld* World = GetWorld();
	Destroy();

	if (AGameModeBase* GameMode = World->GetAuthGameMode())
	{
		GameMode->RestartPlayer(OwningController);
	}
}

void AP1HeroCharacter::OnDeathImpactEventReceived(const FGameplayEventData* EventData)
{
	StartRagdoll();
}

void AP1HeroCharacter::OnStunTagChanged(FGameplayTag Tag, int32 NewCount)
{
	UE_LOG(LogP1, Log, TEXT("[CC] OnStunTagChanged — NewCount=%d (%s)"), NewCount, *GetName());

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !StunMontage)
	{
		return;
	}

	if (NewCount > 0)
	{
		// 기절 지속시간이 소스마다 달라(거리 비례 스케일 등) 몽타주 길이와 정확히 안 맞을 수 있으므로,
		// 몽타주 자체를 Loop로 만들어두고 태그가 사라질 때(NewCount==0) 정지시키는 방식으로 맞춘다.
		AnimInstance->Montage_Play(StunMontage);
	}
	else if (AnimInstance->Montage_IsPlaying(StunMontage))
	{
		AnimInstance->Montage_Stop(0.25f, StunMontage);
	}
}

void AP1HeroCharacter::StartRagdoll()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!IsValid(MeshComp))
	{
		return;
	}

	UE_LOG(LogP1, Log, TEXT("[GAS] StartRagdoll (%s)"), *GetName());

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->SetMovementMode(MOVE_None);

	// 엔진 기본 제공 "Ragdoll" 콜리전 프로파일(PhysicsBody, WorldStatic/Dynamic 블록·Pawn 무시) 사용.
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();
}

void AP1HeroCharacter::ApplyFlatRestoreEffect(TSubclassOf<UGameplayEffect> EffectClass, FGameplayTag SetByCallerTag, float Magnitude)
{
	UAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	if (!IsValid(ASC) || !EffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, Magnitude);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AP1HeroCharacter::ApplyBaseStatsForLevel(int32 Level, bool bFullHeal)
{
	if (!HasAuthority() || !DefaultAttributesEffect)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	AP1PlayerState* P1PS = GetPlayerState<AP1PlayerState>();
	UP1AttributeSet* AttrSet = P1PS ? P1PS->GetAttributeSet() : nullptr;
	if (!IsValid(ASC) || !AttrSet)
	{
		return;
	}

	const float OldMaxHealth = AttrSet->GetMaxHealth();
	const float OldMaxMana = AttrSet->GetMaxMana();
	const float OldHealth = AttrSet->GetHealth();
	const float OldMana = AttrSet->GetMana();

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	// 레벨을 스펙 레벨로 넘겨 DefaultAttributesEffect의 Scalable Float 커브가 이 레벨 기준으로 평가되게 한다.
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DefaultAttributesEffect, static_cast<float>(Level), EffectContext);
	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	const float NewMaxHealth = AttrSet->GetMaxHealth();
	const float NewMaxMana = AttrSet->GetMaxMana();

	float HealAmount;
	float ManaAmount;
	if (bFullHeal)
	{
		// 스폰/리스폰 — 완전 회복.
		HealAmount = FMath::Max(0.0f, NewMaxHealth - OldHealth);
		ManaAmount = FMath::Max(0.0f, NewMaxMana - OldMana);
	}
	else
	{
		// 레벨업 — 최대치가 늘어난 만큼만 현재치도 늘어난다(공짜 완전회복 아님).
		HealAmount = FMath::Max(0.0f, NewMaxHealth - OldMaxHealth);
		ManaAmount = FMath::Max(0.0f, NewMaxMana - OldMaxMana);
	}

	if (HealAmount > 0.0f)
	{
		ApplyFlatRestoreEffect(HealEffectClass, TAG_Data_Heal_Flat, HealAmount);
	}
	if (ManaAmount > 0.0f)
	{
		ApplyFlatRestoreEffect(ManaRestoreEffectClass, TAG_Data_Mana_Flat, ManaAmount);
	}

	UE_LOG(LogP1, Log, TEXT("[Progression] ApplyBaseStatsForLevel(Level=%d, FullHeal=%d) — MaxHealth %.0f→%.0f MaxMana %.0f→%.0f Heal=%.0f ManaRestore=%.0f (%s)"),
		Level, bFullHeal ? 1 : 0, OldMaxHealth, NewMaxHealth, OldMaxMana, NewMaxMana, HealAmount, ManaAmount, *GetName());
}

float AP1HeroCharacter::GetChampionXPReward(UCurveTable* Table, int32 VictimLevel) const
{
	if (!Table)
	{
		return 0.0f;
	}

	const TMap<FName, FRealCurve*>& RowMap = Table->GetRowMap();
	if (RowMap.Num() == 0)
	{
		return 0.0f;
	}

	const FRealCurve* Curve = RowMap.CreateConstIterator().Value();
	return Curve ? Curve->Eval(static_cast<float>(VictimLevel)) : 0.0f;
}

void AP1HeroCharacter::GrantKillReward(int32 VictimKillStreak, float VictimTimeSinceLastDeath, int32 VictimLevel)
{
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	if (!IsValid(ASC))
	{
		return;
	}

	if (GoldRewardKillEffectClass)
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GoldRewardKillEffectClass, 1.0f, EffectContext);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_KillStreak, static_cast<float>(VictimKillStreak));
			SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_TimeSinceLastDeath, VictimTimeSinceLastDeath);
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	const float KillExperienceAmount = GetChampionXPReward(ChampionKillXPTable, VictimLevel);
	ApplyFlatRestoreEffect(ExperienceRewardEffectClass, TAG_Data_Experience_Flat, KillExperienceAmount);

	UE_LOG(LogP1, Log, TEXT("[Reward] GrantKillReward — %s (피해자 레벨=%d 연속킬=%d 생존시간=%.1f초 XP=%.0f)"),
		*GetName(), VictimLevel, VictimKillStreak, VictimTimeSinceLastDeath, KillExperienceAmount);
}

void AP1HeroCharacter::GrantAssistReward(int32 VictimLevel)
{
	if (!HasAuthority())
	{
		return;
	}

	const float AssistExperienceAmount = GetChampionXPReward(ChampionAssistXPTable, VictimLevel);
	ApplyFlatRestoreEffect(GoldRewardFlatEffectClass, TAG_Data_Gold_Flat, AssistGoldAmount);
	ApplyFlatRestoreEffect(ExperienceRewardEffectClass, TAG_Data_Experience_Flat, AssistExperienceAmount);

	UE_LOG(LogP1, Log, TEXT("[Reward] GrantAssistReward — %s (피해자 레벨=%d XP=%.0f)"), *GetName(), VictimLevel, AssistExperienceAmount);
}

void AP1HeroCharacter::CheckLevelUp()
{
	if (!HasAuthority())
	{
		return;
	}

	AP1PlayerState* P1PS = GetPlayerState<AP1PlayerState>();
	UP1AttributeSet* AttrSet = P1PS ? P1PS->GetAttributeSet() : nullptr;
	if (!P1PS || !AttrSet)
	{
		return;
	}

	// 한 번에 여러 레벨이 오를 수도 있으니(대량 경험치 획득 등) while로 반복 처리.
	float XPRequired = P1PS->GetXPRequiredForNextLevel();
	while (XPRequired > 0.0f && AttrSet->GetExperience() >= XPRequired)
	{
		AttrSet->SetExperience(AttrSet->GetExperience() - XPRequired);
		P1PS->SetCharacterLevel(P1PS->GetCharacterLevel() + 1);
		P1PS->AddSkillPoint();

		UE_LOG(LogP1, Log, TEXT("[Progression] 레벨업! %s → Level %d (SkillPoints=%d)"),
			*GetName(), P1PS->GetCharacterLevel(), P1PS->GetSkillPoints());

		ApplyBaseStatsForLevel(P1PS->GetCharacterLevel(), /*bFullHeal=*/false);

		XPRequired = P1PS->GetXPRequiredForNextLevel();
	}
}
