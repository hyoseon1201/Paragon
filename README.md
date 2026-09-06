# P1

Unreal Engine 5.8로 제작한 데디케이티드 서버 기반 MOBA 아레나 프로젝트입니다.  
1인 개발로 로그인·매칭부터 영웅 전투, 정글 몬스터, 상점, 결과 화면과 로비 복귀까지 멀티플레이어 게임의 기본 흐름을 구현했습니다.

## 프로젝트 개요

| 항목 | 내용 |
|---|---|
| 개발 기간 | 2026.06 - 2026.09 / 약 10주 |
| 개발 인원 | 1인 |
| 장르 | 3팀 규모 MOBA 아레나 |
| 엔진 | Unreal Engine 5.8 |
| 주요 언어 | C++ |
| 서버 구조 | Unreal Dedicated Server |
| 관련 백엔드 | Java 21, Spring Boot, MySQL |

## 주요 기능

- 로그인, 영웅 선택, 매칭 신청, 게임 서버 접속
- 2종 영웅의 기본 공격 콤보와 스킬 전투
- GAS 기반 비용, 쿨다운, 상태 태그, Gameplay Effect
- 정글 몬스터 캠프와 Behavior Tree 기반 AI
- 경험치, 레벨업, 스킬 포인트, 킬·데스·어시스트
- 아이템 상점과 인벤토리
- HUD, 머리 위 체력바, 미니맵, 스코어보드
- 킬 스코어 기반 매치 종료, 결과 화면, 로비 복귀
- 헤드리스 봇을 이용한 멀티플레이어 부하 테스트

## 담당 및 구현 방향

게임플레이 클라이언트와 전투 시스템, UI, AI, 네트워크, 백엔드를 전반적으로 구현했습니다.  
Blueprint 이벤트 그래프에 기능 구현이 집중되면 수정과 재사용의 병목이 커질 수 있다고 판단해, 게임플레이 규칙과 UI 흐름은 C++로 작성하고 Blueprint는 에셋과 데이터 설정 중심으로 사용했습니다.

## 클라이언트 구현

### GAS 기반 전투

Gameplay Ability, Gameplay Effect, Gameplay Tag를 이용해 스킬 입력, 발동 조건, 비용, 쿨다운, 버프·디버프와 전투 상태를 구성했습니다. 어빌리티 공통 규칙은 베이스 클래스에서 관리하고 영웅별 효과는 파생 클래스로 확장했습니다.

기본 공격은 AnimNotify 시점에 서버에서 히트 판정과 데미지를 처리합니다. 입력 유지와 재입력을 이용한 콤보, 피격·사망·스턴 반응, 루트모션 도약과 착지 이벤트를 함께 연결했습니다.

### UI 구조

HUD와 월드스페이스 상태 위젯을 Widget - WidgetController - Model 구조로 분리했습니다. WidgetController가 PlayerState와 AbilitySystemComponent의 데이터를 받아 위젯 델리게이트로 전달하며, 위젯이 게임플레이 객체를 직접 조회하지 않도록 구성했습니다.

### AI 및 게임플레이

정글 몬스터는 캠프 앵커를 기준으로 순찰·전투·복귀 상태를 전환합니다. Behavior Tree와 Gameplay Event를 조합해 공격, 피격, 사망, 스턴과 보상 흐름을 구현했습니다.

## 멀티플레이어 구현

### 서버 권위 전투

능력 발동 검증, 히트 판정, 데미지 적용, 킬 보상과 매치 종료 조건은 서버에서 처리합니다. 클라이언트는 입력과 UI 요청을 보내고, 최종 게임 상태는 서버가 결정하도록 구성했습니다.

### Replication과 프로파일링

Unreal Insights Networking으로 복제 비용을 측정하고, 다음 항목을 조정했습니다.

- 변경된 값만 보내도록 Push Model 적용
- 아군은 원거리에서도 보이고 적은 거리 컬링하는 팀 기반 Net Relevancy
- 정글 몬스터의 Net Update Frequency와 Cull Distance 조정
- 클라이언트 이동 전송 주기와 서버 틱레이트 정렬
- GAS 몽타주 위치 복제 방식을 Section ID 기반으로 변경

9개 헤드리스 클라이언트로 로그인·매칭·게임플레이 흐름을 재현하는 부하 테스트 환경도 별도로 구성했습니다.

## 기술적으로 얻은 것

- Unreal Engine Gameplay Ability System의 기본 구조와 확장 방식
- UMG와 WidgetController를 이용한 데이터 기반 UI 갱신
- Dedicated Server 환경의 서버 권위 게임플레이
- Unreal Replication과 Actor relevancy의 기본 동작
- Unreal Insights Networking을 이용한 측정과 Before/After 비교
- 서버와 클라이언트 간 상태 흐름을 고려한 기능 설계

## 실행

프로젝트를 실행하려면 Unreal Engine 5.8 소스 빌드와 프로젝트 의존 플러그인이 필요합니다.  
패키징 및 헤드리스 봇 실행 스크립트는 Scripts/ 폴더에 정리되어 있습니다.

- 클라이언트 패키징: Scripts/Package_Client.bat
- 서버 패키징: Scripts/Package_Server.bat
- 봇 실행: Scripts/Run_Bots.bat
- 서버 실행: Scripts/Run_Server.bat
