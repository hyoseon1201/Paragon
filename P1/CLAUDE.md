# CLAUDE.md

이 파일은 **P1** 언리얼 엔진 프로젝트에서 작업할 때 Claude Code가 참고하는 가이드입니다.

## 엔진 / 프로젝트 정보

- 엔진 버전: **5.8** (`P1.uproject` 참고)
- 메인 모듈: `P1` (Runtime, Default 로딩 페이즈)
- 빌드 파일: `Source/P1/P1.Build.cs`, `Source/P1.Target.cs`, `Source/P1Editor.Target.cs`
- 활성화된 플러그인: ModelingToolsEditorMode, ModelContextProtocol, Terminal, EditorToolset, GameplayAbilities, ModularGameplay, **MotionWarping** (도약/대시 스킬용)

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

### Player (`Source/P1/Player/`)

| 클래스 | 상태 | 비고 |
|---|---|---|
| `AP1PlayerState` | 완료 | ASC + AttributeSet 호스팅, Mixed 리플리케이션, `IGenericTeamAgentInterface` (TeamId 리플리케이션) |
| `AP1PlayerController` | 완료 | Enhanced Input (Move/Look/Jump + AbilityInputActions TMap), `AP1PlayerCameraManager` 지정 |

### Characters (`Source/P1/Characters/`)

| 클래스 | 상태 | 비고 |
|---|---|---|
| `AP1CharacterBase` | 완료 | Abstract 베이스, `IAbilitySystemInterface` + `IGenericTeamAgentInterface`, CachedASC 포인터, `IsSameTeam()` 헬퍼, `CharacterType` 태그(기본 Hero) + `IsHeroOrBoss()` |
| `AP1HeroCharacter` | 완료 | `InitAbilityActorInfo()` (ASC 초기화만) + `AddDefaultAbilities()` (서버 전용, PossessedBy에서 호출) 분리. `DefaultAttributesEffect` 적용, `MovementSpeed` 어트리뷰트 바인딩 |
| `AP1MinionCharacter` | 미구현 | 코드베이스에 없음 |

### Ability System (`Source/P1/AbilitySystem/`)

| 클래스 | 상태 | 비고 |
|---|---|---|
| `UP1AbilitySystemComponent` | 완료 | InputTag → Ability press/release 라우팅. `bReplicateInputDirectly` 서버 전파. 조준 중(`State.TargetingAbility`) LMB=confirm/RMB=cancel 리라우팅 + 나머지 차단 |
| `UP1AttributeSet` | 완료 | 아래 어트리뷰트 전체 구현 — 리플리케이션, 클램핑 포함 |
| `UP1GameplayTags` | 완료 | Native 태그: `InputTag.Ability.BasicAttack`, `Ability.BasicAttack`, `State.Attacking`, `Event.Montage.BasicAttackHit`, `Data.DamageMultiplier`, `Data.Damage.Flat`, `Data.Damage.PhysicalPower`, `Character.Type.Hero/Boss/Minion` |
| `UP1GameplayAbility` (베이스) | 완료 | `InstancedPerActor` + `LocalPredicted` + `bReplicateInputDirectly=true` 기본값, `GetP1Character/PlayerController` 헬퍼 |
| `UP1DamageGameplayAbility` | 완료 | 데미지 어빌리티 공통 베이스 — `DamageEffectClass` + `FlatDamage`/`PhysicalPowerCoefficient` + `ApplyDamageToTarget()` (계수 채널 + `Data.DamageMultiplier` SetByCaller 전달) |
| `UP1ExecCalc_Damage` | 완료 | 물리 데미지 ExecutionCalculation. 계수 **채널 합산**: `Raw=(Flat + PhysCoeff*PhysicalPower)`, `Pre=Raw*Mult` × 방어 감산(`Armor/(Armor+100)`, Penetration flat 차감) → `Damage` 메타. 미래: MagicalPower/TargetMaxHealthPct 채널 |
| `UP1GameplayAbility_MeleeAttack` | 완료 | 콤보 기본공격. AnimNotify → `BasicAttackHit` 이벤트 → 서버 전용 구체 오버랩 → 데미지. 콤보 큐는 `InputPressed()`에서만 처리 |
| `UP1GameplayAbility_AssaultTheGates` (RMB) | C++ 완료 (에셋 대기) | 2단계 지면조준 도약. 조준(WaitTargetData+GroundDecal, 미커밋) → 확정 시 코스트 소모 + 서버 쿨다운 → MotionWarp 도약 → 착지 AOE 데미지 → 영웅/보스 적중 시 이속버프+쿨다운 35%↓ |
| `AP1TargetActor_GroundDecal` | 완료 | 지면 조준 타겟액터. 카메라 트레이스+사거리 클램프+장판 데칼, 확정 위치를 LocationInfo 타겟데이터로 서버 복제 |
| `UP1AnimNotify_SendGameplayEvent` | 완료 | 범용 GameplayEvent 발신 AnimNotify — `EventTag` 프로퍼티로 태그 지정 |
| GameplayEffect 데이터 에셋 | 부분 완료 | `GE_BasicAttackDamage` BP 에셋 생성됨. `DefaultAttributesEffect` 별도 생성 필요 |

**AttributeSet 어트리뷰트 목록:**

| 카테고리 | 어트리뷰트 |
|---|---|
| Vital | `Health`, `MaxHealth`, `Mana`, `MaxMana`, `HealthRegen`, `ManaRegen` |
| Combat | `PhysicalPower`, `AttackSpeed`, `BasicAttackTime`, `AttackRange`, `Cleave` |
| Combat | `PhysicalArmor`, `MagicalArmor`, `PhysicalPenetration`, `MagicalPenetration` |
| Combat | `LifeSteal`, `Tenacity`, `AbilityHaste` |
| Movement | `MovementSpeed` |
| Meta (transient) | `Damage` — ExecCalc_Damage 출력 누적, PostGameplayEffectExecute에서 Health로 변환 (비복제) |

### UI (`Source/P1/UI/`)

디렉토리 구조: `UI/HUD/`, `UI/Widget/`, `UI/WidgetController/`

**Widget:**

| 클래스 | 상태 | 비고 |
|---|---|---|
| `UP1UserWidget` | 완료 | 모든 위젯 베이스. `SetWidgetController()` → `OnWidgetControllerSet()` 패턴 |
| `UP1SegmentedBarWidget` | 완료 | `UProgressBar`(FillBar) + NativePaint 구분선. `SetValues(current, max, regen)`. `SegmentSize`/`DividerColor` EditAnywhere |
| `UP1SkillIconWidget` | 완료 | 스킬 아이콘. `CooldownOverlay`(UImage, MID로 CooldownPercent 파라미터), `CooldownText`. `StartCooldown()`/`ClearCooldown()`. NativeTick으로 0.1s 단위 업데이트 |
| `UP1FloatingStatusWidget` | 완료 | 캐릭터 머리 위 체력/마나 위젯. `OnWidgetControllerSet()`에서 FloatingStatusWidgetController 델리게이트 바인딩 |

**HUD:**

| 클래스 | 상태 | 비고 |
|---|---|---|
| `UP1HUDWidget` | 완료 | 메인 HUD. HealthBar/ManaBar(UP1SegmentedBarWidget), SkillIcon_Q/E/R/RMB/Passive/LMB/Flash(UP1SkillIconWidget). OnWidgetControllerSet에서 6개 어트리뷰트 델리게이트 바인딩 |
| `AP1HUD` | 완료 | 위젯 컨트롤러 레지스트리. `InitOverlay()` → WBP 생성 + SetWidgetController + BroadcastInitialValues |

**WidgetController:**

| 클래스 | 상태 | 비고 |
|---|---|---|
| `UP1WidgetController` | 완료 | 베이스. `FWidgetControllerParams`(PC/PS/ASC/AS). `FOnAttributeChangedSignature` 델리게이트 정의 |
| `UP1OverlayWidgetController` | 완료 | 6개 델리게이트(Health/MaxHealth/HealthRegen/Mana/MaxMana/ManaRegen). ASC 어트리뷰트 변경 구독 |
| `UP1FloatingStatusWidgetController` | 완료 | 4개 델리게이트(Health/MaxHealth/Mana/MaxMana). 캐릭터별 per-instance 생성 |

**에디터 작업 현황 (WBP):**
- WBP_SegmentedBar: 루트 Overlay + FillBar(ProgressBar) + ValueText + RegenText 배치 필요
- WBP_P1Overlay: HealthBar + ManaBar User Widget 배치 + FillColor/SegmentSize 설정 필요
- WBP_SkillIcon: SkillIconImage + CooldownOverlay(M_CooldownMaterial 설정) + CooldownText 배치 필요
- BP_P1HUD: OverlayWidgetClass=WBP_P1Overlay, OverlayWidgetControllerClass=P1OverlayWidgetController 설정 필요
- ArenaGameMode HUDClass=BP_P1HUD 설정 필요

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

---

## 다음 작업 예정

**스킬 어빌리티 (순차 진행 — 이동은 MotionWarping+루트모션, 데미지는 ExecCalc_Damage 공유):**
- [x] 공유 기반: `Damage` 메타 어트리뷰트, `UP1ExecCalc_Damage`, `Data.Damage` 태그, MotionWarping 모듈
- [~] **RMB (Assault The Gates)**: C++ 완료(타겟액터+어빌리티+입력리라우팅). 에디터 에셋 대기 — GA/타겟액터 BP, 도약 몽타주(MotionWarp+Land 노티파이), GE(Damage/Cooldown/Cost/이속버프), 데칼 머티리얼, IA_RMB 입력
- [ ] **Q**: 4초간 캐릭터 주변 불타는 회오리(따라다님) 범위 물리피해 + 물리방어 감소 디버프(최대 4중첩)
- [ ] **E**: 5초간 다음 기본공격 1회 강화(사거리↑, 추가 물리피해, 해당 공격 Cleave=100%) + 범위 적 1.25초 이동속도 감소
- [ ] **R**: 2.5초 상승(체력 회복 + 주변 이동속도 감소 디버프) → 낙하 충돌 시 범위 물리피해. 역동적 카메라
- [ ] **패시브 어빌리티** 구현
- [ ] 미니언 / AI 캐릭터 (`AP1MinionCharacter`, AIController)
- [ ] 매치 흐름 (로비 → 인게임 → 결과 화면, `AP1GameState` 확장)
- [ ] 아레나 맵 (콜로세움 에셋 임포트 및 레벨 구성)
