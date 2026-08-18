// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayTagContainer.h"
#include "P1CharacterBase.generated.h"

class UAbilitySystemComponent;
class UP1FloatingWidgetComponent;
class UP1FloatingStatusWidget;
class UP1FloatingStatusWidgetController;
class UMaterialInterface;
class UParticleSystem;
class UParticleSystemComponent;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS(Abstract)
class P1_API AP1CharacterBase : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AP1CharacterBase();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return CachedAbilitySystemComponent; }

	static bool IsSameTeam(const AActor* A, const AActor* B);

	// 캐릭터 유형 태그 반환 (Character.Type.*).
	FGameplayTag GetCharacterType() const { return CharacterType; }

	// 영웅인지 — RMB "영웅 적중 시" 조건 등에서 사용.
	bool IsHero() const;

	// IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(TeamId); }

	// 코스메틱 효과를 모든 클라이언트(시뮬레이티드 프록시 포함)에 복제하는 범용 Multicast.
	// GameplayCue의 폴더 스캔+태그매칭 대신, 어빌리티가 머티리얼/파티클을 직접 프로퍼티로 들고
	// 이 함수를 호출하는 방식 — GA에서 바로 설정 가능하고 별도 Cue Notify 에셋이 필요 없다.
	// 반드시 서버(권위)에서 호출할 것 — Multicast RPC는 클라에서 호출해도 복제되지 않는다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetMaterialOverride(FName SlotName, UMaterialInterface* OverrideMaterial);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayParticleEffect(UParticleSystem* ParticleTemplate, FName SocketName);

	// 위와 동일한 1회성/소켓부착 이펙트지만 Niagara용 — 레거시 Cascade(UParticleSystem)와 현대
	// Niagara(UNiagaraSystem)는 타입 자체가 다른 별개 시스템이라 같은 함수로 겸용할 수 없다. 신규 이펙트는
	// 가능하면 이쪽을 사용(사면(아이템) "용기"/증강(아이템) "진실의 일격"이 첫 사용처).
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayNiagaraEffect(UNiagaraSystem* NiagaraTemplate, FName SocketName);

	// 위와 동일한 1회성 이펙트지만, 소켓이 아니라 캐릭터와 무관한 임의의 월드 좌표에 재생한다 — 예를 들어
	// 캐릭터 주변이 아니라 스킬 궤적을 따라 이동하는 지점(지면 블래스트 등)에 재생해야 하는 경우.
	// Scale은 파티클 에셋 자체를 수정하지 않고 호출부(어빌리티 등)에서 크기를 조절하기 위함 — 기본값 (1,1,1).
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayParticleEffectAtLocation(UParticleSystem* ParticleTemplate, FVector Location, FRotator Rotation, FVector Scale);

	// 위 MulticastPlayParticleEffect와 달리 자동 파괴되지 않고 소켓에 계속 붙어있는 지속 이펙트를
	// 시작한다 (예: 버프 지속 동안 유지되는 무기 궤적) — 반환값 없이 내부에서 컴포넌트를 보관하고,
	// 이미 재생 중인 게 있으면 먼저 정리한 뒤 새로 시작한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetAttachedParticleEffect(UParticleSystem* ParticleTemplate, FName SocketName);

	// MulticastSetAttachedParticleEffect로 시작한 지속 이펙트를 중지(파괴)한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopAttachedParticleEffect();

	// 위 MulticastSetAttachedParticleEffect의 Niagara판 — 다만 슬롯 하나만 관리하는 원본과 달리
	// SocketName을 키로 삼아 여러 소켓에 동시에 독립적인 지속 이펙트를 유지할 수 있다(증강(아이템)
	// "진실의 일격"처럼 양손에 동시에 다른/같은 이펙트를 유지해야 하는 경우 대비). 같은 소켓에 이미
	// 재생 중인 게 있으면 먼저 정리한 뒤 새로 시작한다. bAutoDestroy=false로 스폰하므로, 이펙트
	// 에셋 자체가 무한 루프(Looping)로 만들어져 있어도 MulticastStopAttachedNiagaraEffect를
	// 호출하기 전까지 확실히 유지되고, 호출하면 확실히 멈춘다 — 1회성 MulticastPlayNiagaraEffect를
	// 지속 버프의 비주얼로 쓰면 루프 에셋이 "완료" 상태에 도달하지 못해 영원히 안 사라지는 문제가
	// 생기므로, 버프 생명주기에 종속된 이펙트는 반드시 이쪽을 사용할 것.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetAttachedNiagaraEffect(UNiagaraSystem* NiagaraTemplate, FName SocketName);

	// MulticastSetAttachedNiagaraEffect로 시작한 지속 이펙트를 SocketName으로 찾아 중지(파괴)한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopAttachedNiagaraEffect(FName SocketName);

	// MulticastSetAttachedParticleEffect의 "소켓 부착" 대신 "고정 월드 좌표"에 지속 이펙트를 시작한다 —
	// 캐릭터를 따라다니지 않고 확정된 지면 지점에 계속 남아있어야 하는 이펙트(예: Ion Strike 화살비)용.
	// bAutoDestroy=false로 스폰하므로, 이펙트 에셋 자체가 무한 루프여도 MulticastStopPersistentParticleEffectAtLocation을
	// 호출하기 전까지 확실히 유지되고, 호출하면 확실히 멈춘다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetPersistentParticleEffectAtLocation(UParticleSystem* ParticleTemplate, FVector Location, FRotator Rotation, FVector Scale);

	// MulticastSetPersistentParticleEffectAtLocation으로 시작한 지속 이펙트를 중지(파괴)한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopPersistentParticleEffectAtLocation();

	// 캐릭터/소켓과 무관하게 StartLocation에서 스폰해 Duration 동안 EndLocation까지 선형 이동시키는
	// 1회성 이펙트(예: 하늘을 가로지르는 드론/빔). Start/End/Duration이 이 호출 하나로 결정적으로 정해지므로,
	// 매 프레임 위치를 리플리케이트할 필요 없이 각 클라이언트가 동일한 파라미터로 독립적으로 로컬 재생한다
	// (호출 자체는 서버 권위 지점에서만, 이후 이동은 순수 로컬 타이머).
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayMovingParticleEffect(UParticleSystem* ParticleTemplate, FVector StartLocation, FVector EndLocation, float Duration);

	// 디버그 전용 — DrawDebugSphere/Line은 호출한 프로세스에서만 보이고 리플리케이트되지 않는다.
	// 서버 권위 어빌리티 로직(OnBombExplode 등)의 판정 지점에서 그대로 DrawDebugSphere를 부르면
	// 서버 창에서만 보이고 정작 사용자가 보고 있는 클라이언트 화면엔 아무것도 안 뜬다 — bShowDebug류
	// 판정 시각화를 모든 클라이언트 화면에서 확인하려면 이 Multicast를 거쳐야 한다.
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastDrawDebugSphere(FVector Location, float Radius, FColor Color, float Duration);

protected:
	// 팀 식별자. 0=팀1, 1=팀2, 255=NoTeam. 각 캐릭터 BP에서 설정.
	// 추후 GameMode가 서버에서 할당하는 방식으로 교체 예정.
	UPROPERTY(EditAnywhere, Category = "Team")
	uint8 TeamId = 255;

	// 캐릭터 유형. 생성자에서 Character.Type.Hero로 기본 설정.
	// 미니언/보스 BP에서 각각 Minion/Boss로 변경.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	FGameplayTag CharacterType;

	// ASC의 원본은 AP1HeroCharacter는 PlayerState, AP1MinionCharacter는 Pawn 자신에 있음.
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;

	// 머리 위 HP/MP 바. BP에서 WidgetClass를 WBP_FloatingStatus로 설정.
	// Space=Screen이라 3D 메시가 아니라 화면에 직접 그려지는 2D 오버레이라서, 캐릭터가 회전해도
	// 절대 옆모습(안 보임)이 되지 않는다 — 3D 앵커 위치만 매 프레임 화면 좌표로 재투영될 뿐.
	// UP1FloatingWidgetComponent라 카메라 거리에 비례해 크기도 함께 스케일된다.
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UP1FloatingWidgetComponent> FloatingStatusComponent;

	// FloatingStatusWidget에 데이터를 공급하는 per-character 컨트롤러.
	UPROPERTY()
	TObjectPtr<UP1FloatingStatusWidgetController> FloatingStatusWidgetController;

	// MulticastSetAttachedParticleEffect로 시작한 지속 이펙트 인스턴스 — 중지 시 여기서 찾아 파괴한다.
	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> AttachedParticleEffectComponent;

	// MulticastSetPersistentParticleEffectAtLocation으로 시작한 지속 이펙트 인스턴스 — 캐릭터에 붙는 게
	// 아니라 고정 월드 좌표에 있으므로 AttachedParticleEffectComponent와 별도 슬롯으로 관리(둘이 동시에
	// 활성화될 수 있음, 예: Q 회오리를 두르고 있는 동안 R 화살비도 같이 떨어지는 경우).
	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> PersistentLocationParticleEffectComponent;

	// MulticastSetAttachedNiagaraEffect로 시작한 지속 이펙트 인스턴스들 — 소켓 이름을 키로 관리해
	// 여러 소켓(예: 양손)에 동시에 독립적으로 유지/중지할 수 있다.
	UPROPERTY()
	TMap<FName, TObjectPtr<UNiagaraComponent>> AttachedNiagaraEffectComponents;
};
