# CLAUDE.md

이 파일은 **Backend**(P1 로그인/매치메이킹 서버) 프로젝트에서 작업할 때 Claude Code가 참고하는 가이드입니다. 언리얼 프로젝트(`P1/`)의 `P1/CLAUDE.md`와 별개로, 이 백엔드에는 이 파일이 적용됩니다.

## 프로젝트 정보

- **역할**: 언리얼 클라이언트(PreGame/로비 레벨)가 붙는 로그인 + 매치메이킹 API 서버. 실제 게임플레이(Arena)는 언리얼 데디케이티드 서버가 담당하고, 이 백엔드는 "회원가입/로그인 → 매칭 대기열 → 배정된 Arena 서버 주소 안내"까지만 책임진다.
- **스택**: Java 21, Spring Boot 4.1.0(Spring Framework 7.0.8), MySQL(Docker 컨테이너, `docker-compose.yml`), Maven(Wrapper 포함 — `./mvnw`, 로컬에 Maven 설치 불필요). 처음엔 3.3.4로 스캐폴딩했으나 이미 EOL(2025-11-18)된 버전이라 활성 지원 버전인 4.1.0으로 즉시 업그레이드함 — `com.mysql:mysql-connector-j`가 9.7.0으로, jjwt/jbcrypt는 Spring Boot BOM과 무관해 그대로 유지됨.
- **인증**: Spring Security를 쓰지 않고 JWT를 직접 구현한다(`JwtService`+`JwtAuthInterceptor`). 비밀번호 해싱은 jBCrypt(`org.mindrot:jbcrypt`) 직접 사용.
- **매치메이킹**: 지금은 인메모리 큐(단일 프로세스 전제) — Redis는 나중에(여러 백엔드 인스턴스로 확장할 때) 붙일 예정. 실제 데디케이티드 서버를 동적으로 띄우지 않고, `application.yml`의 `match.server-address`(고정값)를 대기열 인원이 다 찼을 때 배정한다.
- **DB**: MySQL, `spring.jpa.hibernate.ddl-auto=update`로 스키마 자동 생성(마이그레이션 툴 없음 — 스키마가 안정화되면 Flyway/Liquibase 도입 검토).

## 빌드 / 실행

- `docker compose up -d` — MySQL 컨테이너만 띄움(백엔드 자체는 컨테이너화 안 함, 로컬 실행). **Docker는 사용자가 직접 관리한다 — Claude는 Docker 명령을 실행하지 않는다.**
- `./mvnw spring-boot:run` (또는 IntelliJ에서 `pom.xml` 열어서 `P1BackendApplication` 실행) — 로컬 개발/디버깅은 IntelliJ 쪽이 더 빠름.
- `./mvnw compile` — 컴파일만 확인할 때(MySQL 없이도 가능, 런타임 연결은 앱 기동 시점에만 필요).
- 기본 포트: `8080` (`server.port`, `application.yml`).

## 패키지 구조 컨벤션 (레이어드 아키텍처 — 반드시 지킬 것)

**기능(도메인) 단위가 아니라 기술적 계층(레이어) 단위로 패키지를 나눈다.** 예를 들어 "매치메이킹" 기능이라고 해서 `match/` 패키지 하나에 컨트롤러·서비스·DTO를 전부 몰아넣지 않는다 — 대신:

```
com.p1.backend
├── controller/   HTTP 요청/응답 변환만 담당. 비즈니스 로직 없음 — Service 호출 + 상태코드/응답 바디 매핑만.
├── service/      실제 비즈니스 로직(인증, 매치메이킹 등). Repository/다른 Service를 조합.
├── dto/          API 요청/응답 바디(record). Entity를 직접 노출하지 않기 위한 경계.
├── entity/       JPA @Entity. DB 테이블과 1:1 매핑, 비즈니스 로직을 담지 않는다(빈약한 도메인 모델이어도 지금 규모에선 무방 — 규칙이 복잡해지면 그때 도메인 메서드 추가 검토).
├── repository/   Spring Data JPA 인터페이스.
├── config/       Spring `@Configuration` 클래스(예: 인터셉터 등록).
└── interceptor/  `HandlerInterceptor` 등 요청 파이프라인에 끼어드는 컴포넌트.
```

새 기능을 추가할 때도 이 규칙을 유지한다 — 예를 들어 나중에 "인벤토리" 기능이 생기면 `InventoryController`는 `controller/`에, `InventoryService`는 `service/`에, 관련 DTO는 `dto/`에 추가한다(`inventory/` 패키지를 새로 만들지 않는다).

## 객체지향 / SOLID 원칙

- **단일 책임 원칙(SRP)**: Controller는 HTTP ↔ DTO 변환만, Service는 비즈니스 로직만, Repository는 데이터 접근만. 한 클래스가 여러 계층의 일을 겸하지 않는다(`AuthController`가 비밀번호 해싱을 직접 하지 않고 `AuthService`에 위임하는 게 예시).
- **생성자 주입(Constructor Injection)만 사용** — 필드에 `@Autowired`를 붙이는 방식은 쓰지 않는다. 모든 의존성은 생성자 파라미터로 받고 `final` 필드에 저장한다(불변성 확보, 테스트 시 목 주입이 쉬움). Spring Boot 4.3+에서는 생성자가 하나뿐이면 `@Autowired` 자체가 필요 없다 — 이 프로젝트의 모든 서비스/컨트롤러/컴포넌트가 이 패턴을 따른다.
- **DTO와 Entity의 명확한 분리**: `User`(entity)를 API 응답에 그대로 반환하지 않는다 — 항상 `dto/`의 record로 변환해서 내보낸다(비밀번호 해시 같은 민감 필드가 실수로 노출되는 걸 구조적으로 방지).
- **개방-폐쇄 원칙(OCP)은 실제로 확장 지점이 생겼을 때만 적용** — 지금은 인증 방식이 JWT 하나뿐이라 `AuthService`가 구체 클래스에 직접 의존해도 무방하다. 두 번째 인증 방식(OAuth 등)이 실제로 필요해지기 전까지 인터페이스를 미리 뽑아두지 않는다(추측성 추상화 금지 — 언리얼 프로젝트 `P1/CLAUDE.md`에서도 동일한 원칙을 쓰고 있음).
- **예외를 통한 실패 처리**: 비즈니스 규칙 위반(중복 아이디, 잘못된 비밀번호 등)은 커스텀 `RuntimeException` 서브클래스로 표현하고, Controller가 이를 잡아 적절한 HTTP 상태코드로 변환한다(`AuthService.UsernameTakenException` → 409 등). 여러 컨트롤러에서 반복되면 `@RestControllerAdvice` 전역 핸들러 도입을 검토할 것(아직은 컨트롤러 2개뿐이라 과함).

## 구현 현황

| 영역 | 상태 | 비고 |
|---|---|---|
| 프로젝트 스캐폴드 (`pom.xml`, Maven Wrapper) | 완료 | Spring Boot 3.3.4, Java 21 |
| `docker-compose.yml` (MySQL) | 완료 | `mysql:8.4`, DB=`p1`, 계정 `p1`/`p1password` |
| `entity/User` + `repository/UserRepository` | 완료 | username unique, passwordHash(BCrypt) |
| `service/JwtService` | 완료 | jjwt 0.12.x, HS256, `jwt.secret`/`jwt.expiration-ms`(application.yml) |
| `service/AuthService` (signup/login) | 완료 | jBCrypt 해싱/검증 |
| `controller/AuthController` | 완료 | `POST /api/auth/signup`(201/409), `POST /api/auth/login`(200/401) |
| `interceptor/JwtAuthInterceptor` + `config/WebConfig` | 완료 | `/api/match/**`만 보호, `Authorization: Bearer <token>` 검증 |
| `service/MatchmakingService` | 완료 | 인메모리 큐, `match.required-players`(현재 2) 도달 시 즉시 페어링, `match.server-address`(고정값) 배정 |
| `controller/MatchController` | 완료 | `POST /api/match/queue`, `GET /api/match/status` |
| 컴파일 검증 | 완료 | `./mvnw compile` 성공 |
| **MySQL 기동 후 curl 엔드투엔드 검증** | **미완료** | Docker는 사용자가 직접 관리 — MySQL 띄운 뒤 signup×2 → login×2 → queue×2로 `MATCHED` 확인 필요 |
| 언리얼 PreGame 연동 (HTTP 클라이언트, 로그인/큐 UI) | 미완료 | `P1/` 저장소 쪽 작업, 별도 진행 중 |

## 다음 작업 예정

- 사용자가 MySQL(Docker) 기동 후 curl/Postman으로 회원가입→로그인→매칭 흐름 직접 검증
- 언리얼 클라이언트(`UP1BackendSubsystem`)와 실제 연동 테스트
- (이후) Redis 기반 분산 매치메이킹, 실제 데디케이티드 서버 동적 프로비저닝, 회원 정보 확장(레벨/전적 연동 등)은 MVP 이후 범위
