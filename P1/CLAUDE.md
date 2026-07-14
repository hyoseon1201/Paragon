# CLAUDE.md

이 파일은 **P1** 언리얼 엔진 프로젝트에서 작업할 때 Claude Code가 참고하는 가이드입니다.

## 엔진 / 프로젝트 정보

- 엔진 버전: **5.8** (`P1.uproject` 참고)
- 메인 모듈: `P1` (Runtime, Default 로딩 페이즈)
- 빌드 파일: `Source/P1/P1.Build.cs`, `Source/P1.Target.cs`, `Source/P1Editor.Target.cs`
- 활성화된 플러그인: ModelingToolsEditorMode, ModelContextProtocol, Terminal, EditorToolset, GameplayAbilities, ModularGameplay, **MotionWarping** (설치돼 있으나 현재 미사용 — RMB는 결국 `ApplyRootMotionJumpForce`로 구현. `AP1HeroCharacter::MotionWarpingComponent`는 향후 다른 스킬에서 재사용 가능)

## 언리얼 C++ 컨벤션

- Epic의 [Unreal Engine Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)를 따릅니다.
- 네이밍: 구조체/일반 클래스는 `F`, UObject 파생은 `U`, Actor 파생은 `A`, 인터페이스는 `I`, enum은 `E`, 템플릿은 `T`. bool 변수는 `b` 접두사 사용 (예: `bIsActive`).
- 이 프로젝트에서 직접 만드는 모든 게임플레이 클래스(Character, PlayerState, PlayerController, GameMode, GameState, AttributeSet, AbilitySystemComponent 등)는 위 UE 타입 접두사(`A`/`U`/...) 뒤에 프로젝트 접두사 `P1`을 붙입니다 (예: `AP1HeroCharacter`, `UP1AttributeSet`, `AP1GameMode`). 엔진/플러그인 타입을 그대로 쓰는 경우(`ACharacter`, `UAttributeSet` 등 서드파티 상속 없이 직접 인스턴스화하는 경우)는 제외합니다.
- 서드파티/비-UE 코드와 인터페이스하는 경우가 아니라면 STL보다 언리얼 자체 타입을 우선 사용: `TArray`, `TMap`, `TSet`, `FString`/`FName`/`FText`, `TOptional`, `TSharedPtr`/`TWeakPtr`/`TUniquePtr` 등.
- `UPROPERTY()` / `UFUNCTION()` 매크로에 올바른 스펙시파이어(`BlueprintReadOnly`, `EditAnywhere`, `VisibleAnywhere`, `BlueprintCallable` 등)를 사용. `UObject*` 멤버를 `UPROPERTY()` 없이 raw 포인터로 두지 말 것 — 가비지 컬렉터가 추적하지 못함.
- UObject 포인터 멤버는 raw `T*`보다 `TObjectPtr<T>`를 우선 사용 (UE5 기본값).
- 헤더/소스 분리: 선언은 `.h`, 구현은 `.cpp`. 헤더는 최소화 — 가능하면 `#include` 대신 전방 선언(forward declare) 사용.
- 모든 UCLASS/USTRUCT/UINTERFACE 선언에는 `GENERATED_BODY()`를 사용.
- 로깅은 커스텀 `DECLARE_LOG_CATEGORY_EXTERN` 카테고리를 통해서 하고, 실제 출시 코드 경로에서는 `printf`/`UE_LOG(LogTemp, ...)`를 직접 쓰지 않음.
- 사용 전 포인터 null 체크. `UObject*`는 raw null 체크보다 `IsValid()`를 우선 사용 (pending-kill 상태도 올바르게 처리됨).
- 게임 스레드를 블로킹하지 말 것 — 무거운 작업은 언리얼의 비동기/태스크 시스템(`AsyncTask`, `FRunnable`, Tasks System) 사용.
- 메모리: UObject에 수동 `new`/`delete`를 쓰지 말고 `NewObject<T>()`, `CreateDefaultSubobject<T>()`(생성자 내에서), `SpawnActor<T>()`를 사용.
- 블루프린트에 노출된 API를 수정할 때는 기존 블루프린트/에셋이 의존할 수 있으므로 하위 호환성을 유지.

## 빌드 & 이터레이션

- 이 프로젝트는 Binaries/Intermediate/Saved/DerivedDataCache를 커밋하지 **않습니다** — 빌드 산출물입니다 (`.gitignore` 참고).
- C++ 변경 사항은 보통 에디터에서 테스트하기 전에 IDE나 `UnrealBuildTool`을 통한 재컴파일이 필요합니다. 핫 리로드는 많은 경우 동작하지만 전부는 아닙니다 (새 UCLASS/USTRUCT 추가는 보통 에디터 재시작 필요).
- 빌드는 `Automation_P1.sln`, `P1.sln` 솔루션을 사용. Source 파일이 추가/삭제되면 `.uproject` 우클릭 → "Generate Visual Studio project files"로 프로젝트 파일을 재생성할 수 있습니다.

## 워크플로우 노트

- `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`는 커밋하지 않음 — 로컬/생성 파일입니다.
- 새 C++ 클래스를 추가할 때는 `Source/P1/` 하위의 기존 모듈 구조를 따릅니다.
- **MCP 도구 사용 시 허가 필수:** `mcp__unreal-mcp__*` 도구들(에디터 에셋/오브젝트 조작)은 토큰 사용량이 많으므로, **먼저 사용자의 명시적 허가를 받은 후에만** 실행합니다. 입력 에셋 생성, 블루프린트 구성, 프로퍼티 설정 등을 MCP로 수행하려면 반드시 "이 작업을 MCP로 진행해도 될까요?" 같은 확인 후 진행하십시오.

## GAS (Gameplay Ability System) 컨벤션

GAS는 어빌리티/스탯 시스템의 핵심 토대로 사용할 예정이므로, 부가 기능이 아니라 핵심 아키텍처로 취급합니다:

- 리스폰 캐릭터의 경우, `UAbilitySystemComponent`는 보통 Pawn이 아니라 PlayerState에 두어 Pawn 파괴/소유권 변경에도 어빌리티 상태가 유지되도록 합니다.
- 게임플레이 속성(Attribute)은 `UAttributeSet` 서브클래스로 정의하고, getter/setter는 `GAMEPLAYATTRIBUTE_*` 매크로를 사용하며 `PreAttributeChange`/`PostGameplayEffectExecute`로 클램핑합니다.
- 어빌리티는 `UGameplayAbility` 서브클래스로 구현하고 `GiveAbility`/어빌리티 셋으로 부여합니다. 비용/쿨다운은 직접 타이머를 만들지 말고 `GameplayEffect`로 처리합니다.
- 어빌리티 식별, 차단/취소, 상태 쿼리에는 enum이나 문자열 비교 대신 `FGameplayTag`(`GameplayTagsManager`/`UGameplayTagsManager`)를 사용합니다.
- 모든 어빌리티 발동은 리플리케이션을 고려해야 합니다: 예측 키(prediction keys), 예측되지 않는 효과에는 `ServerRPC`/`ClientRPC`, 게임플레이 상태를 직접 손으로 리플리케이트하지 말고 `ReplicatedGameplayEffects`를 사용합니다.
- **어빌리티 부여(GiveAbility)는 `PossessedBy`에서 호출하는 `AddDefaultAbilities()`에서만 수행한다.** `InitAbilityActorInfo()`는 ASC 참조 초기화만 담당하며 능력 부여 코드를 포함하지 않는다 — 이를 섞으면 `OnRep_PlayerState`와 `PossessedBy` 양쪽에서 호출될 때 중복 부여가 발생한다.
- **`LocalPredicted` 어빌리티의 데미지 판정은 반드시 `CurrentActorInfo->IsNetAuthority()` 체크로 서버에서만 실행한다.** 클라이언트 예측 인스턴스에서도 AnimNotify가 발화되므로 체크 없이는 RTT만큼 늦게 중복 판정된다.
- **홀드 콤보는 `Spec->InputPressed`가 아니라 자체 `bInputHeld` 플래그로 판단한다.** 단, `bInputHeld`가 서버에서 갱신되려면 커스텀 입력 라우팅이 `ServerSetInputPressed/Released()`로 입력을 서버에 복제해야 하고, 어빌리티는 `bReplicateInputDirectly=true`여야 한다. 이게 없으면 서버 `InputReleased()`가 호출되지 않아 콤보가 한 타 더 나간다.
- **지면 조준(ground-targeted) 스킬 패턴**: `UAbilityTask_WaitTargetData(UserConfirmed)` + 커스텀 `AGameplayAbilityTargetActor` 서브클래스. 조준 확정 전까지는 `CommitAbility`를 호출하지 않아 코스트/쿨다운이 안 나가게 하고, 확정 시점에만 커밋한다. 조준 중에는 `State.TargetingAbility` 같은 루즈 태그를 부여해 ASC 입력 라우팅에서 다른 어빌리티 입력을 차단하고 LMB/RMB를 confirm/cancel로 리라우팅한다.
- **루트모션 도약(JumpForce)의 Duration은 몽타주 "전체 길이"가 아니라 "착지 판정 AnimNotify가 실제로 찍힌 시점"에 맞춘다.** `UAbilityTask_ApplyRootMotionJumpForce`의 Duration을 몽타주 총 길이로 주면, 착지 노티파이가 타임라인 중간(예: 60%)에 있을 경우 노티파이가 발동하는 순간 캐릭터가 아직 목표 지점에 도달하지 못한 채 공중에 뜬 상태가 된다. 몽타주의 `Notifies` 배열을 순회해 해당 노티파이의 `GetTriggerTime()`을 Duration으로 써야 물리적 착지와 애니메이션/데미지 판정이 정확히 일치한다.
- **같은 슬롯/겹치는 몽타주가 서로 인터럽트하면 진행 중이던 루트모션 태스크가 조기 종료되어 잔여 속도로 캐릭터가 통제 불가능하게 날아갈 수 있다.** (`ApplyRootMotionJumpForce`가 `VelocityOnFinishMode=MaintainLastRootMotionVelocity`인 상태에서 중간에 강제 종료되면 그 순간 속도가 일반 물리로 그대로 넘어감.) 루트모션 기반 이동 스킬이 활성 중일 때는 다른 어빌리티(특히 몽타주를 쓰는 기본공격)가 끼어들지 못하도록 `ActivationOwnedTags`로 차단 태그(`State.Attacking` 재사용 등)를 부여해야 한다.
- **"다음 기본공격 강화" 류 버프 어빌리티는 DamageGameplayAbility가 아니라 베이스 `UP1GameplayAbility`를 상속한다.** 버프 자체는 데미지를 입히지 않고 GE 하나(지속시간+태그 부여, 필요시 어트리뷰트 모디파이어)를 자신에게 적용하고 끝나며, 실제 강화된 효과는 그 태그를 체크하는 **기본공격(MeleeAttack) 쪽 코드**에서 처리한다. 사거리 증가처럼 기존에 이미 읽고 있는 어트리뷰트를 버프 GE가 잠깐 올려두는 방식이면 소비 어빌리티 쪽 코드 수정이 아예 필요 없다. 버프는 적중 판정 직후 `RemoveActiveEffectsWithGrantedTags()`로 소모(제거)해 "다음 1회만" 적용되게 한다.
- **"어빌리티가 자신에게 GE를 적용" 패턴(버프/디버프/쿨다운 등)은 전용 서브클래스가 아니라 베이스 `UP1GameplayAbility::ApplyEffectToSelf()`로 처리한다.** Damage처럼 여러 어빌리티가 공유하는 계수 프로퍼티가 있는 게 아니라 어빌리티마다 고유한 GE 하나를 참조하는 것뿐이라, `DamageGameplayAbility`식 서브클래스 계층을 만들 필요는 없다 — `MakeOutgoingGameplayEffectSpec`+`ApplyGameplayEffectSpecToOwner`(예측 키 컨텍스트를 올바르게 태움, raw `ASC->ApplyGameplayEffectSpecToSelf`보다 정석적)를 감싼 헬퍼 하나로 충분.
- **"어빌리티 레벨에 따라 값이 달라져야 하는" 수치(Flat 데미지, 커스텀 쿨다운 지속시간 등)는 `float` 대신 `FScalableFloat`(`ScalableFloat.h`)로 선언하고 `GetValueAtLevel(GetAbilityLevel())`로 평가한다.** `FScalableFloat`는 `Value * CurveTable[Level]` 형태로, Curve를 지정하지 않으면 `Value`를 그대로 반환하므로(`IsStatic()`) 기존처럼 고정값으로도 동작한다 — 기존 `float` 프로퍼티를 이 타입으로 바꿔도 `SerializeFromMismatchedTag`가 자동으로 값을 이관해 이미 에디터에서 값을 넣어둔 BP 자산이 깨지지 않는다. 단, **코스트(Cost)는 이 프로젝트에서 SetByCaller를 거치지 않고 `CommitAbilityCost()`가 어빌리티 고유의 `CostGameplayEffectClass`(엔진 내장 프로퍼티)를 그대로 커밋하므로, 코스트 GE 에셋의 Modifier Magnitude를 "Scalable Float"로 설정하면 C++ 변경 없이도 이미 레벨별로 값이 달라진다** — `MakeOutgoingGameplayEffectSpec`이 `GetAbilityLevel()`을 스펙 레벨로 넘기기 때문. 반대로 쿨다운처럼 `Data.CooldownDuration` SetByCaller로 C++이 직접 값을 주입하는 경우(감소 재적용 등 런타임 조정이 필요해서 GE 자체의 Scalable Float 대신 이 방식을 씀, `AssaultTheGates::BaseCooldown` 등)는 GE의 커브 메커니즘을 우회하므로 C++ 프로퍼티 자체를 `FScalableFloat`로 관리해야 레벨 스케일링이 적용된다.
- **버프로 얹는 추가 데미지는 `ApplyDamageToTarget()`의 Bonus 파라미터로 전달한다.** 어빌리티 클래스의 `FlatDamage`/`PhysicalPowerCoefficient`/`MagicalPowerCoefficient`/`TargetMaxHealthPctCoefficient`/`SourceMaxHealthPctCoefficient`는 그 어빌리티 자체의 평소 계수이므로, "평소엔 0, 버프 활성 시에만 추가" 패턴은 호출 시점에 Bonus 인자를 얹어 해결한다 (클래스 프로퍼티를 런타임에 직접 수정하지 않음).
- **"캐릭터를 따라다니며 지속적으로 주변을 판정하는" 스킬(Q의 회오리 등)은 표준 Periodic GameplayEffect가 아니라 어빌리티가 직접 소유하는 반복 타이머로 구현한다.** Periodic GE는 캐스트 시점에 고정된 대상에게만 반복 적용되므로, "캐릭터가 이동하며 매 틱 주변을 새로 스캔"하는 요구를 표현할 수 없다. 이런 스킬도 데미지 채널은 그대로 공유하므로 `UP1DamageGameplayAbility`는 상속하되, 전용 중간 상속 클래스는 실제로 2개 이상의 스킬에서 동일한 형태(반복 스캔+효과)가 확인되기 전까지는 만들지 않는다 (버프 헬퍼를 뽑을 때와 같은 원칙 — 추측성 추상화 금지).
- **지속시간 종료 후에야 쿨다운이 시작되는 스킬은 코스트/쿨다운 커밋을 분리한다.** 캐스트 시점엔 `CommitAbilityCost()`만 호출하고, 지속시간이 끝나는 시점(마지막 틱 등)에 `ApplyEffectToSelf(CooldownGE)`로 쿨다운만 따로 적용한다. 어빌리티가 중간에 취소/중단되어도 코스트는 이미 나갔으므로 `EndAbility` 오버라이드에서 쿨다운 미적용 시 폴백으로 적용해준다 (플래그로 중복 적용 방지).
- **여러 데미지 어빌리티가 겹치는 "주변 오버랩+팀/높이 필터" 로직은 `UP1DamageGameplayAbility::GetEnemiesInRadius()` 공용 헬퍼로 처리한다.** 전방 반원처럼 방향성 필터가 추가로 필요하면(MeleeAttack) 헬퍼가 반환한 목록에 호출자가 덧씌운다 — 헬퍼 자체에 방향 필터를 넣지 않아 360도 판정(Make Way)과 반원 판정(MeleeAttack) 양쪽에 재사용 가능하다.
- **무적/스테이시스류(피격 완전 무시) 스킬은 GE로 `State.Invulnerable` 태그만 부여하면 끝난다 — 별도의 "데미지 취소" 로직을 어빌리티마다 만들지 않는다.** `UP1AttributeSet::PostGameplayEffectExecute`가 Damage 메타 어트리뷰트를 Health로 변환하기 직전 이 태그를 한 번 체크해 전 어빌리티 공용으로 처리하므로, 어떤 스킬이든 이 Duration GE 하나만 자신에게 적용하면 그 지속시간 동안 자동으로 무적이 된다(Stone Forged Soul의 스테이시스가 첫 사용처).
- **목표 지점이 없는 고정 왕복 이동(제자리에서 떴다가 같은 자리로 돌아오는 등)은 `ApplyRootMotionJumpForce` 같은 코드 기반 궤적 생성이 필요 없다 — 원본 애니메이션의 루트모션을 그대로 쓰는 게 더 간단하고 정확하다.** RMB(Assault The Gates)가 코드로 포물선을 만든 이유는 지면 조준으로 "매번 달라지는 가변 목표 지점"까지 도달해야 했기 때문이다. 목표 지점이 항상 "원래 서있던 자리"로 고정된 경우(R의 상승→하강 등)는 애니메이터가 구워둔 루트모션이 정확히 그 왕복을 표현하므로, 원본 소스 애니메이션에 실제 수직 이동이 담겨 있는지만 확인하면 된다 — 있으면 그대로 재생, 없으면(에셋에 흔한 문제) 코드 기반 이동을 다시 설계해야 한다. 공중에 뜬 동안은 중력이 루트모션과 싸우지 않도록 `MovementMode`를 `MOVE_Flying`으로 일시 전환했다가 착지 시점(AnimNotify)에 `MOVE_Walking`으로 되돌린다.
- **"맞았을 때 반응하는" 쿨다운 있는 리액티브 프록(디플렉트, 스펠실드 등)은 어빌리티가 아니라 `UP1AttributeSet::PostGameplayEffectExecute`에서 판정하고, 어빌리티는 그 판정 결과를 통보받아 쿨다운만 커밋하는 얇은 껍데기로 만든다.** 데미지를 실제로 취소할 수 있는 유일한 지점이 AttributeSet이기 때문에 판정 자체를 거기서 하는 게 자연스럽고(무적 판정과 같은 이유), "이 데미지가 어떤 어빌리티에서 왔는지"는 `Data.EffectSpec.CapturedSourceTags`로 확인한다(위 `ApplyDamageToTarget`의 Asset Tags 전달 항목 참고). "지금 사용 가능한지"는 네이티브 쿨다운 태그(`Cooldown.Ability.X`)의 부재로 판단하면 충분 — 새로 부여된 어빌리티는 쿨다운 태그 없이 시작하므로 별도의 "준비완료" 상태를 만들 필요가 없다. 다만 AttributeSet은 어빌리티 클래스를 참조하면 안 되므로(범용성 유지), "이 캐릭터가 애초에 이 어빌리티를 갖고 있는지"는 그 어빌리티가 `OnGiveAbility`에서 자기 Asset Tag를 루즈 태그로 ASC에 심어두는 것으로 판별하고, 발동(쿨다운 커밋)은 AttributeSet이 GameplayEvent를 보내 `AbilityTriggers`로 그 어빌리티를 트리거하는 방식으로 위임한다(Stoicism 디플렉트가 첫 사용처). 이런 반응형 패시브는 다른 어빌리티 사용 중에도 항상 발동해야 하므로 생성자에서 베이스가 공통으로 거는 `State.Attacking` 상호배타 규칙을 `RemoveTag`로 명시적으로 빼야 한다.
- **코스메틱(비-게임플레이) 이펙트를 다른 클라이언트(시뮬레이티드 프록시)에도 보이게 하려면 상황에 맞는 메커니즘을 고른다. GAS 어빌리티 C++ 코드에서 직접 `SetMaterial`/파티클 스폰을 하면 로컬 클라이언트+서버에서만 보이고 다른 플레이어 화면엔 안 보인다(LocalPredicted 어빌리티는 시뮬레이티드 프록시에서 실행되지 않음) — 반드시 아래 중 하나를 거쳐야 한다:**
  1. **AnimNotify(State)** — 몽타주의 고정된 타임라인 구간에 이펙트를 묶을 때 (예: 스윙 궤적, 착지 파티클). 몽타주가 리플리케이트되므로 모든 클라이언트가 재생.
  2. **`AP1CharacterBase`의 Multicast RPC** — 이펙트 수명이 GE의 활성~제거(조기 소모 포함)에 대응해야 하거나, 몽타주에 고정 배치할 수 없는 **조건부/가변 길이** 이펙트일 때. 머티리얼/파티클을 **직접 프로퍼티로 참조**하고 서버에서 이 RPC를 호출하면 끝 — 별도 Cue Notify 에셋이나 스캔 경로 등록이 필요 없어 이 프로젝트에서는 이 방식을 기본으로 채택. 두 갈래가 있다: (a) **1회성** — `MulticastPlayParticleEffect`, "쐈다가 알아서 끝남"에 적합(`SpawnEmitterAttached(..., bAutoDestroy=true)`); 매 스윙마다 다시 트리거해야 하는 이펙트(적중 여부 무관하게 반복 재생)도 이 방식을 매번 호출하는 식으로 처리한다(Sacred Oath 무기 궤적). (b) **지속(버프 생명주기, 상태 자체가 켜져있음을 표현)** — `MulticastSetMaterialOverride` 또는 `MulticastSetAttachedParticleEffect`+`MulticastStopAttachedParticleEffect`(파티클처럼 컴포넌트를 직접 붙였다 떼야 하는 경우, `bAutoDestroy=false`로 스폰 후 반환된 `UParticleSystemComponent*`를 `AttachedParticleEffectComponent`에 보관했다가 `DeactivateSystem()`+`DestroyComponent()`로 정리), "태그가 있는 동안 계속 유지되다 태그가 사라지면 꺼짐"에 적합(Sacred Oath 검 발광). **(a)와 (b)의 선택 기준**: 이펙트가 "그 순간 상태(버프 활성)를 계속 나타내야 하는가"(b) vs "특정 이벤트(스윙, 히트)가 발생할 때마다 다시 트리거되어야 하는가"(a) — 검 발광엔 (b), 무기 궤적엔 (a)를 쓴다(처음엔 궤적도 (b)로 만들었다가 "매 스윙마다 재생"이라는 실제 요구와 안 맞아 (a)로 되돌렸다). (b) 패턴은 어빌리티가 직접 호출하지 않고 아래 `UP1BuffCosmeticEffectComponent`가 담당한다.
  3. **GameplayCue** — `UP1GameplayCueNotify_MaterialOverride`/`_ParticleEffect`가 범용 유틸리티로 존재하지만, 폴더 스캔(`GameplayCueNotifyPaths`)+태그매칭 방식이 에디터 세션 중 갱신이 잘 안 되는 등 다루기 번거로워서 Sacred Oath는 결국 2번(Multicast)으로 전환했다. 팀 규모가 커져 어빌리티 코드와 이펙트 에셋을 완전히 분리해야 할 필요가 생기면 재검토.
- **"버프 상태에 따라 같은 입력이 다른 동작을 해야 하는" 경우("강화된 기본공격" 등)는 같은 InputTag에 두 어빌리티를 경쟁시키지 않고, 그 시너지가 있는 영웅이 아예 다른(상속) 어빌리티 클래스를 자신의 유일한 해당 슬롯 어빌리티로 등록한다.** 판정(버프 태그 유무)은 액티베이션 시점이 아니라 실제 히트/효과 적용 시점에 어빌리티 내부에서 한다 — 이 프로젝트는 처음에 `ActivationRequiredTags`/`ActivationBlockedTags`로 같은 InputTag에 평소 버전과 변종을 동시에 등록해 상호 배타적으로 게이팅하는 방식을 시도했으나, **액티베이션 시점의 태그 스냅샷은 `LocalPredicted` 어빌리티의 클라이언트 예측 GE가 서버 authoritative 버전과 조율(reconcile)되는 타이밍에 취약해서(예측된 버프 태그가 실제보다 훨씬 일찍 사라지는 것처럼 보이는 경우 발생) 폐기했다.** 또한 "먼저 시도되어 성공한 어빌리티가 `State.Attacking`을 걸어 같은 입력에 매칭되는 나머지 후보를 오염시킬 수 있다"는 구조적 위험도 있었다. 대신: 시너지가 없는 영웅은 베이스 클래스(`UP1GameplayAbility_MeleeAttack`)를, 있는 영웅은 서브클래스(`UP1GameplayAbility_MeleeAttack_SacredOath`)를 DefaultAbilities에 등록 — 한 영웅당 해당 슬롯 어빌리티는 항상 하나뿐이라 경쟁 자체가 없다. 콤보/몽타주처럼 복잡한 공유 로직은 베이스에 남기고, 서브클래스가 상속받아 재사용 — 데미지 계산처럼 실제로 달라지는 부분만 `protected virtual` 훅(`ApplyComboHitDamage` 등)으로 뽑아 오버라이드하고, 그 안에서 버프 태그를 체크해 없으면 `Super::`로 폴백한다. 베이스 어빌리티 클래스는 특정 버프의 존재를 전혀 몰라도 된다.
- **"버프 GE가 걸어둔 태그가 있는 동안 코스메틱 이펙트를 켜고, 사라지면(조기 소모든 자연 만료든 원인 무관하게) 되돌려야 하는" 경우는 타이머로 추측하지 말고 `ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)`로 태그 카운트 변경 이벤트를 직접 구독한다.** Sacred Oath 검 발광이 초기엔 "자연 만료 시점에 맞춘 타이머"(조기 소모 시 별도로 즉시 원복)라는 이중 경로였는데, 이건 정확히 "GE 자체 만료"와 "타이머 발화"가 거의 동시에 태그를 건드리면서 경쟁 상태를 만들었던 지점이다. 태그 이벤트 구독은 원인(조기 소모/자연 만료)과 무관하게 카운트가 0↔양수로 바뀌는 정확한 시점에 단 한 번만 반응하므로 이런 경쟁이 애초에 발생하지 않는다.
- **"버프 태그 하나에 반응해 코스메틱을 켜고 끄는" 로직은 어빌리티나 영웅 공용 캐릭터 클래스에 하드코딩하지 않고 `UP1BuffCosmeticEffectComponent`(범용 PawnComponent, 아래 항목 참고)에 데이터로 넣는다.** 처음엔 이 리스너를 `AP1HeroCharacter`(모든 영웅의 공용 베이스)에 직접 구현하고 태그/슬롯 이름을 그 클래스의 멤버로 하드코딩했었는데, 이러면 Sacred Oath 시너지가 없는 다른 영웅도 전혀 안 쓰는 프로퍼티/리스너를 상속받게 되어 `MeleeAttack`이 한때 겪었던 것과 같은 종류의 "공용 클래스 오염"이 캐릭터 레벨에서 재발한다. 컴포넌트로 옮기면 이 시너지가 필요한 영웅의 BP에만 컴포넌트를 추가하면 되고, `AP1HeroCharacter`는 "내가 이 컴포넌트를 갖고 있다면 ASC를 넘겨준다"는 범용 훅 한 줄(`FindComponentByClass`)만 알면 된다 — 특정 스킬 이름은 전혀 몰라도 됨.
- **AnimBP 슬롯은 몽타주 성격에 맞게 골라야 한다.** Layered Blend Per Bone으로 블렌딩되는 `UpperBody` 슬롯은 하체를 Locomotion State Machine에 맡기고 상체만 덮어씌운다 — 이동 중에도 자연스러운 기본공격류에 적합. 반면 도약/궁극기처럼 전신을 새로 정의해야 하는 스킬은 Locomotion+UpperBody 블렌드 이후에 위치한 **FullBody 전용 Slot**(Output Pose 직전, 전신을 100% 오버라이드)을 써야 한다. 잘못된 슬롯을 쓰면 GAS/AnimInstance 레벨에서는 몽타주가 정상 재생(로그·노티파이 전부 발동)되는데도 AnimGraph에 연결이 안 돼 있어 화면에는 전혀 반영되지 않는다 — 재생 여부는 `AnimInstance::Montage_IsPlaying`/`Montage_GetPosition`으로 검증하고, 안 보이면 슬롯 배선부터 의심할 것.

## UI 아키텍처 컨벤션

- **역할 분리**: C++는 델리게이트 바인딩·위젯 컨트롤러 연결·데이터 계산 로직만 담당. 색상·폰트·레이아웃·애니메이션은 WBP 디자이너에서 설정한다.
- **계층 구조**: `UP1UserWidget` → 각 위젯 C++ 클래스 → WBP (Blueprint). WBP의 이벤트 그래프는 최소화하고 C++에서 제어한다.
- **위젯 컨트롤러 패턴 (D1/Aura 방식)**: `AP1HUD`가 화면별 컨트롤러를 레지스트리로 보유. `FWidgetControllerParams`(PC, PS, ASC, AS)로 초기화. ASC 델리게이트 → 컨트롤러 브로드캐스트 → 위젯 업데이트.
- **FloatingStatusWidgetController는 per-character**: AP1HUD가 아닌 `AP1HeroCharacter::InitAbilityActorInfo()`에서 캐릭터마다 생성.
- **BindWidget / BindWidgetOptional**: C++ 멤버 변수 이름과 WBP 위젯 이름이 정확히 일치해야 바인딩된다.
- **NativePaint 사용 기준**: 동적 계산이 필요한 커스텀 렌더링(세그먼트 구분선 등)에만 사용. 단순 색상/스타일은 WBP에서 처리.

## 멀티플레이어 / 네트워킹 컨벤션

- 목표 토폴로지: 데디케이티드 서버 권위(authoritative), 소규모 매치 (단일 아레나 배틀 아레나 모드, 리그오브레전드 "칼바람 나락"/ARAM과 유사).
- 기본적으로 서버 권위 로직을 따릅니다: 게임플레이 판정은 서버에서 확정하고, 클라이언트는 GAS 예측이나 무브먼트 예측(Character Movement Component)이 지원하는 범위에서만 예측합니다.
- 리플리케이트되는 프로퍼티는 `UPROPERTY(Replicated)`로 표시하고 `GetLifetimeReplicatedProps`를 구현합니다. 상태 변경에 대한 클라이언트 반응은 `RepNotify` 콜백을 사용합니다.
- 언리얼 RPC(`Server`, `Client`, `NetMulticast`)는 신중하게 사용합니다 — `Server` RPC는 클라이언트→서버로 신뢰 경계를 넘으므로 `WithValidation`으로 검증합니다.
- 경쟁형 멀티플레이어 게임에 맞게 리플리케이션 범위를 적절히 제한합니다(`bNetUseOwnerRelevancy`, relevancy, `OnlyRelevantToOwner` 등) — 대역폭을 최소화하고, 코스메틱 전용 상태를 과도하게 리플리케이트하지 않습니다.
- 카메라/조작 방식: 백뷰(3인칭, 어깨너머 시점) 캐릭터, 논타겟("non-target") 스킬샷 스타일 어빌리티 캐스팅 — 클릭-투-타겟이 아니라 커서/조준 방향 기반의 라인트레이스/투사체/AOE 타게팅으로 설계합니다.

---

## 프로젝트 설명

**장르:** 멀티플레이어 배틀 아레나 — *Paragon*(Epic의 단종된 MOBA) 에셋을 기반으로 한 단일 아레나(콜로세움) 전투 모드. 리그오브레전드의 "칼바람 나락"(ARAM)처럼, 정식 3라인 MOBA가 아니라 좌우 대칭형 단일 라인 아레나에서 곧바로 교전이 벌어지는 구조.

**프로젝트 목표 (중요, 의사결정의 최우선 기준):** 약 2개월 내에 (1) 비주얼적으로 그럴듯한 완성도와 (2) 언리얼 멀티플레이어 네트워킹 역량을 어필할 수 있는 포트폴리오를 만드는 것이 목표입니다. 콘텐츠 볼륨(맵 크기, 라인/정글 수, 캐릭터 수 등)을 키우는 것보다 적은 콘텐츠를 더 다듬고, 서버 권위 로직/GAS 리플리케이션/예측 같은 네트워킹 코드의 완성도를 높이는 쪽으로 시간을 배분합니다. 작업 범위를 결정할 때 항상 이 목표를 기준으로 판단합니다.

**진행 단계:** 초기 프로토타입. 어빌리티, 전투, UI 등 핵심 게임플레이 시스템의 기반을 갖춘 상태이며, 현재는 스킬 어빌리티 구현 단계입니다.

**코드베이스 출처:** 스타터 템플릿에서 파생되지 않고 완전히 처음부터(from scratch) 구축합니다. 다만 Epic의 **Lyra** 샘플의 구조적 패턴(예: 모듈화된 GAS 구성, Game Feature 플러그인 방식, 경험/데이터 드리븐 설계)을 필요한 부분에서 선택적으로 참고/모방하려는 시도를 할 수 있습니다.

**아트/콘텐츠 에셋:** 오리지널 *Paragon* 에셋 팩과 기타 구매한 마켓플레이스 에셋을 사용합니다 (직접 제작한 아트가 아님) — `Content/ParagonProps`, `Content/KiteDemo` 등에서 확인 가능합니다. 콜로세움/아레나 맵 에셋은 별도로 사용자가 직접 찾아 임포트할 예정입니다. 코드, 게임플레이 시스템, 게임 고유 로직은 전부 커스텀입니다.

**핵심 시스템:**
- **멀티플레이어 모델:** 데디케이티드 서버, 서버 권위적. 소규모 매치(예: 3v3~5v5) 기준의 단일 아레나.
- **맵 구조:** 좌우 대칭형 콜로세움/아레나 1개. 정식 MOBA의 3라인·정글·다중 타워 구조는 사용하지 않음 (ARAM/칼바람 나락 스타일).
- **어빌리티/전투 시스템:** 언리얼의 **Gameplay Ability System(GAS)** 기반 — 속성(Attribute), 게임플레이 이펙트, 게임플레이 태그, 어빌리티 셋.
- **카메라/조작 방식:** 백뷰(3인칭, MOBA/액션RPG 스타일 추적 카메라), **논타겟**("non-targeted") 어빌리티 캐스팅 — 즉 클릭-투-유닛-타겟 방식이 아니라 방향/커서/AOE 기반의 스킬샷 스타일 조준.

**Claude 작업 시 시사점:** 게임플레이에 영향을 주는 모든 코드에서 서버 권위적 설계, GAS에 맞는 구현(임시방편 상태 관리 대신 태그/이펙트/어빌리티 활용), 리플리케이션 정확성을 우선시합니다. 구조에 대해 확신이 없을 때는 Lyra의 패턴(모듈화된 게임플레이, 데이터 드리븐 경험, GAS 구성)을 합리적인 참고 기준으로 삼을 수 있지만, Lyra와 정확히 일치시킬 필요는 없습니다. 작업 범위나 우선순위가 애매할 때는 "2개월 내 비주얼+네트워킹 포트폴리오"라는 목표를 기준으로 판단합니다.

---

## 구현 현황

> 소스코드 기준 현재 상태. 새 시스템을 추가하거나 완료되면 이 섹션을 업데이트할 것.

### GameMode / GameState (`Source/P1/GameModes/`)

| 클래스 | 상태 | 비고 |
|---|---|---|
| `AP1GameMode` | 완료 | 베이스 GameMode (쉘) |
| `AP1ArenaGameMode` | 완료 | DefaultPawnClass=P1HeroCharacter, GameStateClass=P1GameState 지정 |
| `AP1GameState` | 쉘만 존재 | 커스텀 로직 없음 — 매치 상태(라운드, 팀 점수 등) 미구현 |
| `AP1LobbyGameMode` | 완료 | **PreGame(로비) 전용, 데디케이티드 서버 없는 완전 로컬/싱글플레이 레벨**. `DefaultPawnClass=nullptr`(스폰할 Pawn 없음), `PlayerStateClass`를 베이스 `AP1GameMode`가 물려주는 `AP1PlayerState`(ASC 보유, 로비엔 불필요)에서 plain `APlayerState`로 되돌림, `PlayerControllerClass=AP1LobbyPlayerController`. 로그인/매칭 대기열은 전부 `UP1BackendSubsystem`이 웹 백엔드와 직접 통신하고, 매칭 성사 시 `ClientTravel`로 이미 떠 있는 Arena 데디케이티드 서버에 접속 — 이 GameMode 자체는 껍데기에 가깝다 |

### Player (`Source/P1/Player/`)

| 클래스 | 상태 | 비고 |
|---|---|---|
| `AP1PlayerState` | 완료 | ASC + AttributeSet 호스팅, Mixed 리플리케이션, `IGenericTeamAgentInterface` (TeamId 리플리케이션). **레벨/전투기록**: `CharacterLevel`(1~18, `SetCharacterLevel()`), `SkillPoints`(레벨업마다 +1), `Kills`/`Deaths`/`Assists`/`KillStreak`(사망 시 0으로 리셋), `LastDeathTime`(비복제, 서버 전용 — "얼마나 오래 안 죽었는지" 현상금 계산용). `GetXPRequiredForNextLevel()`은 `XPToNextLevelTable`(Row="XPToNextLevel", Time=CharacterLevel)을 `FindCurve`+`Eval`로 조회, 테이블 미설정 시 0(레벨업 불가)으로 안전 폴백. `CharacterLevel`/`SkillPoints`/`Kills`/`Deaths`/`Assists`는 `ReplicatedUsing`(OnRep)에 더해 **네이티브 멀티캐스트 델리게이트**(`OnCharacterLevelChangedNative`/`OnSkillPointsChangedNative`/`OnKDAChangedNative`)를 값이 바뀌는 지점(서버 Setter)과 OnRep(클라이언트) **양쪽에서** 브로드캐스트한다 — GAS Attribute의 `Set()`이 서버/클라 양쪽에서 델리게이트를 쏘는 것과 동일한 이유(리슨서버가 호스트를 겸하면 자기 자신에게는 OnRep이 발동하지 않으므로, Setter 쪽 즉시 호출이 없으면 호스트 자신의 HUD만 갱신 안 됨 — 이 프로젝트는 데디케이티드 서버 토폴로지라 실전에선 안 겪지만 방어적으로 양쪽 다 호출). Kills/Deaths/Assists는 KDA로 항상 함께 표시되므로 델리게이트 하나로 3값을 묶어 브로드캐스트(개별 델리게이트 3개로 안 쪼갬). **왜 GameState가 아니라 PlayerState인가**: `DOREPLIFETIME`가 OwnerOnly 조건 없이 전체 브로드캐스트라 이미 다른 클라이언트도 `GameState->PlayerArray` 순회로 남의 K/D/A를 읽을 수 있음 — 스코어보드 만들 때도 GameState에 별도 복제 불필요, 그때그때 PlayerArray 순회로 충분 |
| `AP1PlayerController` | 완료 | Enhanced Input (Move/Look/Jump + AbilityInputActions TMap), `AP1PlayerCameraManager` 지정 |
| `AP1LobbyPlayerController` | 완료 | PreGame 전용. Enhanced Input/GAS 관련 로직 전혀 없음 — `BeginPlay()`에서 `LobbyWidgetClass`(EditDefaultsOnly)로 `UP1LobbyWidget`을 생성+뷰포트 추가하고 `FInputModeUIOnly`+`bShowMouseCursor=true`로 전환하는 게 전부 |

### Online (`Source/P1/Online/`)

| 클래스 | 상태 | 비고 |
|---|---|---|
| `UP1BackendSubsystem` | 완료 | `UGameInstanceSubsystem` — PreGame 레벨에서 웹 백엔드(`Backend/`, Spring Boot)와 통신하는 유일한 창구. GameInstance 생존주기 동안 유지되므로 `ClientTravel`로 Arena 서버에 접속한 뒤에도 살아있음(향후 매치 결과 보고 등 재사용 가능). `Signup`/`Login`(성공 시 JWT를 내부 `AuthToken`에 저장)/`JoinQueue` HTTP 호출(`FHttpModule`+수동 Json 직렬화, `HTTP`/`Json`/`JsonUtilities` 모듈 `P1.Build.cs`에 추가 필요)과 매칭 상태 폴링(`FTimerHandle`, 2초 간격 `GET /api/match/status`, `MatchStatusPollIntervalSeconds`)을 전담. `BackendBaseUrl`은 `UPROPERTY(Config)`(`UCLASS(Config=Game)`)라 `DefaultGame.ini`의 `[/Script/P1.P1BackendSubsystem]` 섹션에서 재컴파일 없이 주소 변경 가능. `OnSignupComplete`/`OnLoginComplete`/`OnQueueJoined`/`OnMatchFound`(dynamic multicast, BlueprintAssignable) 델리게이트로 위젯에 결과 전달 — `OnMatchFound`를 받으면 `HandleMatchStatusPayload()`가 알아서 폴링 타이머를 멈춘다 |

### Characters (`Source/P1/Characters/`)

| 클래스 | 상태 | 비고 |
|---|---|---|
| `AP1CharacterBase` | 완료 | Abstract 베이스, `IAbilitySystemInterface` + `IGenericTeamAgentInterface`, CachedASC 포인터, `IsSameTeam()` 헬퍼, `CharacterType` 태그(기본 Hero) + `IsHeroOrBoss()` |
| `AP1HeroCharacter` | 완료 | `InitAbilityActorInfo()` (ASC 초기화만) + `AddDefaultAbilities()` (서버 전용, PossessedBy에서 호출) 분리. `MovementSpeed` 어트리뷰트 바인딩. 특정 스킬/버프를 아는 코드는 없음 — `UP1BuffCosmeticEffectComponent`가 붙어있으면 ASC를 넘겨주는 범용 훅만 보유. **레벨업/스폰/리스폰 스탯**: `ApplyBaseStatsForLevel(Level, bFullHeal)`이 `DefaultAttributesEffect`를 실제 `CharacterLevel`로 재적용 후, `bFullHeal=true`(스폰/리스폰)면 신규 MaxHealth/MaxMana까지 완전 회복, `bFullHeal=false`(레벨업)면 딱 증가분(NewMax-OldMax)만큼만 현재 체력/마나를 올린다 — **레벨업이 공짜 풀힐이 되면 안 된다는 요구사항** 때문에 두 경우를 분리. `CheckLevelUp()`은 `UP1AttributeSet`이 Experience 증가를 감지해 호출 — `PS->GetXPRequiredForNextLevel()` 임계치를 넘는 동안 while로 반복 레벨업(한 번에 여러 레벨 상승 가능), 매 레벨 `AddSkillPoint()` + `ApplyBaseStatsForLevel(..., false)`. **킬/어시스트 보상**: `GrantKillReward(VictimKillStreak, VictimTimeSinceLastDeath, VictimLevel)`은 `GoldRewardKillEffectClass`(MMC `UP1MMC_GoldKillBounty` 기반 현상금)+`ChampionKillXPTable`(Row 1개짜리 CurveTable, `GetChampionXPReward()`가 `GetRowMap()`의 첫 커브를 `Eval(VictimLevel)`) 조합으로 골드/경험치 지급, `GrantAssistReward(VictimLevel)`은 `GoldRewardFlatEffectClass`(고정 `AssistGoldAmount`=50)+`ChampionAssistXPTable` 조합 — 둘 다 `UP1AttributeSet::HandleKillRewards()`에서 호출됨. `ApplyFlatRestoreEffect(EffectClass, SetByCallerTag, Magnitude)`가 "SetByCaller 태그 하나짜리 Instant GE를 자신에게 적용"하는 공용 헬퍼(회복/마나/골드/경험치 전부 공유) |
| `AP1MinionCharacter` | 미구현 | 코드베이스에 없음 |
| `UP1BuffCosmeticEffectComponent` | 완료 | 자세한 내용은 Ability System 섹션 참고(GAS 태그 이벤트에 강하게 의존) — "버프 태그 활성 동안 코스메틱 유지, 소멸 시 원복"을 데이터로 구성하는 범용 `UPawnComponent`, 필요한 영웅에만 추가 |

### Ability System (`Source/P1/AbilitySystem/`)

**`Abilities/` 하위 폴더 구조**: 여러 영웅이 공유하는 베이스(`P1DamageGameplayAbility`, `P1GameplayAbility_MeleeAttack`)는 `Abilities/` 바로 밑에 두고, 특정 영웅 전용 어빌리티(현재는 그레이스톤의 RMB/Q/E/R/기본공격 변종/패시브 전부)는 `Abilities/Greystone/`처럼 영웅별 하위 폴더로 분리한다. **이 파일 이동은 블루프린트에 전혀 영향을 주지 않는다** — 블루프린트의 부모 클래스 참조는 `Source/` 안의 파일 경로가 아니라 컴파일된 리플렉션 경로(`/Script/모듈이름.클래스이름`)로 결정되므로, 클래스 이름과 모듈이 그대로면 파일을 어디로 옮기든 안전하다(클래스 "이름"을 바꾸는 것과는 다른 얘기 — 그건 Core Redirects가 필요함). 파일 이동 시 실제로 해야 하는 건: (1) 그 파일을 include하는 다른 `.cpp`의 include 경로 갱신, (2) `.uproject` 우클릭 → Generate Visual Studio project files로 프로젝트 파일 재생성.

| 클래스 | 상태 | 비고 |
|---|---|---|
| `UP1AbilitySystemComponent` | 완료 | InputTag → Ability press/release 라우팅. `bReplicateInputDirectly` 서버 전파. 조준 중(`State.TargetingAbility`) LMB=confirm/RMB=cancel 리라우팅 + 나머지 차단 |
| `UP1AttributeSet` | 완료 | 아래 어트리뷰트 전체 구현 — 리플리케이션, 클램핑 포함. `PostGameplayEffectExecute`가 Damage 메타 어트리뷰트를 Health로 변환하기 직전 두 가지를 순서대로 체크: (1) `State.Invulnerable` 태그가 있으면 데미지를 완전히 무시(전 어빌리티 공유, Stone Forged Soul 스테이시스가 첫 사용처), (2) 이 데미지가 기본공격에서 왔고(`Data.EffectSpec.CapturedSourceTags`에서 `Ability.BasicAttack` 확인) 캐릭터가 Stoicism 디플렉트를 갖고 있으며(`Ability.StoicismDeflect` 루즈 태그) 쿨다운 중이 아니면(`Cooldown.Ability.StoicismDeflect` 부재) 데미지를 무효화하고 `Event.StoicismDeflect.Consumed` 이벤트를 보내 해당 어빌리티가 자기 쿨다운을 커밋하게 함 — **AttributeSet은 Stoicism 어빌리티 클래스를 전혀 참조하지 않고 태그만으로 전부 판별**. **킬/어시스트 감지**: `RecordDamageContribution()`이 데미지를 준 시점마다 `TMap<TWeakObjectPtr<AP1PlayerState>, float>`(기여자→마지막 기여 시각)에 기록해두고, `HandleKillRewards()`가 Health<=0 감지 시 `Data.EffectSpec.GetEffectContext().GetInstigator()`로 킬러를, 이 맵에서 10초(`AssistWindowSeconds`) 이내 기여자 전원을 어시스터로 판별해 각각 `AP1HeroCharacter::GrantKillReward()`/`GrantAssistReward()`를 호출한다(피해자의 죽기 직전 KillStreak/생존시간/CharacterLevel을 `VictimPS->AddDeath()` 호출 **이전에** 먼저 캡처 — 리셋되기 전 값이 필요). AttributeSet은 이때도 `AP1HeroCharacter`를 직접 캐스트해 호출하지만, 이건 "어빌리티 클래스 참조 금지" 원칙과 무관한 별개 카테고리(Character 클래스 참조는 허용, Ability 클래스 참조만 금지) — 킬 보상은 전 영웅 공통 로직이라 어차피 per-ability 유연성이 필요 없기도 함. `Experience` 어트리뷰트 변경 시 `Hero->CheckLevelUp()`을 호출해 레벨업 체크는 캐릭터 클래스에 위임(AttributeSet은 XP 커브테이블을 몰라도 됨) |
| `UP1MMC_GoldKillBounty` | 완료 | `GE_GoldReward_Kill`의 Gold Modifier 계산 클래스(`UGameplayModMagnitudeCalculation` 상속). `Data.KillStreak`/`Data.TimeSinceLastDeath`를 `Spec.GetSetByCallerMagnitude(Tag, bWarnIfNotFound=false, Default=0.f)`로 읽어 `BaseKillGold`(기본 300) + `KillStreakBonusTable`(CurveTable, Row="KillStreakBonusGold") 조회값 + `GoldPerSecondAlive`(초당 2, `MaxTimeAliveBonus`=200에서 캡) 선형 가산을 합산해 최종 골드를 반환 — 오래 살아있었거나 연속킬 중인 상대를 잡으면 더 많은 골드를 주는 현상금 시스템. **GE의 Custom Calculation Class는 런타임에 해당 클래스의 CDO를 그대로 쓰므로, `KillStreakBonusTable` 같은 에셋 참조 프로퍼티는 이 네이티브 C++ 클래스가 아니라 이를 상속하는 블루프린트(예: `MMC_GoldKillBounty`)의 Class Defaults에서 지정해야 한다** — GE 에셋 자체에는 SetByCaller 관련 설정이 전혀 없고, 값은 전부 `AP1HeroCharacter::GrantKillReward()`가 스펙에 미리 심어둔다 |
| `UP1GameplayTags` | 완료 | Native 태그: `InputTag.Ability.BasicAttack/RMB/E`, `Ability.BasicAttack/AssaultTheGates/SacredOath`, `State.Attacking`, `Event.Montage.*`, `Data.DamageMultiplier`, `Data.Damage.Flat/PhysicalPower/TargetMaxHealthPct`, `Data.DebuffMagnitude`(범용 디버프 크기 채널), `Character.Type.Hero/Boss/Minion` |
| `UP1GameplayAbility` (베이스) | 완료 | `InstancedPerActor` + `LocalPredicted` + `bReplicateInputDirectly=true` 기본값, `GetP1Character/PlayerController` 헬퍼, `ApplyEffectToSelf(EffectClass, SetByCallerTag, Magnitude)` — "자신에게 GE 적용" 공통 헬퍼 (버프/디버프/쿨다운 등 전 어빌리티 공유). **`ActivationOwnedTags`/`ActivationBlockedTags`에 `State.Attacking`을 전 어빌리티 공통으로 부여/차단 — 게임 전역 규칙("스킬 사용 중엔 다른 어빌리티 발동 불가", WASD 이동은 어빌리티가 아니라 예외)을 베이스 하나로 처리, 개별 어빌리티마다 반복 설정 불필요**. `bActivateOnGranted`(기본 false) — true면 `AP1HeroCharacter::AddDefaultAbilities()`가 `GiveAbility` 대신 `GiveAbilityAndActivateOnce`로 부여해 그 자리에서 1회 활성화한다. **입력도 트리거 이벤트도 없이 부여되는 즉시 스스로 계속 돌아야 하는 상시 패시브에 필수**(`GiveAbility`만으로는 `ActivateAbility`가 절대 호출되지 않아 영원히 잠들어있게 됨 — Stoicism Vitality에서 처음 발견된 문제, `GA_Lightbringer`처럼 입력 바인딩이 있거나 `GA_StoicismDeflect`처럼 GameplayEvent로 트리거되는 어빌리티는 이 플래그가 필요 없음) |
| `UP1DamageGameplayAbility` | 완료 | 데미지 어빌리티 공통 베이스 — `DamageEffectClass` + `FlatDamage`(`FScalableFloat`, 레벨 스케일링)/`PhysicalPowerCoefficient`/`MagicalPowerCoefficient`/`TargetMaxHealthPctCoefficient`/`SourceMaxHealthPctCoefficient` + `ApplyDamageToTarget()` (계수 채널 + 호출 시점 Bonus 파라미터로 버프성 추가 데미지 지원) + `GetEnemiesInRadius(Center, Radius, HalfHeight)` (구 오버랩+팀+높이 필터 공용 헬퍼, 방향성 필터는 호출자가 추가). **`ApplyDamageToTarget`은 raw `ASC->MakeOutgoingSpec`을 쓰기 때문에(어빌리티 레벨 `MakeOutgoingGameplayEffectSpec`과 달리) 스펙에 어빌리티 Asset Tags가 자동으로 안 실린다 — `SpecHandle.Data->CapturedSourceTags.GetSpecTags().AppendTags(GetAssetTags())`로 수동으로 채워준다.** "이 데미지가 어떤 어빌리티에서 왔는지"를 타겟 쪽(AttributeSet 등)에서 판별해야 하는 모든 경우(Stoicism 디플렉트가 첫 사용처)에 필요 |
| `UP1ExecCalc_Damage` | 완료 | 물리/마법 혼합 데미지 ExecutionCalculation. 계수 **채널 합산**: `Raw=(Flat + PhysCoeff*Source.PhysicalPower + MagCoeff*Source.MagicalPower + TargetMaxHPCoeff*Target.MaxHealth + SourceMaxHPCoeff*Source.MaxHealth)`, `Pre=Raw*Mult` × 방어 감산(`Armor/(Armor+100)`, Penetration flat 차감) → `Damage` 메타. Source.MaxHealth와 Target.MaxHealth는 같은 어트리뷰트를 서로 다른 Capture(Source/Target)로 캡처해야 해서, `DEFINE_ATTRIBUTE_CAPTUREDEF` 매크로 대신 `SourceMaxHealthDef`는 `FGameplayEffectAttributeCaptureDefinition` 생성자를 직접 호출해 구성(매크로는 `GET_MEMBER_NAME_CHECKED`로 프로퍼티 실명을 요구해 같은 이름을 두 캡처에 재사용 불가). 미래: damage-type 태그로 물리/마법 방어(PhysicalArmor vs MagicalArmor) 선택 |
| `UP1GameplayAbility_MeleeAttack` | 완료 | 콤보 기본공격. AnimNotify → `BasicAttackHit` 이벤트 → `OnHitEventReceived()`(protected virtual, 적중 여부와 무관하게 스윙마다 호출)에서 `GetEnemiesInRadius()` + 전방 반원 필터 → `ApplyComboHitDamage()`(protected virtual)로 데미지 적용. 콤보 큐는 `InputPressed()`에서만 처리. Sacred Oath 관련 코드는 전혀 없음(서브클래스로 분리) — 이 클래스는 특정 버프의 존재를 모르며, 시너지가 있는 영웅은 아예 이 클래스 대신 `MeleeAttack_SacredOath`를 등록한다(같은 InputTag 경쟁 없음, 상세 배경은 GAS 컨벤션 참고) |
| `UP1GameplayAbility_MeleeAttack_SacredOath` | 완료 | `MeleeAttack` 상속, 콤보/몽타주 로직은 그대로 재사용하고 `ApplyComboHitDamage()` + `OnHitEventReceived()` 둘 다 오버라이드. Sacred Oath 시너지가 있는 영웅(그레이스톤)의 **유일한** BasicAttack 어빌리티로 등록 — 평소 버전과 경쟁하지 않는다. `OnHitEventReceived`(적중 여부와 무관하게 스윙 자체가 발생하는 시점, `Super`가 `ApplyComboHitDamage`를 호출하기 *전*)에서 `Buff.SacredOath.Active` 태그가 아직 있으면 `TrailParticleTemplate`을 1회 재생하고 `Super::OnHitEventReceived()`를 호출한다 — 허공에 헛스윙해도(버프가 소모되지 않으므로) 매 스윙마다 트레일이 다시 나오는 이유. `ApplyComboHitDamage()`에서는 같은 태그로 분기: 없으면 `Super::ApplyComboHitDamage()`(평소 로직)로 폴백, 있으면 강화 로직(Cleave 무시(전원 100%)+추가데미지(`BonusFlatDamage`는 `FScalableFloat`, 레벨 스케일링)+슬로우) 적용 후 `RemoveActiveEffectsWithGrantedTags`로 버프 소모. 콤보가 여러 스텝 이어지는 동안 같은 인스턴스가 재사용되므로 매 히트마다 재확인 — 첫 스윙에서 소모되면 이후 스텝은 자동 폴백돼 "다음 1회만" 강화를 보장(단, 트레일은 태그가 있는 한 헛스윙에도 계속 나옴 — 강화 데미지와는 다른 생명주기). 검 발광은 이 클래스도, 다른 어떤 어빌리티도 전혀 관여하지 않음 — `UP1BuffCosmeticEffectComponent`가 태그 자체를 구독해 자동 처리. `bShowDebugSacredOath`로 강화 히트 대상에 금색 디버그 구체+보너스데미지 텍스트 표시 가능 |
| `UP1GameplayAbility_SacredOath` (E) | C++ 완료 (에셋 대기) | 순수 버프 어빌리티(Damage 상속 아님). 커밋 후 자신에게 `BuffEffectClass`(5초 지속, `Buff.SacredOath.Active` 태그+`AttackRange` 보너스 모디파이어)를 적용하고, `CastMontage` 재생 후 종료(몽타주 없으면 즉시 종료) — **그게 전부**. 검 발광은 물론 어떤 코스메틱 Multicast도 이 어빌리티가 직접 호출하지 않는다(`UP1BuffCosmeticEffectComponent`가 태그 부여/소멸에 반응해 알아서 처리하므로 어빌리티는 GAS 로직만 신경 쓰면 됨). 무기 궤적(트레일)은 "버프 지속 동안 매 스윙마다 재생"이라는 요구 때문에 태그 상태가 아니라 스윙 이벤트에 반응해야 해서 `MeleeAttack_SacredOath::OnHitEventReceived`가 담당. 실제 데미지/슬로우/궤적은 `MeleeAttack_SacredOath` 쪽에서 처리 |
| `UP1BuffCosmeticEffectComponent` (`Characters/`) | 완료 | "버프 태그가 활성인 동안 코스메틱 이펙트(머티리얼/지속 파티클)를 유지하고, 태그가 사라지면 자동 원복"하는 범용 `UPawnComponent`. `Effects`(`TArray<FP1BuffCosmeticEffectEntry>`) 배열로 태그↔머티리얼 슬롯/파티클을 데이터로 매핑 — 특정 스킬 이름을 전혀 몰라도 되므로 이 시너지가 필요한 영웅의 BP에만 컴포넌트를 추가하면 되고, 다른 영웅은 영향받지 않는다. `BindToAbilitySystemComponent(ASC)`를 소유 캐릭터가 ASC 준비 시점(`AP1HeroCharacter::InitAbilityActorInfo`)에 호출해줘야 하며, 각 Entry의 태그마다 `RegisterGameplayTagEvent(NewOrRemoved)`를 구독해 카운트>0이면 켜고 0이면 끈다(`AddUObject`의 페이로드 파라미터로 Entry 데이터를 콜백에 함께 전달). 서버(`HasAuthority()`)에서만 Multicast를 호출. 그레이스톤 BP에 이 컴포넌트를 추가하고 Sacred Oath 검 발광 항목(`Buff.SacredOath.Active` → `MaterialSlotName`+`OverrideMaterial`) 하나를 등록해 사용 |
| `UP1GameplayAbility_MakeWay` (Q) | C++ 완료 (에셋 대기) | 캐릭터를 따라다니는 화염 회오리. 어빌리티가 직접 반복 타이머(1초 간격 4틱=1,2,3,4초) 소유 → 매 틱 `GetEnemiesInRadius()`로 현재 위치 기준 재스캔 → 데미지+방어력감소 디버프(중첩, `Data.DebuffMagnitude`). 코스트는 캐스트 시 `CommitAbilityCost()`, 쿨다운은 4번째 틱 직후(지속시간 종료 시점)에 적용 — 표준 GE Periodicity 대신 C++ 타이머를 쓴 이유는 "캐릭터가 이동하며 매번 재판정"이 고정 타겟 기반 Periodic GE로는 불가능하기 때문. `WhirlwindParticleTemplate`/`WhirlwindSocketName`으로 캐릭터를 따라다니는 회오리 지속 파티클을 `ActivateAbility`에서 `MulticastSetAttachedParticleEffect`로 시작하고, `EndAbility`(정상 종료/조기 취소 공통 경로)에서 `MulticastStopAttachedParticleEffect`로 정리 — 버프 태그 기반이 아니라 어빌리티 자신의 타이머 생명주기에 직접 맞물려 있다는 점이 Sacred Oath 패턴과 다름(별도 GE 태그 없이 어빌리티가 이미 시작/종료 시점을 정확히 알고 있으므로) |
| `UP1AnimNotifyState_MaterialOverride` | 완료 | 범용 머티리얼 오버라이드 AnimNotifyState. 구간 동안 지정 슬롯을 `OverrideMaterial`로 교체, 끝나면 `SetMaterial(Index, nullptr)`로 스켈레탈메시 기본 머티리얼 복귀. 몽타주 기반이라 모든 클라이언트에 리플리케이트됨(GAS 코드로 직접 SetMaterial하면 로컬+서버에서만 보이는 것과 대비). **몽타주의 "고정된 타임라인 구간"에만 종속** — GE 수명(조기 소모 등 가변 길이)에 정확히 대응해야 하면 아래 GameplayCue를 대신 사용 |
| `UP1GameplayCueNotify_MaterialOverride` / `_ParticleEffect` | 완료, **현재 미사용** | GameplayCue 기반 코스메틱 효과(자세한 설명은 GAS 컨벤션 참고). Sacred Oath는 결국 이 방식 대신 아래 Multicast 패턴으로 전환했음 — 폴더 스캔(`GameplayCueNotifyPaths`)+태그매칭이 에디터 세션 중 갱신이 안 되는 등 다루기 번거로워서. 범용 유틸리티 클래스라 삭제하지 않고 남겨둠 — 다른 어빌리티에서 GameplayCue 방식이 더 적합하면 재사용 가능 |
| `AP1CharacterBase::MulticastSetMaterialOverride` / `MulticastPlayParticleEffect`(1회성) / `MulticastSetAttachedParticleEffect`+`MulticastStopAttachedParticleEffect`(부착형 지속) | 완료 | 코스메틱 Multicast 배관(범용 인프라, 특정 스킬을 모름). GameplayCue의 스캔 경로/태그 매칭 없이, 서버에서 호출하면 Multicast RPC로 모든 클라이언트(시뮬레이티드 프록시 포함)에 자동 복제됨 — 별도 Cue Notify 에셋 불필요. 호출부는 `UP1BuffCosmeticEffectComponent`(태그 기반 지속, 예: Sacred Oath 검 발광), 개별 어빌리티의 1회성 호출(예: Sacred Oath 무기 궤적), 또는 어빌리티가 자기 타이머 생명주기에 맞춰 직접 시작/정지(예: Make Way 회오리)까지 세 갈래. `MulticastSetAttachedParticleEffect`는 소켓이 비어있거나 존재하지 않으면 루트 컴포넌트(액터 전체)에 부착하는 폴백이 있음 — 특정 본이 아니라 캐릭터 전체를 중심으로 도는 이펙트(회오리 등)에 적합 |
| `AP1HeroCharacter::InitAbilityActorInfo` → `UP1BuffCosmeticEffectComponent` 바인딩 훅 | 완료 | ASC 준비 시점에 `FindComponentByClass<UP1BuffCosmeticEffectComponent>()`로 컴포넌트를 찾아 `BindToAbilitySystemComponent(ASC)`를 호출하는 한 줄짜리 범용 훅 — 컴포넌트가 없는 영웅(대부분)은 그냥 스킵된다. `AP1HeroCharacter` 자체엔 특정 버프 태그/슬롯 이름이 전혀 남아있지 않음(예전엔 `WeaponGlowMaterialSlotName`/`BindSacredOathBuffTagListener`가 여기 하드코딩돼 있었으나 컴포넌트로 이관) |
| `UP1GameplayAbility_AssaultTheGates` (RMB) | **완료 (풀 테스트 통과)** | 2단계 지면조준 도약. 조준(WaitTargetData+GroundDecal, 미커밋) → LMB 확정/RMB 취소 → 확정 시 코스트 소모 + 서버 쿨다운(SetByCaller 지속시간) → `ApplyRootMotionJumpForce`(Duration=Land 노티파이 트리거 시점) 포물선 도약 → 착지 AOE 데미지 → 영웅/보스 적중 시 이속버프+쿨다운 35%↓. 도약 내내 `State.Attacking` 소유해 LMB 등 다른 어빌리티 인터럽트 차단 |
| `AP1TargetActor_GroundDecal` | 완료 | 지면 조준 타겟액터. 카메라 트레이스+사거리 클램프. 시각 표시는 `UStaticMeshComponent`+Translucent Surface 머티리얼(원형 마스크, 팀컬러 홀로그램) — **Deferred Decal이 아님**: UE5.8 Substrate 데칼(Coverage/Convert To Decal 등)이 불안정해 평면 메시 방식으로 전환. 확정 위치를 LocationInfo 타겟데이터로 서버 복제 |
| `UP1GameplayAbility_StoneForgedSoul` (R, 궁극기) | C++ 완료 (에셋 대기, 카메라 연출 미구현) | 조준 없이 제자리에서 발동. 캐스팅+상승+공중대기+하강+착지가 전부 담긴 몽타주 하나만 재생 — **RMB와 달리 `ApplyRootMotionJumpForce` 등 코드 기반 궤적 생성이 없음**: 목표 지점이 항상 "원위치"로 고정된 왕복 이동이라 원본 애니메이션에 실제 수직 루트모션이 있으면 그걸 그대로 쓰는 게 더 정확하고 간단함(RMB가 코드 궤적을 쓴 이유는 지면 조준으로 매번 달라지는 가변 목표 지점 때문이었는데, 여긴 그 조건 자체가 없음). 몽타주 재생 시작 시 `MovementMode`를 `MOVE_Flying`으로 전환(중력이 루트모션과 안 싸우게), 착지 AnimNotify(`Event.Montage.StoneForgedSoul.Crash`, RMB의 Land 이벤트와 동일 패턴)에서 `MOVE_Walking`으로 복귀 + 그 자리 기준 범위 데미지(`UP1DamageGameplayAbility`의 `FlatDamage`/`PhysicalPowerCoefficient` 재사용). 발동 즉시(스테이시스 시작) 3가지 병행: (1) 잃은 체력 비율로 최대 `MaxHealAmplification`배까지 증폭되는 회복 — 공식이 단순 계수 곱셈이 아니라 C++에서 최종값을 계산해 `Data.Heal.Flat` SetByCaller로 전달, (2) `State.Invulnerable` 태그를 부여하는 무적 GE(전 어빌리티 공용 태그, 아래 참고), (3) `Make Way`와 동일한 패턴의 반복 재스캔 타이머로 근접도 비례 슬로우(거리 0=`MaxSlowPercent`, 반경 끝=0%, 선형 감쇠) 계속 갱신 적용 |
| `UP1AnimNotify_SendGameplayEvent` | 완료 | 범용 GameplayEvent 발신 AnimNotify — `EventTag` 프로퍼티로 태그 지정. 콤보 히트/스킬 착지 이벤트 등 재사용 |
| `UP1GameplayAbility_StoicismDeflect` (패시브 절반 1/2) | C++ 완료 (에셋 대기) | "다음 기본공격 무효화, N초 쿨다운" — 실제 무효화 판정은 이 어빌리티가 아니라 `UP1AttributeSet::PostGameplayEffectExecute`에서 처리(위 참고). 이 클래스의 역할은 `OnGiveAbility`에서 자신의 Asset Tag(`Ability.StoicismDeflect`)를 루즈 태그로 ASC에 심어 "존재 판별"을 가능케 하는 것과, `Event.StoicismDeflect.Consumed` GameplayEvent(`AbilityTriggers`)로 트리거되면 `CommitAbilityCooldown()`만 커밋하고 즉시 종료하는 것뿐 — "지금 사용 가능한지"는 네이티브 쿨다운 태그 부재로 이미 판별되므로 별도 "준비완료" 상태 관리가 필요 없다. 다른 어빌리티 사용 중에도 항상 반응해야 해서 베이스의 `State.Attacking` 상호배타 규칙에서 제외(`ActivationBlockedTags`/`ActivationOwnedTags`에서 Remove). `NetExecutionPolicy=ServerOnly`. 쿨다운(15→4.8초 등, 영웅 레벨별)은 `CooldownGameplayEffectClass`의 Duration Modifier를 Scalable Float로 설정하는 것만으로 충분(Cost와 동일한 이유로 C++에 값을 안 둠) |
| `UP1GameplayAbility_StoicismVitality` (패시브 절반 2/2) | C++ 완료 (에셋 대기) | "초당 최대체력 비례 회복 + 고정 방어력, 둘 다 잃은 체력 비율에 비례 증폭 + 체력 50% 미만이면 회복만 2배" — 상시 켜져있는 패시브라 코스트/쿨다운 없이 `ActivateAbility`에서 반복 타이머(기본 1초 간격, 캐릭터 생존 내내 반복)를 시작하고 즉시 1틱 선실행. 매 틱 현재 Health/MaxHealth를 읽어 회복량(즉시 회복 GE, `Data.Heal.Flat` SetByCaller)과 방어력 보너스(버프 GE, `Data.ArmorBonus.Flat` SetByCaller)를 매번 새로 계산해 재적용 — 둘 다 "현재 체력"이라는 계속 바뀌는 값에 의존해 한 번 적용하고 끝나는 Infinite GE로는 표현 불가. 방어력 버프 GE는 Stacking Type=AggregateByTarget/Stack Limit=1로 설정해 누적이 아니라 항상 최신 값으로 덮어써야 함(Stone Forged Soul 슬로우 디버프와 동일 패턴). Deflect와 마찬가지로 `State.Attacking` 제외 + `NetExecutionPolicy=ServerOnly`. **`bActivateOnGranted=true`** — 이게 없으면 `ActivateAbility`가 절대 안 불려서 타이머 자체가 시작을 안 함(실제로 겪은 버그, 아래 알려진 버그 표 참고) |
| GameplayEffect 데이터 에셋 | 진행 중 | 기본공격: `GE_BasicAttackDamage`. RMB: `GE_Damage`(공용, `P1ExecCalc_Damage` Execution 등록 — 모든 물리 데미지 어빌리티가 공유), `GE_Cooldown_AssaultTheGates`(Duration=SetByCaller `Data.CooldownDuration`, Target Tags로 `Cooldown.Ability.AssaultTheGates` 부여), `GE_Cost_Assault`, `GE_Buff_MoveSpeed`. `DefaultAttributesEffect`(`GE_GreystoneDefaultAttributes`) 완료. **전투 보상/레벨업**: `GE_ExperienceReward_Flat`(Instant, `Experience` Modifier=Additive, Magnitude=SetByCaller `Data.Experience.Flat` — 킬/어시스트 공용, 액수만 호출부에서 다르게 주입) 완료·PIE 실전 검증됨(`[AttributeSet] Experience 획득` 로그로 확인). `GE_GoldReward_Kill`(Gold Modifier=Custom Calculation Class=`MMC_GoldKillBounty`)/`GE_GoldReward_Flat`(Gold Modifier=Additive, SetByCaller `Data.Gold.Flat`) 완료 |

**AttributeSet 어트리뷰트 목록:**

| 카테고리 | 어트리뷰트 |
|---|---|
| Vital | `Health`, `MaxHealth`, `Mana`, `MaxMana`, `HealthRegen`, `ManaRegen` |
| Combat | `PhysicalPower`, `MagicalPower`, `AttackSpeed`, `BasicAttackTime`, `AttackRange`, `Cleave` |
| Combat | `PhysicalArmor`, `MagicalArmor`, `PhysicalPenetration`, `MagicalPenetration` |
| Combat | `LifeSteal`, `Tenacity`, `AbilityHaste` |
| Movement | `MovementSpeed` |
| Progression | `Gold`, `Experience` — 전투 보상 시스템용. GE Modifier 타겟이 될 수 있어야 해서(MMC 현상금 계산 등) PlayerState의 plain int가 아니라 정식 GAS Attribute로 구현 |
| Meta (transient) | `Damage` — ExecCalc_Damage 출력 누적, PostGameplayEffectExecute에서 Health로 변환 (비복제) |

### UI (`Source/P1/UI/`)

디렉토리 구조: `UI/HUD/`, `UI/Widget/`, `UI/WidgetController/`

**Widget:**

| 클래스 | 상태 | 비고 |
|---|---|---|
| `UP1UserWidget` | 완료 | 모든 위젯 베이스. `SetWidgetController()` → `OnWidgetControllerSet()` 패턴 |
| `UP1SegmentedBarWidget` | 완료 | `UProgressBar`(FillBar) + NativePaint 구분선. `SetValues(current, max, regen)`. `SegmentSize`/`DividerColor` EditAnywhere. **`FillColor`/`BackgroundColor`도 EditAnywhere+`SetFillColor()`/`SetBackgroundColor()`(BlueprintCallable)로 노출** — 이 위젯을 배치하는 쪽(WBP_P1Overlay에서 HealthBar/ManaBar 인스턴스마다, 또는 위젯 컨트롤러 런타임 코드)에서 색을 지정. `NativePreConstruct`에서 적용해 디자이너 미리보기에도 즉시 반영됨. Fill은 `UProgressBar::SetFillColorAndOpacity()`로 바로 되지만, Background는 단순 색상 프로퍼티가 없어서(Style의 `BackgroundImage` 브러시 틴트로만 제어 가능) `GetWidgetStyle()`로 스타일을 복사해 `BackgroundImage.TintColor`만 바꾸고 `SetWidgetStyle()`로 재적용하는 방식 사용 |
| `UP1SkillIconWidget` | 완료 | 스킬 아이콘. `CooldownOverlay`(UImage, MID로 CooldownPercent 파라미터), `CooldownText`. `StartCooldown()`/`ClearCooldown()`. NativeTick으로 0.1s 단위 업데이트. **`SkillIconImage`도 마름모(다이아몬드) 모양을 내기 위해 CooldownOverlay와 같은 MID 패턴 사용** — 위젯을 45도 회전시키면 아이콘 그림 자체가 기울어지므로, 대신 UV 기반 마름모 알파 마스크 머티리얼(`IconTexture` Texture Parameter 필요)을 Brush로 설정해두면 `NativeConstruct`가 MID를 만들고 `SetSkillIcon()`이 텍스처 파라미터만 갈아끼운다. WBP의 Brush가 마스크 머티리얼이 아니면(일반 텍스처/빈 Brush) `IconMID`가 null로 남아 예전처럼 텍스처를 직접 Brush에 설정하는 방식으로 자동 폴백(하위 호환) |
| `UP1FloatingStatusWidget` | 완료 | 캐릭터 머리 위 체력/마나 위젯. `OnWidgetControllerSet()`에서 FloatingStatusWidgetController 델리게이트 바인딩 |
| `UP1LobbyWidget` | 완료 (C++, WBP 배치 대기) | PreGame 로그인+매칭 대기 UI. **`UP1UserWidget`을 상속하지 않고 순수 `UUserWidget`을 직접 상속** — 이 위젯의 데이터 소스는 GAS/ASC 기반 위젯 컨트롤러가 아니라 `UP1BackendSubsystem`(HTTP)이라 위젯 컨트롤러 패턴 자체가 안 맞기 때문에 의도적으로 다른 베이스를 씀. `UsernameBox`/`PasswordBox`(UEditableTextBox)+`LoginButton`/`SignupButton`(BindWidget), `QueueButton`/`StatusText`/`LoginPanel`/`QueuePanel`(BindWidgetOptional, 로그인 성공 시 `LoginPanel` 숨기고 `QueuePanel` 노출). `NativeConstruct`에서 `UP1BackendSubsystem`의 4개 델리게이트 구독, `OnMatchFound` 수신 시 `GetOwningPlayer()->ClientTravel(ServerAddress, TRAVEL_Absolute)`로 Arena 서버 접속 |

**HUD:**

| 클래스 | 상태 | 비고 |
|---|---|---|
| `UP1HUDWidget` | 완료 (C++, WBP 배치 대기) | 메인 HUD. HealthBar/ManaBar(UP1SegmentedBarWidget), SkillIcon_Q/E/R/RMB/Passive/LMB/Flash(UP1SkillIconWidget). **좌하단 플레이어 정보(신규)**: `LevelText`/`KDAText`/`GoldText`(UTextBlock, BindWidgetOptional)+`ExperienceBar`(UP1SegmentedBarWidget, HealthBar/ManaBar와 같은 타입 재사용) — `OnLevelChanged`("Lv. N")/`OnKDAChanged`("K / D / A")/`OnGoldChanged`("N G")/`OnExperienceChanged`(`ExperienceBar->SetValues(Current, Max(XPRequired,1))`, 0으로 나누기 방지로 1 클램프) 핸들러가 텍스트/바를 갱신 |
| `AP1HUD` | 완료 | 위젯 컨트롤러 레지스트리. `InitOverlay()` → WBP 생성 + SetWidgetController + BroadcastInitialValues |

**WidgetController:**

| 클래스 | 상태 | 비고 |
|---|---|---|
| `UP1WidgetController` | 완료 | 베이스. `FWidgetControllerParams`(PC/PS/ASC/AS). `FOnAttributeChangedSignature` 델리게이트 정의 |
| `UP1OverlayWidgetController` | 완료 | 6개 델리게이트(Health/MaxHealth/HealthRegen/Mana/MaxMana/ManaRegen). ASC 어트리뷰트 변경 구독. **Progression 델리게이트(신규)**: `OnGoldChanged`/`OnExperienceChanged`(Gold/Experience 둘 다 GAS Attribute라 기존과 동일하게 `GetGameplayAttributeValueChangeDelegate()` 구독)는 Health/Mana와 같은 경로. `OnLevelChanged`/`OnKDAChanged`는 GAS Attribute가 아닌 `AP1PlayerState`의 plain int라 이 어트리뷰트 델리게이트 경로를 못 쓰므로, `BindCallbacksToDependencies()`에서 `AP1PlayerState`로 캐스트해 `OnCharacterLevelChangedNative`/`OnKDAChangedNative`(네이티브 델리게이트, PlayerState 섹션 참고)를 직접 구독한다. `BroadcastExperience()` 헬퍼가 Experience 변경 시**와** CharacterLevel 변경 시(XPRequired가 레벨마다 달라지므로) 양쪽에서 호출돼 `OnExperienceChanged(CurrentXP, XPRequired)`를 다시 쏜다 |
| `UP1FloatingStatusWidgetController` | 완료 | 4개 델리게이트(Health/MaxHealth/Mana/MaxMana). 캐릭터별 per-instance 생성 |

**에디터 작업 현황 (WBP):**
- WBP_SegmentedBar: 루트 Overlay + FillBar(ProgressBar) + ValueText + RegenText 배치 필요
- WBP_P1Overlay: HealthBar + ManaBar User Widget 배치 + FillColor/SegmentSize 설정 필요
- WBP_SkillIcon: SizeBox(고정 크기) → Overlay(SkillIconImage → CooldownOverlay → CooldownText 순서, z-order=배치 순서) 구조로 배치. `SkillIconImage`의 Brush에 마름모 마스크 머티리얼(`IconTexture` Texture Parameter, UV 맨해튼거리 `abs(u-0.5)+abs(v-0.5)` 기반 마스크를 Opacity로 출력) 설정 필요 — 없으면 사각형 아이콘으로 폴백. `CooldownOverlay`(M_CooldownMaterial, `CooldownPercent` Scalar Parameter) + `CooldownText`(Overlay 슬롯 Center/Center 정렬)도 배치 필요
- BP_P1HUD: OverlayWidgetClass=WBP_P1Overlay, OverlayWidgetControllerClass=P1OverlayWidgetController 설정 필요
- ArenaGameMode HUDClass=BP_P1HUD 설정 필요
- **WBP_P1Overlay 좌하단 플레이어 정보(신규)**: `LevelText`/`KDAText`/`GoldText`(TextBlock)+`ExperienceBar`(UP1SegmentedBarWidget 인스턴스) 4개 위젯을 정확히 이 이름으로 배치 필요 — 전부 BindWidgetOptional이라 없어도 크래시는 안 나고 해당 UI만 안 뜸. 시각적으로 묶고 싶으면 그냥 Overlay/VerticalBox로 감싸면 됨(C++은 이름만 찾으므로 컨테이너 중첩 깊이 무관). `ExperienceBar`는 `bShowLabels`로 숫자 표시 여부 선택 가능

### Camera (`Source/P1/Camera/`)

| 클래스 | 상태 | 비고 |
|---|---|---|
| `AP1PlayerCameraManager` | 완료 | FOV(80°) / Pitch 범위(-40°~+15°) 상수 정의 |
| `UP1CameraComponent` | 완료 | `UCameraComponent` 확장, `CameraModeStack` 관리, `DetermineCameraModeDelegate` |
| `UP1CameraMode` | 완료 | 카메라 모드 추상 베이스 (ViewOffset, FOV, 블렌딩 파라미터) |
| `UP1CameraModeStack` | 완료 | 모드 스택 관리, 블렌딩 평가 (Linear / EaseIn / EaseOut / EaseInOut) |
| `UP1HeroComponent` | 완료 | PawnComponent — `DefaultCameraMode` → `DetermineCameraModeDelegate` 연결 |

### 기타

| 항목 | 상태 | 비고 |
|---|---|---|
| `LogP1` 커스텀 로그 카테고리 | 완료 | `P1.h` / `P1.cpp` |
| Enhanced Input IMC / IA 에셋 | 완료 | BP_P1PlayerController에서 AbilityInputActions(TMap) + DefaultMappingContext 설정 |

---

## 알려진 버그 / 설계 결정 기록

| 항목 | 내용 |
|---|---|
| 어빌리티 중복 부여 방지 | `InitAbilityActorInfo`와 `AddDefaultAbilities` 분리. `PossessedBy`에서만 `AddDefaultAbilities` 호출 |
| LocalPredicted 이중 데미지 | `OnHitEventReceived`에 `IsNetAuthority()` 체크 추가 — 서버에서만 오버랩/데미지 실행 |
| **서버 +1 콤보 버그 (근본 원인)** | 커스텀 입력 라우팅(`AbilityInputTagReleased`)이 release를 서버에 전파하지 않아, 서버 인스턴스의 `InputReleased()`가 절대 호출되지 않음 → 서버가 버튼을 계속 눌린 걸로 오인 → 콤보 +1. **해결:** base ability `bReplicateInputDirectly=true` + ASC에서 non-authority일 때 `ServerSetInputPressed/Released()` 호출 (표준 `AbilityLocalInputPressed` 패턴) |
| 콤보 입력 방식 | hold-to-combo 지원. `bInputHeld` 플래그(ActivateAbility에서 true 시작, InputPressed/InputReleased로 갱신)로 추적. `OnHitEventReceived`에서 `bInputHeld`면 콤보 예약. 윈도우 중 재입력(tap)도 지원 |
| RMB 조준 장판이 Substrate Deferred Decal에서 안 보임 | Coverage 연결/Fade/Show Flag 등 여러 원인 시도 후에도 미해결 → `UDecalComponent` 자체를 포기하고 `UStaticMeshComponent`(평면) + 일반 Translucent Surface 머티리얼(Substrate Unlit BSDF, Emissive+Transmittance)로 교체해 해결 |
| RMB 도약이 포물선이 아니라 제자리/직진 | 1차 원인: 애니메이션에 수직 루트모션이 없어 MotionWarping이 아크를 만들 게 없었음 → `ApplyRootMotionJumpForce`(코드로 아크 생성)로 교체. 2차 원인: 몽타주의 루트모션이 켜져 있어 캐릭터를 제자리에 고정, JumpForce를 무효화 → 소스 AnimSequence들의 Root Motion을 꺼서 해결 |
| RMB 착지 판정이 공중에서 발동(캐릭터가 목표에 도달하기 전) | `ApplyRootMotionJumpForce`의 Duration을 몽타주 전체 길이로 줬더니, Land 노티파이가 타임라인 중간(약 60%)에 있어 노티파이 발동 시점에 캐릭터가 아직 도약 초반이었음. Duration을 "Land 노티파이의 실제 트리거 시점"(`FAnimNotifyEvent::GetTriggerTime()`)으로 바꿔 해결 — 오차가 649→94(캡슐 반높이 수준)로 감소 |
| 도약 중 LMB 입력 시 캐릭터가 목표를 지나쳐 멀리 날아감 | LMB 몽타주가 같은/겹치는 슬롯에서 RMB 몽타주를 인터럽트 → `OnLeapMontageInterrupted`가 `EndAbility` 호출 → `ApplyRootMotionJumpForce`가 아크 도중 강제 종료 → `MaintainLastRootMotionVelocity`로 잔여 속도가 일반 물리로 넘어가 무통제 비행. `UP1GameplayAbility_AssaultTheGates`가 `ActivationOwnedTags`로 `State.Attacking`을 소유해 도약 내내 LMB 발동 자체를 차단해 해결 |
| RMB 몽타주(Jump_Melee 스윙)가 재생되는데 화면에 안 보임 | GAS/AnimInstance 레벨에서는 정상 재생(로그로 세그먼트 진입 확인됨)됐지만, 몽타주가 AnimBP에서 실제로 참조하지 않는 `DefaultSlot`을 쓰고 있어 AnimGraph에 반영 안 됨. LMB 기본공격이 쓰는 `UpperBody` 슬롯은 하체를 Locomotion에 맡기므로 전신 도약 스킬에는 부적합 — 대신 Locomotion+UpperBody 블렌드 이후의 **FullBody 전용 Slot**을 AnimBP에 추가해 그 슬롯으로 교체해 해결 |
| `MeleeAttack_SacredOath`(어빌리티 변종)가 발동 자체를 안 함 | 로그의 `DynamicTags=(GameplayTags=)`(빈 값)와 `Default__P1GameplayAbility_MeleeAttack_SacredOath`(BP `_C` 접미사 없음)로 원인 특정 — **C++ 클래스를 Blueprint화하지 않고 `DefaultAbilities`에 직접 넣으면**, `InputTag`가 비어있어 `AddDefaultAbilities()`가 `DynamicSpecSourceTags`를 못 채우고, 입력 라우팅이 이 스펙을 영원히 못 찾는다. **어빌리티 변종은 반드시 Blueprint(부모=C++ 서브클래스)로 만들고 `InputTag`를 평소 버전과 동일하게 명시적으로 설정**해야 한다 (BP 상속이 아니라 별도 BP라 InputTag가 자동으로 안 넘어옴) |
| 코스메틱 효과 되돌리기를 "버프 태그 존재 여부"로 판단하면 경쟁 상태 발생 | Sacred Oath 검 발광을 "태그가 아직 있으면 자연만료로 원복" 방식으로 짰더니, GE 자체의 Duration 만료가 원복 타이머와 거의 동시에 태그를 스스로 지워버려 "조기소모로 이미 원복됨"과 "자연만료됐지만 아무도 원복 안 함"을 구분 못 함 → 무한 지속 버그. **1차 해결(임시)**: 태그 체크 없이 무조건 원복. **최종 해결**: 타이머 자체를 없애고 `RegisterGameplayTagEvent`로 태그 카운트 변경 이벤트를 구독 — 아래 항목 참고 |
| "강화된 기본공격" 어빌리티 변종이 같은 InputTag에서 평소 버전과 경쟁하는 구조 자체가 근본 원인이었던 다중 버그 | `MeleeAttack_SacredOath`를 평소 `MeleeAttack`과 같은 `InputTag.Ability.BasicAttack`에 `ActivationRequiredTags`/`ActivationBlockedTags`로 상호 배타 게이팅해 등록했더니: (1) Blueprint화 누락으로 `DynamicTags` 비어 발동 자체 불가(→ BP 생성으로 해결), (2) 로그 정밀 분석 결과 BP 수정 후에도 여전히 `TryActivateAbility result: 0`이 계속 나옴 → 원인은 **클라이언트 예측 GE가 서버 authoritative 버전과 조율되는 타이밍 문제**: 클라이언트 자체 시계로 버프 적용 후 겨우 ~1.2초 만에(5초 Duration인데도) `Buff.SacredOath.Active`가 로컬에서 사라진 것처럼 보임 — `SacredOath`(E)의 `CastMontage`가 짧게 끝나 클라 인스턴스가 `EndAbility`를 호출하는 시점이 서버의 authoritative 확인보다 빠르면 예측된 GE가 조기에 씻겨나갈 수 있음. 부가적으로 (3) 같은 `AbilityInputTagPressed` 루프 안에서 먼저 시도돼 성공한 어빌리티가 `State.Attacking`(전 어빌리티 공통 소유 태그)을 즉시 걸어버려 같은 루프의 나머지 후보를 오염시킬 수 있는 구조적 위험도 발견. **최종 해결**: 어빌리티 스왑(경쟁) 구조 자체를 폐기 — Sacred Oath 시너지가 있는 영웅은 아예 `MeleeAttack_SacredOath`를 자신의 유일한 BasicAttack으로 등록하고, 버프 판정은 액티베이션 시점이 아니라 **히트 적용 시점**에 어빌리티 내부에서 직접 체크. 이러면 위 세 문제가 전부 구조적으로 발생 불가능해진다(경쟁할 두 번째 어빌리티가 없으므로) |
| Stoicism 디플렉트가 매번 발동(쿨다운이 전혀 안 걸림) | 로그에서 `Default__P1GameplayAbility_StoicismDeflect`(BP `_C` 접미사 없음)로 원인 특정 — `MeleeAttack_SacredOath` 때와 완전히 같은 실수(위 항목 참고): raw C++ 클래스가 `DefaultAbilities`에 직접 들어가 에디터에서 설정한 `CooldownGameplayEffectClass`가 반영 안 됨. `CommitAbilityCooldown()`은 쿨다운 GE가 없으면 "적용할 게 없다"는 이유로 그냥 `true`(성공)를 반환하기 때문에, 로그만 봐서는 "커밋에 성공했는데 왜 쿨다운이 안 걸리지"처럼 보여서 원인 파악이 헷갈리기 쉬움 — **`OnCooldownTagChanged` 같은 태그 이벤트 리스너 로그가 한 번도 안 찍힌다면 태그 자체가 안 붙고 있다는 뜻**이므로 이럴 때 BP 등록 여부부터 의심할 것 |
| 상시 패시브(`GiveAbility`만으로 부여)가 영원히 실행되지 않음 | `UP1GameplayAbility_StoicismVitality`가 반복 타이머를 `ActivateAbility` 안에서 시작하는데, `AddDefaultAbilities()`가 `ASC->GiveAbility()`로만 부여해서 `ActivateAbility` 자체가 한 번도 안 불림 — 입력 바인딩도 없고(`InputTag` 없음) `AbilityTriggers`도 없는 어빌리티는 `GiveAbility`만으로는 절대 활성화되지 않는다(부여=활성화가 아님). **해결**: 베이스 `UP1GameplayAbility`에 `bActivateOnGranted` 플래그를 추가하고, `AddDefaultAbilities()`가 이 플래그를 보고 `GiveAbility` 대신 `GiveAbilityAndActivateOnce`로 부여하도록 분기 — 이 어빌리티에서 처음 발견됐지만 앞으로 "입력/트리거 없이 상시 작동해야 하는" 모든 패시브에 재사용 가능한 범용 플래그 |
| `UAttributeSet::GetOwningActor()`가 Pawn이 아니라 PlayerState를 반환 | `HandleKillRewards()`에서 처음엔 `Cast<AP1HeroCharacter>(GetOwningActor())`로 작성했으나 이 ASC는 PlayerState에 있으므로(`GetOwningActor()`=ASC의 `OwnerActor`=`AP1PlayerState`) 항상 null이 됨 — `Cast<AP1PlayerState>(GetOwningActor())->GetPawn()`으로 먼저 PlayerState를 얻은 뒤 Pawn을 가져와야 한다. 빌드 전에 자체 발견/수정 |
| **CurveTable JSON 임포트 시 "Key 'Keys' on row 'X' is not a float and cannot be parsed" 에러** | `{"Name": "Row", "Keys": [{"Time":X,"Value":Y}, ...]}` 형태(=`UCurveFloat` 에셋을 익스포트했을 때 나오는 내부 직렬화 포맷)로 `Data/CT_*.json`을 전부 작성했었는데, 이건 **CurveTable 임포터가 기대하는 포맷이 아니다** — CurveTable은 CSV와 동일한 구조로 "Row 하나 = 커브 하나, 시간값이 컬럼 헤더"여야 한다: `[{ "Name": "RowName", "1": 42, "2": 98, ... }]` (컬럼 키가 시간, 값이 float). `Data/` 하위 모든 `CT_*.json`을 이 포맷으로 재작성해 해결 |
| GE Custom Calculation Class의 에셋 참조 프로퍼티가 항상 비어있음 | `UGameplayModMagnitudeCalculation` 서브클래스(`UP1MMC_GoldKillBounty`)의 `KillStreakBonusTable`(`UCurveTable*`) 같은 `EditDefaultsOnly` 프로퍼티는, GE가 그 네이티브 C++ 클래스를 Custom Calculation Class로 직접 참조하면 **CDO 기본값(null)**이 그대로 쓰인다 — 네이티브 클래스의 CDO는 콘텐츠 브라우저에서 값을 편집할 방법이 없기 때문. 다른 "BP화 필요" 사례(`MeleeAttack_SacredOath`, `StoicismDeflect`)와 같은 근본 원인의 변형: **이 MMC를 상속하는 블루프린트(`MMC_GoldKillBounty`)를 만들어 그 Class Defaults에서 값을 지정하고, GE의 Calculation Class로는 이 BP를 선택**해야 한다 |

---

## 다음 작업 예정

**스킬 어빌리티 (순차 진행 — 데미지는 ExecCalc_Damage 공유, 도약류는 ApplyRootMotionJumpForce 패턴 재사용):**
- [x] 공유 기반: `Damage` 메타 어트리뷰트, `UP1ExecCalc_Damage`(계수 채널 방식), `Character.Type` 태그
- [x] **RMB (Assault The Gates)**: 완료, 풀 테스트 통과 (조준 장판/포물선 도약/착지 판정/AnimBP FullBody 슬롯까지 전부 해결)
- [x] **RMB 이펙트**: 착지 파티클 적용 완료. 검 화염(스킬 사용 중 무기 슬롯 머티리얼 교체)은 `UP1AnimNotifyState_MaterialOverride`로 C++ 완료 — 에디터에서 검 슬롯용 발광 머티리얼 제작/지정 필요 (Paragon 기존 에셋 여러 개 시도했으나 GPU 파티클 전용/파라미터 없음 등으로 부적합, 커스텀 제작 필요)
- [x] **E (Sacred Oath)**: C++ 완료 (에셋 대기) — 버프(`GA_SacredOath`) + 강화된 MeleeAttack. 아래 "에디터 작업 필요" 참고
- [x] **Q (Make Way)**: C++ 완료 (에셋 대기) — 반복 타이머 기반 지속 회오리. 아래 "에디터 작업 필요" 참고
- [x] **R (Stone Forged Soul)**: C++ 완료 (에셋 대기, **카메라 연출은 별도 작업으로 보류**) — 2.5초 스테이시스(회복+무적+근접도 슬로우) → 착지 시 범위 물리피해. 아래 "에디터 작업 필요" 참고
- [x] **패시브 (Stoicism)**: C++ 완료 (에셋 대기) — 서로 성격이 달라 어빌리티 2개로 분리(디플렉트=반응형 쿨다운, 재생+방어력=상시 틱). 아래 "에디터 작업 필요" 참고
- [x] **전투 보상/레벨업 시스템**: C++ 완료, PIE 실전 검증(킬 시 Experience 획득 로그 확인). 아래 "전투 보상/레벨업 에디터 작업" 참고
- [ ] 정글 몬스터 / AI 캐릭터 (**미니언 없는 게임으로 최종 설계 변경** — 3라인+정글 에셋을 구하기 어려워 던전형 맵에 정글몬스터만 배치하는 방향. `AP1MinionCharacter`는 만들지 않음. 몬스터는 스폰 시점마다 매치 경과 시간에 따라 "레벨"이 갱신되고 그에 따라 골드/경험치 보상도 스케일링 — 데이터(`Data/CT_MonsterLevelByMatchTime.json`, `Data/CT_MonsterRewardMultiplier.json`, `Data/DT_MonsterGoldXP.json`)는 준비됐지만 이를 소비하는 몬스터 액터/AI 자체는 아직 미구현. `RiftHerald`/`ElementalDrake`/`BaronNashor`는 던전 맵 레이아웃에 실제로 들어갈지 미정이라 낮은 우선순위)
- [x] **로비(PreGame) + 백엔드 매치메이킹**: C++/백엔드 코드 완료(컴파일 검증됨) — 아래 "로비/백엔드 에디터 작업" 참고. `AP1GameState` 확장(라운드/팀 점수 등)은 여전히 미착수
- [ ] 매치 흐름 (로비 → 인게임 → 결과 화면, `AP1GameState` 확장)
- [ ] 아레나 맵 (콜로세움 에셋 임포트 및 레벨 구성)

**전투 보상/레벨업 에디터 작업 (일부 완료, 남은 항목만 표시):**
- [x] `GE_ExperienceReward_Flat`, `GE_GoldReward_Kill`(+`MMC_GoldKillBounty` BP), `GE_GoldReward_Flat` 생성 완료
- [x] `CT_ChampionKillXP`/`CT_ChampionAssistXP`(챔피언 킬/어시스트 경험치, Row=피해자 레벨 1~18) CurveTable 임포트 및 BP_Greystone의 `ChampionKillXPTable`/`ChampionAssistXPTable`에 지정 — PIE로 검증됨
- [ ] `CT_XPToNextLevel`(레벨업 요구 경험치)을 BP_P1PlayerState의 `XPToNextLevelTable`에 지정 — 안 해도 XP는 정상 누적되고 레벨업만 안 일어남(안전 폴백)
- [ ] `GE_ManaRestore_Instant`(Duration=Instant, Mana Modifier=Additive, Magnitude=SetByCaller `Data.Mana.Flat`) 생성 — `HealEffectClass`(`GE_Heal`, 기존 존재)는 이미 있지만 마나 버전은 아직 없음
- [ ] BP_Greystone에 `HealEffectClass`/`ManaRestoreEffectClass`/`GoldRewardKillEffectClass`/`GoldRewardFlatEffectClass`/`ExperienceRewardEffectClass` 전부 지정 확인
- [ ] **WBP_P1Overlay 좌하단 플레이어 정보 위젯 배치** — 위 UI 섹션 "에디터 작업 현황 (WBP)" 참고 (`LevelText`/`KDAText`/`GoldText`/`ExperienceBar`)

**로비(PreGame)/백엔드 에디터 작업 (C++/백엔드 코드는 완료, 에디터 자산이 전부 대기 중):**
- [ ] 백엔드: `docker compose up -d`(MySQL, `Backend/docker-compose.yml`) → `./mvnw spring-boot:run`(또는 IntelliJ) → curl/Postman으로 signup×2 → login×2 → queue×2 → `MATCHED` 확인 (Docker/실행은 사용자가 직접 — Claude는 Docker 명령을 실행하지 않음)
- [ ] 새 Level(`Content/Maps/L_PreGame.umap` 등) 생성 — World Settings → GameMode Override = `BP_P1LobbyGameMode`
- [ ] `BP_P1LobbyGameMode`(parent=`AP1LobbyGameMode`), `BP_P1LobbyPlayerController`(parent=`AP1LobbyPlayerController`, `LobbyWidgetClass`=`WBP_Lobby` 지정) 생성
- [ ] `WBP_Lobby`(parent=`UP1LobbyWidget`) 생성 — `UsernameBox`/`PasswordBox`(EditableTextBox)+`LoginButton`/`SignupButton`(필수 BindWidget), `QueueButton`/`StatusText`/`LoginPanel`/`QueuePanel`(선택) 배치. `LoginPanel`/`QueuePanel`은 아무 컨테이너 위젯이나 가능(로그인 성공 시 자동으로 Visibility 전환됨)
- [ ] Project Settings → Maps & Modes → Game Default Map을 `L_PreGame`으로 변경 (현재 `L_Battle_Arena_Day`로 직접 시작하도록 돼 있음 — `GlobalDefaultGameMode`=`BP_P1ArenaGameMode`는 그대로 유지, PreGame 레벨만 자체 World Settings로 오버라이드)
- [ ] `DefaultGame.ini`에 `[/Script/P1.P1BackendSubsystem]` 섹션 추가해 `BackendBaseUrl` 확인/조정(기본값은 C++에 `http://127.0.0.1:8080`으로 하드코딩돼 있어 로컬 테스트는 설정 없이도 동작)
- [ ] PIE 2클라이언트로 로그인→매칭→`ClientTravel`까지 엔드투엔드 검증 (Arena 맵을 `127.0.0.1:7777`에서 데디케이티드/리슨 서버로 미리 띄워둬야 함 — 백엔드 `match.server-address`와 일치시킬 것)

**E(Sacred Oath) 에디터 작업 필요:**
- `IA_E` 입력 액션 생성 → PlayerController `AbilityInputActions`에 `InputTag.Ability.E` 매핑
- `GE_Buff_SacredOath` (Duration 5초): `Buff.SacredOath.Active` 태그 부여 + `AttackRange` Modifier (GameplayCue 등록 불필요 — Multicast 방식으로 전환됨)
- `GE_Debuff_MovementSlow` (Duration 1.25초): `MovementSpeed` Modifier가 SetByCaller `Data.DebuffMagnitude` 참조, `Debuff.MovementSlow` 태그 부여
- `GA_SacredOath` BP (← `UP1GameplayAbility_SacredOath`): BuffEffectClass/Cooldown/Cost/CastMontage만 지정하면 끝 — **`WeaponGlowMaterial`/`WeaponMaterialSlotName` 프로퍼티는 이제 이 어빌리티에 없음**(코스메틱은 컴포넌트가 전담)
- **그레이스톤의 BasicAttack 어빌리티를 `GA_Lightbringer_SecredOath`(← `UP1GameplayAbility_MeleeAttack_SacredOath`, 기존 BP 재사용) 하나로 통일**: `AP1HeroCharacter::DefaultAbilities`에서 평소 `GA_Lightbringer`는 **제거**하고 이 BP만 등록(같은 InputTag에 두 개를 나란히 두지 않음). `ComboMontages`/`AttackHalfHeight` 등은 기존 `GA_Lightbringer`와 동일하게(이미 설정돼 있다면 유지) + `SlowDebuffEffectClass`=`GE_Debuff_MovementSlow` + **`TrailParticleTemplate`=궤적 파티클, `TrailSocketName`=검 소켓**(매 스윙마다 재생되므로 이 클래스가 소유 — 컴포넌트가 아니라 여기 남는 이유는 "태그 상태"가 아니라 "스윙 이벤트"에 반응해야 하기 때문)
- **BP_Greystone(`AP1HeroCharacter`)에 `UP1BuffCosmeticEffectComponent` 컴포넌트를 추가**하고 `Effects` 배열에 항목 1개 등록: `BuffTag`=`Buff.SacredOath.Active`, `MaterialSlotName`=검 슬롯(예: `M03_SwordShieldFur_skin03`), `OverrideMaterial`=`MI_Sword_Fire`. 이 컴포넌트가 없으면 검 발광이 전혀 동작하지 않는다(예전처럼 `GA_SacredOath`나 `AP1HeroCharacter`에 직접 설정하는 방식이 아님)
- **`GE_Buff_SacredOath`는 반드시 Tags → Granted Tags에 `Buff.SacredOath.Active`를 실제로 추가해야 한다** — AttackRange 모디파이어만 설정하고 이 태그 부여를 빠뜨리면, 사거리 버프는 5초간 정상 작동하는 것처럼 보이지만 강화 공격 판정(`ApplyComboHitDamage`의 `HasMatchingGameplayTag` 체크)과 `UP1BuffCosmeticEffectComponent`의 리스너(태그 카운트가 0→양수로도 전혀 안 바뀜)가 둘 다 조용히 무력화된다 — 실제로 이 프로젝트에서 겪은 버그이므로 GE 에셋 생성/수정 시 반드시 확인할 것
- BP_Greystone(`AP1HeroCharacter`)에 **`WeaponGlowMaterialSlotName`=검 슬롯 이름**(예: `M03_SwordShieldFur_skin03`) 설정 — `GA_SacredOath`와 `MeleeAttack_SacredOath` 양쪽에 중복돼 있던 슬롯 이름 프로퍼티를 캐릭터 하나로 통합
- `AP1HeroCharacter::DefaultAbilities`에 `GA_SacredOath` 추가

**Q(Make Way) 에디터 작업 필요:**
- `IA_Q` 입력 액션 생성 → PlayerController `AbilityInputActions`에 `InputTag.Ability.Q` 매핑
- `GE_Debuff_ArmorShred` (Duration 5초, **Stacking Type=AggregateBySource, Stack Limit=4**): `PhysicalArmor` Modifier가 SetByCaller `Data.DebuffMagnitude` 참조(누적 감소), `Debuff.ArmorShred` 태그 부여
- `GA_MakeWay` BP (← `UP1GameplayAbility_MakeWay`): `DamageEffectClass`=공용 `GE_Damage`, `ArmorShredDebuffEffectClass`=`GE_Debuff_ArmorShred`, `CooldownGameplayEffectClass`/`CostGameplayEffectClass` 지정 + **`WhirlwindParticleTemplate`=회오리 파티클**, `WhirlwindSocketName`=본 소켓(비워두면 루트 컴포넌트에 부착 — 회오리는 보통 이쪽이 자연스러움)
- `AP1HeroCharacter::DefaultAbilities`에 `GA_MakeWay` 추가

**R(Stone Forged Soul) 에디터 작업 필요:**
- `IA_R` 입력 액션 생성 → PlayerController `AbilityInputActions`에 `InputTag.Ability.R` 매핑
- **캐스팅 몽타주 원본 애니메이션에 실제 수직 루트모션(상승→공중대기→하강)이 있는지 먼저 확인** — 없으면 코드 기반 이동으로 다시 설계해야 함(RMB의 JumpForce는 가변 목표 지점 때문에 필요했던 것이라 그대로 재사용은 안 되고, 고정 왕복 이동에 맞는 별도 방식 필요). 이 몽타주에 착지 프레임 AnimNotify로 `Event.Montage.StoneForgedSoul.Crash` 이벤트 발신(`UP1AnimNotify_SendGameplayEvent` 재사용) 삽입 필수 — 없으면 착지 데미지도, 비행 모드 해제도 영원히 안 일어남
- `GE_Heal_Instant` (또는 이름 자유): Duration=Instant, `Health` Modifier=Additive, Magnitude=SetByCaller `Data.Heal.Flat`
- `GE_Buff_Invulnerable` (또는 이름 자유): Duration=Has Duration 2.5초(레벨 무관 고정값 — 몽타주 길이와 일치해야 함), Granted Tags에 `State.Invulnerable` 추가
- `GE_Debuff_StasisSlow` (또는 이름 자유): Duration=Has Duration 0.4초 안팎(재스캔 간격 `SlowTickPeriod`보다 살짝 길게, 끊김 없이 갱신되도록), **Stacking Type=AggregateByTarget, Stack Limit=1**(누적이 아니라 매번 최신 값으로 덮어쓰기), `MovementSpeed` Modifier가 SetByCaller `Data.DebuffMagnitude` 참조, `Debuff.MovementSlow` 태그 부여(Sacred Oath 슬로우와 태그 공유 — 새 태그 안 만듦)
- `GE_Cooldown_StoneForgedSoul`: Duration=Has Duration, Modifier Magnitude=**Scalable Float**(커브테이블에 레벨 1/2/3 → 150/135/120), `Cooldown.Ability.StoneForgedSoul` 태그 부여 — **AssaultTheGates와 달리 동적 쿨감 요구사항이 없으므로 SetByCaller 없이 표준 `CommitAbility()`/`CooldownGameplayEffectClass`로 충분**
- `GE_Cost_StoneForgedSoul`: Mana -= 100 (고정값)
- `GA_StoneForgedSoul` BP (← `UP1GameplayAbility_StoneForgedSoul`): `CastMontage`, `HealEffectClass`=`GE_Heal_Instant`, `InvulnerabilityEffectClass`=`GE_Buff_Invulnerable`, `SlowDebuffEffectClass`=`GE_Debuff_StasisSlow`, `CooldownGameplayEffectClass`=`GE_Cooldown_StoneForgedSoul`, `CostGameplayEffectClass`=`GE_Cost_StoneForgedSoul`, `DamageEffectClass`=공용 `GE_Damage`. `HealPercent`(FScalableFloat, 커브테이블 레벨 1/2/3 → 0.12/0.14/0.16), `FlatDamage`(FScalableFloat, 커브테이블 레벨 1/2/3 → 240/370/500), `PhysicalPowerCoefficient`=1.8(180%) 지정
- `AP1HeroCharacter::DefaultAbilities`에 `GA_StoneForgedSoul` 추가
- **카메라 연출(스킬 사용 시 역동적 카메라 이동)은 별도 작업으로 보류** — 사용자가 명시적으로 후순위로 미룸

**패시브(Stoicism) 에디터 작업 필요:**
- `GE_Cooldown_StoicismDeflect`: Duration=Has Duration, Modifier Magnitude=**Scalable Float**(커브테이블에 레벨 1~18 → 15/14.4/13.8/.../4.8, 영웅 레벨 스케일링이라 아직 레벨업 시스템이 없다면 당장은 레벨 1=15초 고정값으로 시작해도 무방), `Cooldown.Ability.StoicismDeflect` 태그 부여
- `GA_StoicismDeflect` BP (← `UP1GameplayAbility_StoicismDeflect`): `CooldownGameplayEffectClass`=`GE_Cooldown_StoicismDeflect`만 지정하면 끝(Cost 없음)
- `GE_Heal_Instant`는 R에서 이미 만들었다면 그대로 재사용 가능(Duration=Instant, `Health` += SetByCaller `Data.Heal.Flat`)
- `GE_Buff_StoicismArmor` (또는 이름 자유): Duration=Has Duration(1.5~2초 안팎, 재적용 주기보다 살짝 길게), **Stacking Type=AggregateByTarget, Stack Limit=1**(누적 아니라 최신 값으로 덮어쓰기), `PhysicalArmor` Modifier가 SetByCaller `Data.ArmorBonus.Flat` 참조
- `GA_StoicismVitality` BP (← `UP1GameplayAbility_StoicismVitality`): `HealEffectClass`=`GE_Heal_Instant`, `ArmorBuffEffectClass`=`GE_Buff_StoicismArmor`. `RegenPercentPerSecond`(기본 0.0015=0.15%)/`BaseArmorBonus`(기본 4)/`ArmorBonusPerLevel`(기본 1)/`LowHealthThreshold`(기본 0.5) 필요 시 조정
- `AP1HeroCharacter::DefaultAbilities`에 `GA_StoicismDeflect`, `GA_StoicismVitality` 둘 다 추가 — 패시브라 InputTag 없이(둘 다 `InputTag` 미설정 상태로 두면 됨) 그냥 상시 부여되는 어빌리티로만 등록
