// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/P1HeroCharacter.h"
#include "Player/P1PlayerState.h"
#include "Player/P1PlayerController.h"
#include "AbilitySystem/P1AttributeSet.h"
#include "AbilitySystem/P1GameplayAbility.h"
#include "AbilitySystem/P1AbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Camera/P1CameraComponent.h"
#include "Characters/P1HeroComponent.h"
#include "Characters/P1BuffCosmeticEffectComponent.h"
#include "MotionWarpingComponent.h"
#include "Components/WidgetComponent.h"
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

	InitAbilityActorInfo();
	AddDefaultAbilities();
}

void AP1HeroCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilityActorInfo();
}

void AP1HeroCharacter::InitAbilityActorInfo()
{
	UE_LOG(LogP1, Log, TEXT("[GAS] InitAbilityActorInfo called on %s | Authority=%d"), *GetName(), HasAuthority());

	AP1PlayerState* P1PS = GetPlayerState<AP1PlayerState>();
	if (!IsValid(P1PS))
	{
		UE_LOG(LogP1, Warning, TEXT("[GAS] InitAbilityActorInfo: PlayerState is null on %s"), *GetName());
		return;
	}

	UAbilitySystemComponent* ASC = P1PS->GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		UE_LOG(LogP1, Warning, TEXT("[GAS] InitAbilityActorInfo: ASC is null on %s"), *GetName());
		return;
	}

	ASC->InitAbilityActorInfo(P1PS, this);
	CachedAbilitySystemComponent = ASC;

	// 버프 태그에 반응하는 코스메틱 이펙트(검 발광 등)가 있는 영웅이면(컴포넌트가 붙어있으면) ASC를 넘겨준다.
	// 어떤 태그/이펙트인지는 전부 컴포넌트 쪽 데이터가 갖고 있어 이 클래스는 특정 스킬을 몰라도 된다.
	if (UP1BuffCosmeticEffectComponent* CosmeticComp = FindComponentByClass<UP1BuffCosmeticEffectComponent>())
	{
		CosmeticComp->BindToAbilitySystemComponent(ASC);
	}

	if (HasAuthority() && DefaultAttributesEffect)
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DefaultAttributesEffect, 1.0f, EffectContext);
		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
	else if (!DefaultAttributesEffect)
	{
		UE_LOG(LogP1, Warning, TEXT("[GAS] InitAbilityActorInfo: DefaultAttributesEffect is not set on %s"), *GetName());
	}

	BindMoveSpeedAttribute();

	// 머리 위 위젯 — per-character FloatingStatusWidgetController 생성·바인딩.
	if (IsValid(FloatingStatusComponent))
	{
		if (UP1FloatingStatusWidget* FloatingWidget =
			Cast<UP1FloatingStatusWidget>(FloatingStatusComponent->GetUserWidgetObject()))
		{
			if (!FloatingStatusWidgetController)
			{
				AP1PlayerState* PS = GetPlayerState<AP1PlayerState>();
				const FWidgetControllerParams Params(nullptr, PS, ASC, PS ? PS->GetAttributeSet() : nullptr);
				FloatingStatusWidgetController = NewObject<UP1FloatingStatusWidgetController>(this);
				FloatingStatusWidgetController->SetWidgetControllerParams(Params);
				FloatingStatusWidgetController->BindCallbacksToDependencies();
			}
			FloatingWidget->SetWidgetController(FloatingStatusWidgetController);
			FloatingStatusWidgetController->BroadcastInitialValues();
		}
	}

	// 소유 클라이언트(리슨서버 포함)에서만 메인 HUD 생성.
	if (IsLocallyControlled())
	{
		if (AP1PlayerController* PC = Cast<AP1PlayerController>(GetController()))
		{
			PC->CreateHUDForASC(ASC);
		}
	}
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
