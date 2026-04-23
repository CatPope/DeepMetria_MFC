# DeepMetria SRS — LLM 기반 데이터 분석 SaaS 플랫폼 소프트웨어 요구사항 명세

## 목차
1. 목적 (Purpose) ............................................. L16
2. 전체 설명 (Overall Description) ........................... L38
3. 구체적 요구사항 (Specific Requirements) ................... L100
4. 외부 인터페이스 요구사항 .................................. L268
5. 시스템 속성 ............................................... L248
6. 부록 ...................................................... L288





---

## 1. 목적 (Purpose)

### 1.1 문서 목적

본 문서는 DeepMetria 소프트웨어 시스템의 기능 요구사항 및 비기능 요구사항을 IEEE 830 표준에 따라 명세한다. 개발팀, 설계자, 이해관계자가 시스템 범위와 동작을 공통적으로 이해하기 위한 기준 문서로 사용된다.

### 1.2 제품 범위

DeepMetria는 비개발자(비즈니스 사용자)가 데이터 파일을 업로드하면, AI(LLM)가 자동으로 데이터를 요약하고 자연어 질문에 따라 적절한 분석 도구와 차트를 선택하여 대시보드에 시각화를 생성하는 웹 SaaS 플랫폼이다. MVP 범위는 파일 업로드, AI 데이터 요약, 자연어 기반 분석, 시각화, 서식 편집 및 다운로드로 한정한다.

### 1.3 정의 및 약어

| 약어/용어 | 설명 |
|-----------|------|
| LLM | Large Language Model (대형 언어 모델) |
| MCP | Model Context Protocol — AI가 외부 도구를 호출하는 프로토콜 |
| CoT | Chain of Thinking — 단계적 추론 방식 |
| MVP | Minimum Viable Product |
| SaaS | Software as a Service |
| DataSource | 사용자가 업로드하거나 연결한 데이터 원천 |

---

## 2. 전체 설명 (Overall Description)

### 2.1 제품 관점

DeepMetria는 독립형 웹 SaaS 플랫폼으로, 기존 BI 도구(Tableau, Power BI 등)와 달리 코드나 전문 지식 없이 자연어만으로 데이터 분석과 시각화를 수행할 수 있다. 핵심 아키텍처는 AI 오케스트레이터 모델을 따른다: LLM이 직접 계산하지 않고, 미리 정의된 분석 도구(AnalysisTool)를 MCP 서버/CLI/Skills 인터페이스를 통해 호출하여 분석을 수행한다.

```
사용자 (브라우저)
    ↓ 파일 업로드 / 자연어 질문
웹 프론트엔드
    ↓ API
백엔드 서버
    ↓ LLM Router (멀티 LLM 추상화)
LLM (Claude / GPT / Gemini)
    ↓ MCP / CLI / Skills
분석 도구 세트 (AnalysisTool)
    ↓ 분석 결과
시각화 렌더러 → 대시보드
```

### 2.2 제품 기능 (핵심 플로우)

1. 데이터 파일(CSV, Excel, JSON) 업로드
2. AI 자동 데이터 요약 생성 (스키마, 도메인, 기본 통계) → 기본 대시보드 구성
3. 사용자 자연어 질문 입력
4. AI(CoT)가 분석 계획 수립 및 분석 도구 선택/호출
5. 분석 결과를 적절한 차트 유형으로 자동 시각화
6. 대시보드 실시간 반영
7. 사용자가 시각화 요소의 서식 편집 및 다운로드

### 2.3 사용자 특성

| 특성 | 내용 |
|------|------|
| 주요 사용자 | 비개발자 비즈니스 담당자 (마케터, 기획자, 경영진 등) |
| 기술 수준 | 데이터 분석 도구 비숙련, 자연어 입력에 익숙 |
| 주요 목표 | 코드 작성 없이 데이터에서 인사이트 도출 및 보고서 작성 |
| 과금 티어 | Free / Pro / Max (MVP에서는 과금 연동 제외, 티어 구조만 설계) |

### 2.4 운영 환경

- **배포 환경**: 웹 SaaS, 클라우드 호스팅
- **접근 방식**: 웹 브라우저 (데스크톱 우선)
- **MVP 규모**: 소규모 사용자 (수십~수백 명 수준)

### 2.5 설계 및 구현 제약

- 기술 스택 미확정 — 아키텍처 설계 단계에서 결정
- AI 도구 인터페이스는 MCP 서버 + CLI + Skills 구조로 확정
- 멀티 LLM 지원을 위한 LLM Router 추상화 레이어 필수
- 거버넌스: 표준 (OWASP Top 10 준수, 라이선스 검토)
- MVP는 파일 업로드 방식만 지원 (DB/API 연결은 post-MVP)

### 2.6 가정 및 의존성

- 사용자는 항상 웹 브라우저를 통해 접근한다
- 외부 LLM API(Claude, GPT, Gemini 등) 서비스가 정상 운영 중이다
- MVP 기간 중 과금/결제 시스템 연동은 필요하지 않다
- 분석 도구(AnalysisTool)는 미리 정의된 세트로 제공되며, AI가 직접 계산 로직을 구현하지 않는다

---

## 3. 구체적 요구사항 (Specific Requirements)

### 3.1 기능 요구사항 (MVP)

#### FR-01 데이터 업로드

| 항목 | 내용 |
|------|------|
| ID | FR-01 |
| 명칭 | 데이터 파일 업로드 |
| 설명 | 사용자는 CSV, Excel(.xlsx/.xls), JSON 형식의 데이터 파일을 시스템에 업로드할 수 있다 |
| 입력 | 사용자가 선택한 파일 (CSV / Excel / JSON) |
| 처리 | 파일 형식 검증, 저장, DataSource 엔티티 생성 |
| 출력 | 업로드 완료 상태, DataSource 생성 확인 |
| 우선순위 | 필수 |

**인수 기준**
- CSV, Excel, JSON 파일 업로드 후 데이터 요약이 자동 생성된다
- 지원하지 않는 파일 형식은 업로드 시점에 오류 메시지가 표시된다

#### FR-02 AI 데이터 자동 요약

| 항목 | 내용 |
|------|------|
| ID | FR-02 |
| 명칭 | AI 데이터 자동 요약 생성 |
| 설명 | 파일 업로드 완료 시 AI가 자동으로 데이터의 스키마, 도메인, 기본 통계를 분석하여 DataSummary를 생성하고, 기본 대시보드에 표시한다 |
| 입력 | 업로드된 DataSource |
| 처리 | LLM이 CoT로 스키마 파악 → 도메인 분류 → 기본 통계 계산 도구 호출 |
| 출력 | DataSummary (스키마, 도메인, 기본 통계), 기본 대시보드 |
| 우선순위 | 필수 |

**인수 기준**
- 데이터 요약에는 컬럼 스키마(이름, 타입), 도메인 추정, 기본 통계(행 수, 결측치, 주요 수치 통계)가 포함된다
- 업로드 후 자동으로 기본 대시보드가 구성된다

#### FR-03 자연어 질문 입력

| 항목 | 내용 |
|------|------|
| ID | FR-03 |
| 명칭 | 자연어 분석 질문 입력 |
| 설명 | 사용자는 자연어로 분석 질문을 입력할 수 있다 (예: "월별 매출 추이 보여줘") |
| 입력 | 사용자 자연어 텍스트 |
| 처리 | 텍스트를 LLM Router로 전달, AnalysisFlow 생성 |
| 출력 | 분석 처리 시작 상태 |
| 우선순위 | 필수 |

**인수 기준**
- 자연어 질문 입력 UI가 제공된다
- 입력 후 AI 분석이 시작되는 피드백(로딩 상태 등)이 표시된다

#### FR-04 AI 분석 도구 선택 및 호출 (CoT)

| 항목 | 내용 |
|------|------|
| ID | FR-04 |
| 명칭 | AI 오케스트레이터 — 분석 도구 선택 및 호출 |
| 설명 | AI(LLM)가 Chain of Thinking으로 분석 계획을 수립하고, 미리 정의된 AnalysisTool 중 적합한 도구를 선택하여 MCP/CLI/Skills 인터페이스로 호출한다. AI는 어떤 데이터를, 어떤 도구로, 어떤 차트로 표현할지를 결정한다 |
| 입력 | 자연어 질문, DataSource |
| 처리 | CoT 추론 → 도구 선택 → MCP/CLI/Skills 호출 → 결과 수신 |
| 출력 | 분석 결과 데이터, 차트 유형 결정 |
| 우선순위 | 필수 |

**인수 기준**
- 자연어 질문을 입력하면 AI가 적절한 분석 도구와 차트 유형을 선택한다
- Chain of Thinking 추론 과정이 동작한다 (추론 단계 로그 또는 상태 노출)
- AI는 미리 정의된 AnalysisTool 세트 내에서만 도구를 선택한다

#### FR-05 차트 시각화 생성

| 항목 | 내용 |
|------|------|
| ID | FR-05 |
| 명칭 | 분석 결과 시각화 생성 |
| 설명 | AI가 선택한 차트 유형으로 분석 결과를 시각화하여 대시보드에 표시한다. 표(테이블) 형태의 시각화를 포함한다 |
| 입력 | 분석 결과 데이터, AI가 결정한 차트 유형 |
| 처리 | 차트 렌더링, Visualization 엔티티 생성, Dashboard에 추가 |
| 출력 | 대시보드에 시각화 요소 표시 |
| 우선순위 | 필수 |

**인수 기준**
- 시각화가 대시보드에 실시간으로 반영된다
- 표(테이블) 형태의 시각화를 지원한다
- AI가 데이터 특성과 사용자 요구에 맞는 차트 유형을 자동 선택한다

#### FR-06 시각화 서식 편집

| 항목 | 내용 |
|------|------|
| ID | FR-06 |
| 명칭 | 시각화 요소 서식 편집 |
| 설명 | 사용자는 대시보드의 시각화 요소 크기(높낮이, 너비), 위치, 스타일을 편집할 수 있다 |
| 입력 | 사용자의 드래그/리사이즈/스타일 변경 인터랙션 |
| 처리 | Visualization의 position, size, style 필드 업데이트 |
| 출력 | 변경된 서식이 즉시 반영된 대시보드 |
| 우선순위 | 필수 |

**인수 기준**
- 시각화 요소의 크기(높낮이, 너비)를 조절할 수 있다
- 시각화 요소의 위치를 변경할 수 있다
- 스타일(색상 등 기본 스타일)을 변경할 수 있다

#### FR-07 시각화 다운로드

| 항목 | 내용 |
|------|------|
| ID | FR-07 |
| 명칭 | 시각화 다운로드 |
| 설명 | 사용자는 대시보드의 시각화를 이미지 또는 파일 형태로 다운로드할 수 있다 |
| 입력 | 다운로드 요청 (시각화 선택) |
| 처리 | 시각화를 이미지/파일로 변환 후 다운로드 제공 |
| 출력 | 다운로드 가능한 이미지/파일 |
| 우선순위 | 필수 |

**인수 기준**
- 시각화를 이미지(PNG 등) 또는 파일로 다운로드할 수 있다

#### FR-08 멀티 LLM 지원

| 항목 | 내용 |
|------|------|
| ID | FR-08 |
| 명칭 | 멀티 LLM 라우터 |
| 설명 | 시스템은 LLM Router 추상화 레이어를 통해 복수의 LLM 제공자(Claude, GPT, Gemini 등)를 지원한다. MVP에서 최소 2개 이상의 LLM을 지원한다 |
| 입력 | LLM 설정, 분석 요청 |
| 처리 | LLM Router가 설정에 따라 적절한 LLM 제공자로 요청을 라우팅 |
| 출력 | LLM 응답 |
| 우선순위 | 필수 |

**인수 기준**
- 멀티 LLM 중 최소 2개 이상 지원한다
- LLM 전환 시 기능 동작에 영향을 주지 않는다 (추상화 레이어 보장)

### 3.2 기능 요구사항 (post-MVP, 범위 외)

| ID | 기능 | 비고 |
|----|------|------|
| FR-P01 | DB 직접 연결 (PostgreSQL, MySQL 등) | post-MVP |
| FR-P02 | 외부 API 연결 (Google Sheets, Salesforce 등) | post-MVP |
| FR-P03 | 과금/결제 시스템 연동 (Free/Pro/Max) | post-MVP |
| FR-P04 | MCP/CLI/Skills 외부 공개 | post-MVP |
| FR-P05 | 온프레미스 배포 | post-MVP |

### 3.3 비기능 요구사항

#### NFR-01 성능

| 항목 | 목표값 | 비고 |
|------|--------|------|
| 파일 업로드 후 데이터 요약 생성 | 30초 이내 | 일반적인 중소형 파일 기준 |
| 자연어 질문 → 시각화 생성 | 60초 이내 | LLM 응답 포함 |
| 대시보드 페이지 초기 로딩 | 3초 이내 | — |

#### NFR-02 사용성

- 비개발자가 별도 교육 없이 핵심 플로우(업로드 → 질문 → 시각화)를 완료할 수 있어야 한다
- 오류 발생 시 사용자가 이해할 수 있는 메시지를 표시한다
- 모든 주요 기능은 웹 브라우저(데스크톱)에서 접근 가능해야 한다

#### NFR-03 신뢰성

- 분석 요청 중 LLM API 오류 발생 시 사용자에게 오류 상태를 알리고, 재시도 방법을 안내한다
- 파일 업로드 실패 시 사용자 데이터가 손실되지 않는다

---

## 4. 외부 인터페이스 요구사항

### 4.1 사용자 인터페이스

| 화면/컴포넌트 | 설명 |
|---------------|------|
| 파일 업로드 화면 | 드래그앤드롭 또는 파일 선택으로 파일 업로드 |
| 데이터 요약 뷰 | 업로드된 데이터의 스키마, 도메인, 기본 통계 표시 |
| 대시보드 화면 | 시각화 요소들이 배치되는 캔버스, 서식 편집 인터페이스 |
| 자연어 질문 입력 | 채팅 또는 검색 바 형태의 자연어 입력 UI |
| 시각화 편집 패널 | 크기, 위치, 스타일 편집 컨트롤 |
| 다운로드 버튼 | 개별 시각화 또는 대시보드 전체 다운로드 |

- 언어: 한국어 우선 (국제화 구조는 설계 시 고려)
- 접근성: 기본 웹 접근성 준수 (alt 텍스트, 키보드 탐색)

### 4.2 소프트웨어 인터페이스

#### 4.2.1 LLM API 인터페이스

- 지원 제공자: Anthropic Claude, OpenAI GPT, Google Gemini (최소 2개)
- 인터페이스 방식: LLM Router 추상화 레이어를 통한 REST API 호출
- 인증: API 키 방식 (환경변수로 관리)

#### 4.2.2 분석 도구 인터페이스 (MCP/CLI/Skills)

- AI(LLM)는 MCP 서버 프로토콜 또는 CLI 명령, Skills 인터페이스를 통해 AnalysisTool을 호출한다
- AnalysisTool은 미리 정의된 세트로, AI는 도구를 선택/호출만 하며 계산 로직을 직접 구현하지 않는다
- MCP 서버는 내부 전용 (MVP 기간 외부 미공개)

#### 4.2.3 데이터 저장소 인터페이스

- 업로드 파일 저장: 클라우드 오브젝트 스토리지 (구체 기술은 설계 단계 확정)
- 메타데이터 저장: 관계형 DB 또는 문서형 DB (설계 단계 확정)

### 4.3 통신 인터페이스

- 프론트엔드 ↔ 백엔드: HTTPS REST API (또는 WebSocket, 실시간 업데이트 요건에 따라)
- 대시보드 실시간 반영: WebSocket 또는 Server-Sent Events 방식 검토
- 모든 외부 통신은 TLS 1.2 이상 사용

---

## 5. 시스템 속성

### 5.1 보안

| 요구사항 | 내용 |
|---------|------|
| 인증/인가 | 사용자 인증 필수, 타 사용자의 데이터에 접근 불가 |
| OWASP Top 10 | OWASP Top 10 위협에 대한 방어 구현 필수 |
| 파일 업로드 보안 | 파일 형식 검증, 악성 파일 차단, 업로드 크기 제한 |
| API 키 관리 | LLM API 키는 환경변수로 관리, 클라이언트에 노출 금지 |
| 전송 암호화 | 모든 데이터 전송은 HTTPS(TLS 1.2+) 적용 |
| 사용자 데이터 격리 | 사용자별 데이터 격리 (멀티테넌트 보안) |

### 5.2 가용성

- MVP 목표 가용성: 99% 이상 (월간 기준)
- 계획된 점검 시간은 사용자에게 사전 공지
- 단일 장애점(SPOF) 회피 설계 권장

### 5.3 확장성

- MVP 이후 DB 직접 연결, 외부 API 연결을 추가할 수 있는 DataSource 추상화 구조 적용
- 새로운 LLM 제공자 추가 시 LLM Router만 확장하면 되는 구조
- 새로운 AnalysisTool 추가 시 기존 시스템 변경 최소화

### 5.4 유지보수성

- 멀티 LLM 지원을 위한 추상화 레이어로 LLM 교체 용이
- MCP 서버와 분석 도구는 코어 시스템과 독립적으로 배포 가능한 구조 권장
- 라이선스: 사용된 모든 오픈소스 라이브러리의 라이선스 검토 필수 (표준 거버넌스)

---

## 6. 부록

### 6.1 온톨로지 (핵심 엔티티)

| Entity | 분류 | 주요 필드 | 관계 |
|--------|------|-----------|------|
| BusinessUser | core domain | id, email, tier, usageCount | owns Dashboards, has UsageQuota |
| Dashboard | core domain | id, name, layout, createdAt | has many Visualizations, owned by BusinessUser |
| Visualization | core domain | id, type, config, style, position, size | belongs to Dashboard, uses DataSource |
| DataSource | core domain | id, name, type(file/db/api), schema, metadata | has many AnalysisFlows |
| DataSummary | core domain | id, schema, domain, statistics | belongs to DataSource, displayed on Dashboard |
| AnalysisFlow | supporting | id, status, steps, result | belongs to DataSource, produces Visualizations |
| ChainOfThinking | core domain | id, steps, reasoning, result | part of AnalysisFlow |
| AnalysisTool | core domain | id, name, type, parameters | invoked by LLMRouter via MCP |
| LLMRouter | infrastructure | providers, activeModel, config | routes to multiple LLM providers |
| MCPServer | infrastructure | endpoints, tools, skills | exposes AnalysisTools to LLM |
| PricingTier | core domain | name(free/pro/max), limits | determines UsageQuota |
| UsageQuota | supporting | tier, used, remaining | belongs to BusinessUser |
| DBConnector | supporting (post-MVP) | type, connectionString | extends DataSource |
| APIConnector | supporting (post-MVP) | type, endpoint, auth | extends DataSource |

### 6.2 엔티티 관계 요약

```
BusinessUser
  ├── owns → Dashboard (1:N)
  └── has → UsageQuota (1:1, per PricingTier)

Dashboard
  └── has → Visualization (1:N)

DataSource
  ├── has → DataSummary (1:1)
  └── has → AnalysisFlow (1:N)

AnalysisFlow
  ├── contains → ChainOfThinking (1:1)
  └── produces → Visualization (1:N)

LLMRouter
  └── calls → AnalysisTool via MCPServer (N:M)
```

### 6.3 용어 정의

| 용어 | 정의 |
|------|------|
| AI 오케스트레이터 | LLM이 직접 계산하지 않고, 미리 정의된 분석 도구를 선택·호출하여 분석을 조율하는 역할 |
| Chain of Thinking (CoT) | LLM이 단계별로 추론 과정을 명시하며 분석 계획을 수립하는 방식 |
| AnalysisTool | 특정 분석 연산(통계 계산, 집계, 추세 분석 등)을 수행하는 미리 정의된 도구 단위 |
| LLM Router | 여러 LLM 제공자(Claude, GPT, Gemini 등)를 추상화하여 일관된 인터페이스로 요청을 라우팅하는 레이어 |
| DataSummary | 업로드된 데이터의 스키마, 도메인, 기본 통계를 AI가 자동 생성한 요약 정보 |
| Dashboard | 여러 Visualization 요소를 배치하고 편집할 수 있는 캔버스 화면 |
| Visualization | 차트, 표 등 데이터 분석 결과를 시각적으로 표현한 단위 요소 |
| MCP (Model Context Protocol) | AI가 외부 도구(AnalysisTool)를 호출하기 위한 표준 프로토콜 인터페이스 |
| 비즈니스 사용자 | 데이터 분석에 코드 작성이 필요 없는 비개발자 사용자 (DeepMetria의 주요 대상) |
| MVP (Minimum Viable Product) | 파일 업로드, AI 데이터 요약, 자연어 분석, 시각화, 서식 편집 및 다운로드로 한정된 최소 제품 범위 |

### 6.4 MVP 인수 기준 체크리스트

| # | 인수 기준 | 관련 FR |
|---|-----------|---------|
| AC-01 | 파일(CSV/Excel/JSON) 업로드 후 데이터 요약이 자동 생성된다 | FR-01, FR-02 |
| AC-02 | 데이터 요약에는 스키마, 도메인, 기본 통계가 포함된다 | FR-02 |
| AC-03 | 자연어 질문을 입력하면 AI가 적절한 분석 도구와 차트를 선택한다 | FR-03, FR-04 |
| AC-04 | 시각화가 대시보드에 실시간으로 반영된다 | FR-05 |
| AC-05 | 시각화 요소의 서식(크기, 높낮이, 스타일)을 편집할 수 있다 | FR-06 |
| AC-06 | 시각화를 이미지/파일로 다운로드할 수 있다 | FR-07 |
| AC-07 | 표 형태의 시각화를 지원한다 | FR-05 |
| AC-08 | 멀티 LLM 중 최소 2개 이상 지원한다 | FR-08 |
| AC-09 | Chain of Thinking 추론 과정이 동작한다 | FR-04 |
