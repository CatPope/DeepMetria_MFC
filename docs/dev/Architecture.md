# DeepMetria Architecture — LLM 기반 데이터 분석 SaaS 플랫폼 아키텍처 설계

## 목차
1. 시스템 개요 및 아키텍처 패턴 ........................... L17
2. 기술 스택 추천 및 확정 ................................. L70
3. 모듈 / 레이어 구조 ..................................... L127
4. 데이터 흐름 (핵심 플로우) .............................. L212
5. AI 오케스트레이션 아키텍처 ............................. L277
6. 인프라 토폴로지 (MVP 기준) ............................. L372
7. 확장 계획 (post-MVP) ................................... L429




---

## 1. 시스템 개요 및 아키텍처 패턴

### 1.1 시스템 개요

DeepMetria는 비개발자 비즈니스 사용자를 대상으로 하는 LLM 기반 데이터 분석 SaaS 플랫폼이다.
사용자가 데이터 파일을 업로드하면 AI가 자동으로 데이터를 요약하고, 자연어 질문에 따라
적절한 분석 도구와 차트를 선택하여 대시보드에 시각화를 생성한다.

DeepMetria에 접근하는 사용자 인터페이스는 세 가지다:
- **브라우저 (웹 대시보드)**: 일반 웹 사용자가 데이터 업로드·자연어 질문·시각화 확인
- **MCP 서버**: 외부 AI 클라이언트(Claude Desktop, Cursor 등)가 DeepMetria에 연결하여 분석 기능 사용
- **CLI + Skills**: 터미널에서 DeepMetria 기능을 명령줄로 사용

```
┌─────────────────────────────────────────────┐
│              사용자 인터페이스 레이어           │
│                                             │
│  [브라우저]   [MCP 클라이언트]   [CLI/Skills]  │
│  웹 대시보드  Claude Desktop 등  터미널 명령줄  │
└──────┬──────────────┬──────────────┬────────┘
       │ HTTPS        │ MCP 프로토콜  │ CLI
       ↓              ↓              ↓
┌─────────────────────────────────────────────┐
│              FastAPI 백엔드                  │
│  ┌─────────────────────────────────────┐   │
│  │         AI Agent (오케스트레이터)     │   │
│  │  자연어 질문 → Chain of Thinking     │   │
│  │  → 내부 분석 도구 선택 및 실행       │   │
│  └────────────────┬────────────────────┘   │
│                   ↓                         │
│  ┌─────────────────────────────────────┐   │
│  │       내부 분석 도구 (AnalysisTool)   │   │
│  │  pandas / 통계 라이브러리 기반 함수   │   │
│  └────────────────┬────────────────────┘   │
└───────────────────┼─────────────────────────┘
                    ↓
         [PostgreSQL] + [S3/R2 스토리지]
```

### 1.2 아키텍처 패턴

| 패턴 | 적용 근거 |
|------|----------|
| **레이어드 아키텍처** | 프론트엔드 / API / 도메인 / 인프라 계층 분리. MVP 단순성 확보 |
| **AI 오케스트레이터 패턴** | LLM이 직접 계산하지 않고 미리 정의된 AnalysisTool(내부 함수)을 선택·호출 |
| **추상화 레이어 (LLM Router)** | 멀티 LLM 지원 — Claude, GPT, Gemini를 단일 인터페이스로 라우팅 |
| **MCP 서버 패턴** | DeepMetria가 MCP 서버를 제공하여 외부 AI 클라이언트가 분석 기능에 연결 |
| **CLI + Skills 인터페이스** | 터미널 사용자가 DeepMetria 기능을 명령줄로 접근하는 인터페이스 제공 |
| **SSE 기반 실시간 스트리밍** | 분석 진행 상태·CoT 추론 단계·시각화 결과를 클라이언트에 실시간 전달 |
| **멀티테넌트 데이터 격리** | 사용자별 DataSource 격리, OWASP Top 10 준수 |

---

## 2. 기술 스택 추천 및 확정

### 2.1 프론트엔드

| 항목 | 선택 기술 | 선택 근거 |
|------|-----------|----------|
| 프레임워크 | **Next.js 14 (App Router)** | React 기반, SSR/SSG 지원, SaaS에 최적화된 파일 기반 라우팅 |
| UI 컴포넌트 | **shadcn/ui + Tailwind CSS** | 비개발자 대상 풍부한 UI, 라이선스 자유(MIT), 커스터마이즈 용이 |
| 차트 라이브러리 | **Recharts** | React 네이티브, SVG 기반, 커스터마이즈 용이, MIT 라이선스 |
| 대시보드 그리드 | **react-grid-layout** | 드래그/리사이즈 가능한 대시보드 캔버스, MIT 라이선스 |
| 상태 관리 | **Zustand** | 경량, 보일러플레이트 최소화, SSE 상태 관리에 적합 |
| 실시간 통신 | **EventSource (SSE)** | 단방향 서버→클라이언트 스트리밍, LLM 응답 스트리밍에 최적 |
| 파일 업로드 | **react-dropzone** | 드래그앤드롭 파일 업로드, MIT 라이선스 |
| 언어 | **TypeScript** | 타입 안전성, 팀 협업 품질 확보 |

### 2.2 백엔드

| 항목 | 선택 기술 | 선택 근거 |
|------|-----------|----------|
| 프레임워크 | **FastAPI (Python)** | async 지원, MCP SDK Python 버전 활용, 데이터 분석 생태계(pandas 등) 직접 연동 |
| LLM 추상화 | **LiteLLM** | Claude / GPT / Gemini를 단일 OpenAI 호환 인터페이스로 추상화, 활발한 유지보수 |
| MCP 서버 | **FastMCP (Python MCP SDK)** | Python 기반 MCP 서버 구현, AnalysisTool을 MCP 도구로 노출 |
| 데이터 처리 | **pandas + openpyxl** | CSV/Excel/JSON 파싱, 통계 계산 |
| 실시간 스트리밍 | **FastAPI SSE (StreamingResponse)** | CoT 추론 단계·분석 진행 상태 실시간 전달 |
| 인증/인가 | **JWT + FastAPI Security** | 사용자 인증, 멀티테넌트 데이터 격리 |
| 언어 | **Python 3.11+** | MCP SDK, 데이터 분석 라이브러리 생태계 최적 |

### 2.3 데이터베이스 및 스토리지

| 항목 | 선택 기술 | 선택 근거 |
|------|-----------|----------|
| 관계형 DB | **PostgreSQL** | 온톨로지 엔티티 관계 관리, ACID 보장, 클라우드 호환 |
| ORM | **SQLAlchemy 2.0 (async)** | 비동기 쿼리, Python 표준 ORM |
| 파일 스토리지 | **AWS S3 / Cloudflare R2** | 업로드 파일 저장, presigned URL, MVP 비용 최적화를 위해 R2 우선 검토 |
| 캐시 | **Redis** | 분석 결과 캐시, 세션 관리, UsageQuota 빠른 조회 |

### 2.4 인프라 및 배포

| 항목 | 선택 기술 | 선택 근거 |
|------|-----------|----------|
| 프론트엔드 배포 | **Vercel** | Next.js 최적화, 무중단 배포, MVP 비용 효율 |
| 백엔드 배포 | **Railway 또는 Render** | FastAPI 컨테이너 배포, 자동 스케일링, MVP 비용 효율 |
| DB 호스팅 | **Supabase PostgreSQL 또는 Railway PostgreSQL** | 관리형 PostgreSQL, 자동 백업 |
| 컨테이너 | **Docker** | 로컬 개발 환경 일관성, 배포 단순화 |
| CI/CD | **GitHub Actions** | 자동 테스트 및 배포 파이프라인 |

### 2.5 기술 스택 선택 요약

```
Frontend:  Next.js 14 + TypeScript + shadcn/ui + Tailwind + Recharts + react-grid-layout
Backend:   FastAPI (Python 3.11) + LiteLLM + FastMCP + pandas
Database:  PostgreSQL + Redis + S3/R2
Infra:     Vercel (FE) + Railway (BE) + GitHub Actions
```

---

## 3. 모듈 / 레이어 구조

### 3.1 프론트엔드 모듈 구조

```
src/
├── app/                          # Next.js App Router
│   ├── (auth)/                   # 인증 관련 페이지
│   │   ├── login/
│   │   └── register/
│   ├── dashboard/                # 대시보드 메인
│   │   ├── [dashboardId]/        # 개별 대시보드
│   │   └── page.tsx
│   └── upload/                   # 파일 업로드 페이지
├── components/
│   ├── ui/                       # shadcn/ui 기본 컴포넌트
│   ├── dashboard/
│   │   ├── DashboardCanvas.tsx   # react-grid-layout 캔버스
│   │   ├── VisualizationCard.tsx # 차트/테이블 래퍼
│   │   └── StyleEditor.tsx       # 서식 편집 패널
│   ├── analysis/
│   │   ├── QueryInput.tsx        # 자연어 질문 입력
│   │   ├── CotProgress.tsx       # CoT 추론 단계 표시
│   │   └── AnalysisStatus.tsx    # 분석 진행 상태
│   └── upload/
│       └── FileDropzone.tsx      # 파일 업로드 컴포넌트
├── lib/
│   ├── api/                      # API 클라이언트 (fetch 래퍼)
│   ├── sse/                      # SSE 연결 관리
│   └── chart/                    # 차트 타입 매핑 유틸리티
└── store/                        # Zustand 상태 스토어
    ├── dashboardStore.ts
    ├── analysisStore.ts
    └── userStore.ts
```

### 3.2 백엔드 레이어 구조

```
app/
├── api/                          # FastAPI 라우터 (API 레이어)
│   ├── auth.py                   # 인증/인가 엔드포인트
│   ├── datasource.py             # 파일 업로드, DataSource CRUD
│   ├── analysis.py               # 분석 요청, SSE 스트리밍
│   ├── dashboard.py              # Dashboard CRUD
│   └── visualization.py          # Visualization CRUD, 다운로드
├── domain/                       # 도메인 레이어 (비즈니스 로직)
│   ├── datasource/
│   │   ├── service.py            # DataSource 생성, 파일 파싱
│   │   └── summarizer.py        # AI 데이터 요약 (FR-02)
│   ├── analysis/
│   │   ├── orchestrator.py       # AI 오케스트레이터 (CoT, FR-04)
│   │   ├── flow.py               # AnalysisFlow 관리
│   │   └── chart_selector.py    # 차트 유형 자동 선택
│   ├── dashboard/
│   │   └── service.py
│   └── visualization/
│       ├── service.py
│       └── exporter.py           # 다운로드 (PNG 등)
├── infrastructure/               # 인프라 레이어
│   ├── llm/
│   │   ├── router.py             # LLM Router (LiteLLM 래퍼)
│   │   └── providers.py          # Claude / GPT / Gemini 설정
│   ├── mcp/
│   │   ├── server.py             # FastMCP 서버
│   │   └── tools/                # AnalysisTool 구현체
│   │       ├── statistics.py     # 기본 통계 도구
│   │       ├── aggregation.py    # 집계 도구
│   │       ├── timeseries.py     # 시계열 분석 도구
│   │       └── correlation.py    # 상관관계 도구
│   ├── storage/
│   │   ├── file_store.py         # S3/R2 파일 스토리지
│   │   └── db.py                 # PostgreSQL 연결 (SQLAlchemy)
│   └── cache/
│       └── redis.py              # Redis 캐시
└── models/                       # SQLAlchemy ORM 모델
    ├── user.py
    ├── dashboard.py
    ├── visualization.py
    ├── datasource.py
    └── analysis_flow.py
```

---

## 4. 데이터 흐름 (핵심 플로우)

### 4.1 파일 업로드 → 데이터 요약 플로우 (FR-01, FR-02)

```
[브라우저 FileDropzone]
    → POST /api/datasource/upload (multipart/form-data)
    → [BackEnd] 파일 형식 검증 (CSV/Excel/JSON)
    → S3/R2 스토리지에 원본 파일 저장
    → DataSource 엔티티 DB 생성
    → SSE 스트림 시작: "업로드 완료, 분석 시작"
    → [AI Summarizer] LLM Router → LLM (CoT)
        → 내부 분석 도구 호출: statistics.basic_stats(datasource_id)
        → 스키마 파악, 도메인 분류, 기본 통계 수집
    → DataSummary DB 저장
    → SSE 이벤트: "요약 완료" + DataSummary 페이로드
    → [프론트엔드] 기본 대시보드 구성 및 DataSummary 표시
```

### 4.2 자연어 질문 → 시각화 플로우 (FR-03, FR-04, FR-05)

```
[브라우저 QueryInput]
    → POST /api/analysis/query { question, datasource_id }
    → AnalysisFlow 엔티티 생성 (status: pending)
    → SSE 스트림 연결: GET /api/analysis/{flow_id}/stream

[AI 오케스트레이터 (orchestrator.py)]
    → SSE 이벤트: "CoT 추론 시작"
    → LLM Router → LLM (CoT 프롬프트)
        Step 1: 질문 분석 — 어떤 데이터 컬럼이 필요한가?
        Step 2: 도구 선택 — 어떤 AnalysisTool을 호출할 것인가?
        Step 3: 차트 선택 — 어떤 시각화 유형이 적합한가?
    → SSE 이벤트: CoT 단계별 추론 텍스트 스트리밍

    → 내부 분석 도구 호출 (선택된 AnalysisTool 함수 직접 실행)
        예: timeseries.monthly_trend(column="sales", datasource_id=...)
    → SSE 이벤트: "분석 도구 실행 중"
    → 분석 결과 수신

    → Visualization 엔티티 생성 (type, config, data)
    → Dashboard에 추가
    → SSE 이벤트: "시각화 완료" + Visualization 페이로드

[프론트엔드 DashboardCanvas]
    → 새 VisualizationCard 추가 (Recharts로 렌더링)
    → 실시간 대시보드 반영
```

### 4.3 서식 편집 → 다운로드 플로우 (FR-06, FR-07)

```
[StyleEditor / react-grid-layout 드래그/리사이즈]
    → PATCH /api/visualization/{id} { position, size, style }
    → Visualization 엔티티 업데이트
    → [프론트엔드] 즉시 반영 (optimistic update)

[다운로드 버튼]
    → GET /api/visualization/{id}/export?format=png
    → [BackEnd exporter.py] 차트 데이터를 이미지로 변환
    → 파일 다운로드 응답
```

---

## 5. AI 오케스트레이션 아키텍처

### 5.1 AI 오케스트레이터 구조

AI는 직접 계산하지 않는다. LLM은 오케스트레이터 역할만 수행하며,
내부 분석 도구(pandas 기반 Python 함수)를 선택하여 실행한다.

```
[LLM Router]
    ↓ LiteLLM 통합 인터페이스
[LLM Provider]  ←→  Claude API / OpenAI API / Gemini API
    ↓ Chain of Thinking 추론
[도구 선택 결정]
    ↓ Function Calling / Tool Use (내부 함수 스키마 전달)
[내부 AnalysisTool 함수 (pandas 기반)]
    ├── statistics.basic_stats      — 기본 통계 (평균, 중앙값, 표준편차 등)
    ├── statistics.distribution     — 분포 분석
    ├── aggregation.group_by        — 그룹 집계
    ├── aggregation.pivot           — 피벗 테이블
    ├── timeseries.monthly_trend    — 월별 추이
    ├── timeseries.moving_average   — 이동평균
    ├── correlation.pearson         — 피어슨 상관계수
    └── visualization.recommend     — 차트 유형 추천
```

### 5.2 LLM Router (멀티 LLM 추상화)

LiteLLM을 사용하여 Claude / GPT / Gemini를 단일 인터페이스로 추상화한다.

```python
# infrastructure/llm/router.py (개념 구조)
class LLMRouter:
    def __init__(self, provider: str, model: str):
        self.client = litellm  # Claude, GPT, Gemini 모두 동일 인터페이스

    async def chat(self, messages, tools=None) -> LLMResponse:
        return await litellm.acompletion(
            model=f"{self.provider}/{self.model}",
            messages=messages,
            tools=tools  # MCP AnalysisTool 스키마 전달
        )
```

지원 제공자:
- `anthropic/claude-3-5-sonnet` (기본값, CoT 성능 우수)
- `openai/gpt-4o`
- `google/gemini-1.5-pro`

### 5.3 Chain of Thinking (CoT) 프롬프트 전략

```
시스템 프롬프트:
  "당신은 데이터 분석 오케스트레이터입니다.
   사용자의 질문을 분석하여 다음 단계를 명시적으로 추론하세요:
   1. 필요한 데이터 컬럼 식별
   2. 적합한 분석 도구 선택 (제공된 내부 도구 목록 내에서만)
   3. 적합한 차트 유형 결정
   각 단계를 <thinking> 태그로 명시하세요."

도구 목록: 백엔드 내부 AnalysisTool 함수 스키마를 LLM에 전달 (Function Calling)
```

### 5.4 MCP 서버 (외부 사용자 인터페이스)

FastMCP를 사용하여 외부 AI 클라이언트가 DeepMetria에 연결할 수 있는 MCP 서버를 제공한다.
MCP 서버는 AI Agent의 내부 도구가 아니라, 외부 클라이언트(Claude Desktop, Cursor 등)가
DeepMetria 분석 기능에 접근하는 사용자 인터페이스 역할을 한다.

```
[외부 MCP 클라이언트]          ← 사용자 접점
  Claude Desktop, Cursor 등
    ↓ MCP 프로토콜
[DeepMetria MCP 서버 (FastMCP)]
    └── [FastAPI 백엔드 API 호출]
            → AI Agent → 내부 AnalysisTool 실행
            → 분석 결과 반환
```

MVP에서는 기본 MCP 엔드포인트 제공. post-MVP에서 독립 배포로 확장한다.

### 5.5 차트 자동 선택 로직

AI가 데이터 특성과 사용자 질문을 분석하여 차트 유형을 자동 결정한다.

| 데이터 특성 | 기본 차트 후보 |
|------------|--------------|
| 시간 + 수치 | 라인 차트, 영역 차트 |
| 카테고리 + 수치 | 바 차트, 수평 바 차트 |
| 비율/구성 | 파이 차트, 도넛 차트 |
| 분포 | 히스토그램, 박스플롯 |
| 두 수치 간 관계 | 산점도 |
| 테이블 형태 필요 | 데이터 테이블 |

---

## 6. 인프라 토폴로지 (MVP 기준)

### 6.1 MVP 인프라 구성

```
[사용자 브라우저]
    ↓ HTTPS
[Vercel Edge Network]
    └── [Next.js 앱 (Vercel)]
            ↓ HTTPS REST API / SSE
        [Railway / Render]
            └── [FastAPI 백엔드 (Docker 컨테이너)]
                    ├── [Supabase / Railway PostgreSQL]
                    ├── [Redis (Railway Redis 또는 Upstash)]
                    └── [Cloudflare R2 (파일 스토리지)]
                            ↓ HTTPS
                        [LLM APIs]
                            ├── Anthropic Claude API
                            ├── OpenAI API
                            └── Google Gemini API
```

### 6.2 환경별 구성

| 환경 | 프론트엔드 | 백엔드 | DB |
|------|-----------|--------|-----|
| 개발 (local) | `next dev` | `uvicorn` + Docker Compose | PostgreSQL + Redis (Docker) |
| 스테이징 | Vercel Preview | Railway (branch deploy) | 스테이징 DB |
| 프로덕션 | Vercel Production | Railway (main) | Supabase PostgreSQL |

### 6.3 보안 설계 (OWASP Top 10 준수)

| 위협 | 대응 방안 |
|------|----------|
| 인증/인가 취약점 | JWT 토큰 인증, 사용자별 DataSource 격리 (Row-level 필터링) |
| 파일 업로드 취약점 | 파일 형식 검증(magic bytes), 업로드 크기 제한(50MB), 파일명 sanitization |
| API 키 노출 | LLM API 키 서버 환경변수 관리, 클라이언트 미노출 |
| SQL Injection | SQLAlchemy ORM 파라미터 바인딩 사용 |
| XSS | Next.js 기본 이스케이프, CSP 헤더 설정 |
| 전송 암호화 | 모든 통신 TLS 1.2+ (Vercel/Railway 자동 적용) |
| 사용량 어뷰징 | Redis 기반 요청 속도 제한 (rate limiting) |

### 6.4 CI/CD 파이프라인

```
[GitHub 코드 푸시]
    → GitHub Actions
        ├── 린트 (ESLint + Ruff)
        ├── 타입 체크 (TypeScript + mypy)
        ├── 테스트 (pytest + Jest)
        └── 배포
            ├── main 브랜치 → Vercel Production + Railway Production
            └── PR 브랜치 → Vercel Preview + Railway Staging
```

---

## 7. 확장 계획 (post-MVP)

### 7.1 DataSource 확장 (DB / API 연결)

DataSource 추상화 레이어를 통해 최소한의 코드 변경으로 새로운 데이터 원천을 추가한다.

```
DataSource (abstract)
    ├── FileDataSource (MVP)       — CSV, Excel, JSON 파일 업로드
    ├── DBConnector (post-MVP)     — PostgreSQL, MySQL, BigQuery 연결
    └── APIConnector (post-MVP)    — Google Sheets, Salesforce, REST API 연결
```

추가 방법:
1. `infrastructure/connectors/` 에 새 커넥터 구현
2. MCP 서버에 커넥터 도구 등록
3. 프론트엔드 업로드 화면에 연결 옵션 추가

### 7.2 과금/결제 시스템 (post-MVP)

UsageQuota 엔티티와 PricingTier는 MVP에서 DB 구조만 설계해두고, 연동은 post-MVP에 구현한다.

| 단계 | 내용 |
|------|------|
| MVP | DB에 PricingTier, UsageQuota 테이블 생성, tier=free로 초기화 |
| post-MVP | Stripe 결제 연동, 웹훅으로 tier 업데이트, 분석 횟수 제한 미들웨어 |

### 7.3 MCP 서버 독립 배포 (post-MVP)

MVP에서는 FastAPI 백엔드와 통합 배포. post-MVP에서 MCP 서버를 독립 서비스로 분리한다.

```
MVP 구조:
[외부 MCP 클라이언트]
    ↓ MCP 프로토콜
[FastAPI 백엔드 내 MCP 엔드포인트]
    └── 백엔드 API를 통해 분석 기능 접근

post-MVP 구조:
[외부 MCP 클라이언트]
    ↓ MCP 프로토콜
[DeepMetria MCP Server] (독립 배포)
    └── 분석 기능 전체를 MCP 도구로 공개
```

### 7.4 확장성 고려 사항

| 항목 | MVP | post-MVP |
|------|-----|----------|
| 동시 사용자 | 수십~수백 명 | 수천 명 → 백엔드 수평 확장 (Railway 자동 스케일링) |
| 분석 작업 | 동기 SSE 스트리밍 | 비동기 작업 큐 (Celery + Redis) 도입 |
| 파일 크기 | 50MB 제한 | 청크 업로드 + 대용량 파일 처리 (Dask 등) |
| LLM 비용 | 단순 사용량 추적 | 과금 티어별 LLM 모델 차등 적용 |
| 모니터링 | 로그만 수집 | OpenTelemetry + Sentry + 대시보드 |
