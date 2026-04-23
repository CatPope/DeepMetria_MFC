# DeepMetria DetailedSpec — LLM 기반 데이터 분석 SaaS 플랫폼 상세 설계

## 목차
1. API 엔드포인트 설계 .......................................... L17
2. DB 스키마 설계 ................................................ L292
3. 컴포넌트 설계 (프론트엔드) ................................... L470
4. AI 오케스트레이션 상세 ....................................... L627
5. 파일 업로드/처리 파이프라인 .................................. L781
6. 대시보드 레이아웃 시스템 ..................................... L851
7. 시각화 서식 편집 모델 ........................................ L916
8. 에러 핸들링 전략 ............................................. L965
9. 상태 관리 설계 ............................................... L1036


---

## 1. API 엔드포인트 설계

### 1.1 기본 규칙

- Base URL: `https://api.deepmetria.com/api/v1`
- 인증: `Authorization: Bearer <JWT>` 헤더 필수 (공개 엔드포인트 제외)
- 응답 형식: JSON (SSE 스트리밍 엔드포인트 제외)
- 오류 응답 공통 형식:
  ```json
  { "error": { "code": "ERROR_CODE", "message": "사용자 친화적 메시지", "detail": "..." } }
  ```

### 1.2 인증 (Auth)

#### POST /auth/register
- 설명: 신규 사용자 등록
- 요청 본문:
  ```json
  { "email": "string", "password": "string", "name": "string" }
  ```
- 응답 `201`:
  ```json
  { "user": { "id": "uuid", "email": "string", "name": "string", "tier": "free" }, "access_token": "string", "refresh_token": "string" }
  ```
- 오류: `400 VALIDATION_ERROR`, `409 EMAIL_ALREADY_EXISTS`

#### POST /auth/login
- 설명: 로그인, JWT 발급
- 요청 본문:
  ```json
  { "email": "string", "password": "string" }
  ```
- 응답 `200`:
  ```json
  { "access_token": "string", "refresh_token": "string", "expires_in": 3600 }
  ```
- 오류: `401 INVALID_CREDENTIALS`

#### POST /auth/refresh
- 설명: Access Token 갱신
- 요청 본문: `{ "refresh_token": "string" }`
- 응답 `200`: `{ "access_token": "string", "expires_in": 3600 }`

#### GET /auth/me
- 설명: 현재 사용자 정보 조회
- 응답 `200`:
  ```json
  { "id": "uuid", "email": "string", "name": "string", "tier": "free|pro|max", "usage": { "used": 0, "limit": 10 } }
  ```

### 1.3 데이터소스 (DataSource)

#### POST /datasources/upload
- 설명: 파일 업로드 및 DataSource 생성
- 요청: `multipart/form-data`
  - `file`: 파일 바이너리 (CSV / .xlsx / .xls / JSON)
  - `name`: 데이터소스 이름 (optional, 기본값: 파일명)
- 응답 `202`:
  ```json
  { "datasource_id": "uuid", "status": "processing", "stream_url": "/api/v1/datasources/{id}/stream" }
  ```
- 오류: `400 UNSUPPORTED_FILE_TYPE`, `413 FILE_TOO_LARGE`, `422 PARSE_ERROR`
- 제한: 최대 파일 크기 50MB

#### GET /datasources
- 설명: 사용자 DataSource 목록 조회
- 쿼리 파라미터: `page=1&size=20`
- 응답 `200`:
  ```json
  { "items": [{ "id": "uuid", "name": "string", "type": "csv|excel|json", "row_count": 0, "created_at": "ISO8601", "status": "ready|processing|error" }], "total": 0, "page": 1 }
  ```

#### GET /datasources/{id}
- 설명: DataSource 상세 조회 (스키마, 요약 포함)
- 응답 `200`:
  ```json
  {
    "id": "uuid", "name": "string", "status": "ready",
    "schema": [{ "column": "string", "dtype": "string|int|float|datetime", "nullable": true }],
    "summary": { "domain": "string", "row_count": 0, "column_count": 0, "statistics": {} }
  }
  ```

#### GET /datasources/{id}/stream
- 설명: 업로드 후 요약 생성 진행 상태를 SSE로 수신
- 응답: `text/event-stream`
  ```
  event: status
  data: {"stage": "uploading|parsing|summarizing|done", "message": "string", "progress": 0.0}

  event: summary_complete
  data: {"summary": {...}, "dashboard_id": "uuid"}

  event: error
  data: {"code": "string", "message": "string"}
  ```

#### DELETE /datasources/{id}
- 설명: DataSource 삭제 (연관 분석 결과, 시각화 포함)
- 응답 `204`

### 1.4 분석 (Analysis)

#### POST /analysis/query
- 설명: 자연어 질문으로 분석 요청
- 요청 본문:
  ```json
  { "question": "월별 매출 추이 보여줘", "datasource_id": "uuid", "dashboard_id": "uuid" }
  ```
- 응답 `202`:
  ```json
  { "flow_id": "uuid", "status": "pending", "stream_url": "/api/v1/analysis/{flow_id}/stream" }
  ```
- 오류: `402 QUOTA_EXCEEDED`, `404 DATASOURCE_NOT_FOUND`, `422 EMPTY_QUESTION`

#### GET /analysis/{flow_id}/stream
- 설명: CoT 추론 단계 및 분석 결과를 SSE로 수신
- 응답: `text/event-stream`
  ```
  event: cot_step
  data: {"step": 1, "type": "column_analysis", "content": "sales, date 컬럼 필요"}

  event: tool_call
  data: {"tool": "timeseries.monthly_trend", "params": {"column": "sales", "date_column": "date"}}

  event: tool_result
  data: {"tool": "timeseries.monthly_trend", "result": {...}}

  event: visualization_ready
  data: {"visualization_id": "uuid", "type": "line", "config": {...}}

  event: done
  data: {"flow_id": "uuid", "status": "completed"}

  event: error
  data: {"code": "string", "message": "string"}
  ```

#### GET /analysis/{flow_id}
- 설명: 분석 플로우 상태 및 결과 조회
- 응답 `200`:
  ```json
  { "id": "uuid", "status": "pending|running|completed|failed", "question": "string", "cot_steps": [], "visualization_ids": [] }
  ```

### 1.5 대시보드 (Dashboard)

#### GET /dashboards
- 설명: 사용자 대시보드 목록
- 응답 `200`: `{ "items": [{ "id": "uuid", "name": "string", "datasource_id": "uuid", "created_at": "ISO8601" }], "total": 0 }`

#### POST /dashboards
- 설명: 대시보드 생성
- 요청 본문: `{ "name": "string", "datasource_id": "uuid" }`
- 응답 `201`: `{ "id": "uuid", "name": "string", "layout": [] }`

#### GET /dashboards/{id}
- 설명: 대시보드 상세 (레이아웃 + 시각화 목록 포함)
- 응답 `200`:
  ```json
  {
    "id": "uuid", "name": "string",
    "layout": [{ "i": "viz-uuid", "x": 0, "y": 0, "w": 6, "h": 4 }],
    "visualizations": [{ "id": "uuid", "type": "line", "config": {}, "style": {} }]
  }
  ```

#### PATCH /dashboards/{id}
- 설명: 대시보드 이름 또는 레이아웃 업데이트
- 요청 본문: `{ "name": "string", "layout": [...] }`
- 응답 `200`: 업데이트된 대시보드

#### DELETE /dashboards/{id}
- 응답 `204`

### 1.6 MCP 서버 (DeepMetria MCP Server)

DeepMetria는 MCP 서버를 제공하여 Claude Desktop, Cursor 등 외부 AI 클라이언트가 연결할 수 있습니다.
외부 AI 클라이언트는 아래 MCP 도구를 호출하여 DeepMetria의 기능을 사용합니다.

| MCP 도구 이름 | 설명 | 입력 파라미터 | 반환 형식 |
|--------------|------|-------------|----------|
| `upload_datasource` | 파일 업로드 및 DataSource 생성 | `file_path`, `name?` | `{ datasource_id, status }` |
| `list_datasources` | DataSource 목록 조회 | `page?`, `size?` | `{ items[], total }` |
| `get_datasource` | DataSource 상세 조회 (스키마, 요약) | `datasource_id` | `{ id, schema[], summary }` |
| `request_analysis` | 자연어 질문으로 분석 요청 | `question`, `datasource_id`, `dashboard_id?` | `{ flow_id, status }` |
| `get_analysis_result` | 분석 결과 조회 | `flow_id` | `{ status, cot_steps[], visualization_ids[] }` |
| `list_dashboards` | 대시보드 목록 조회 | — | `{ items[], total }` |
| `get_dashboard` | 대시보드 상세 조회 (레이아웃 + 시각화) | `dashboard_id` | `{ id, name, layout[], visualizations[] }` |
| `get_visualization` | 개별 시각화 조회 | `visualization_id` | `{ id, type, config, style }` |
| `export_visualization` | 시각화 이미지 다운로드 URL 획득 | `visualization_id`, `format?` | `{ download_url }` |

- MCP 서버는 내부적으로 동일한 REST API (`/api/v1`)를 호출합니다.
- 인증: MCP 클라이언트 연결 시 API Key 또는 JWT를 설정 파일에 지정합니다.
- MCP 서버 설정 예시 (`claude_desktop_config.json`):
  ```json
  {
    "mcpServers": {
      "deepmetria": {
        "command": "uvx",
        "args": ["deepmetria-mcp"],
        "env": { "DEEPMETRIA_API_KEY": "your_api_key" }
      }
    }
  }
  ```

### 1.7 CLI 인터페이스

`deepmetria` CLI는 터미널에서 DeepMetria 기능을 명령줄로 사용하는 인터페이스입니다.
내부적으로 동일한 REST API (`/api/v1`)를 호출합니다.

#### 인증 설정
```bash
deepmetria login                          # API Key 또는 이메일/비밀번호 입력, ~/.deepmetria/config에 저장
deepmetria logout                         # 인증 정보 삭제
deepmetria whoami                         # 현재 사용자 정보 출력
```

#### 데이터소스 관리
```bash
deepmetria upload <file>                  # 파일 업로드 (진행 상태 출력)
deepmetria upload <file> --name "이름"    # 이름 지정 업로드
deepmetria datasource list                # DataSource 목록 출력
deepmetria datasource get <id>            # DataSource 상세 출력 (스키마 포함)
deepmetria datasource delete <id>         # DataSource 삭제
```

#### 분석 요청
```bash
deepmetria analyze "<질문>" --datasource <id>             # 분석 요청 (결과 대기 후 출력)
deepmetria analyze "<질문>" --datasource <id> --stream    # 분석 CoT 단계 실시간 출력
deepmetria analyze "<질문>" --datasource <id> --dashboard <id>  # 특정 대시보드에 추가
```

#### 대시보드 관리
```bash
deepmetria dashboard list                 # 대시보드 목록 출력
deepmetria dashboard get <id>             # 대시보드 상세 출력
deepmetria dashboard create --name "이름" --datasource <id>  # 대시보드 생성
deepmetria dashboard delete <id>          # 대시보드 삭제
```

#### 시각화 관리
```bash
deepmetria viz get <id>                   # 시각화 상세 출력
deepmetria viz export <id> --output out.png  # 시각화 이미지 다운로드
```

### 1.8 시각화 (Visualization)

#### GET /visualizations/{id}
- 응답 `200`:
  ```json
  { "id": "uuid", "dashboard_id": "uuid", "type": "line|bar|pie|scatter|table|area|histogram", "config": { "title": "string", "data": [], "axes": {} }, "style": { "color_scheme": "default", "font_size": 14 }, "position": { "x": 0, "y": 0, "w": 6, "h": 4 } }
  ```

#### PATCH /visualizations/{id}
- 설명: 서식(위치, 크기, 스타일) 업데이트
- 요청 본문:
  ```json
  { "position": { "x": 0, "y": 0, "w": 6, "h": 4 }, "style": { "color_scheme": "blue", "font_size": 14, "title": "string" } }
  ```
- 응답 `200`: 업데이트된 Visualization

#### DELETE /visualizations/{id}
- 응답 `204`

#### GET /visualizations/{id}/export
- 설명: 시각화 이미지 다운로드
- 쿼리 파라미터: `format=png` (기본값)
- 응답: `image/png` 바이너리 또는 `{ "download_url": "presigned_url" }`

---

## 2. DB 스키마 설계

### 2.1 설계 원칙

- PostgreSQL 사용, SQLAlchemy 2.0 (async) ORM
- UUID v4를 PK로 사용 (예측 불가능성, 분산 환경 대응)
- `created_at`, `updated_at` 모든 테이블에 공통 포함
- 소프트 삭제(`deleted_at`)는 user, dashboard 테이블에만 적용
- 멀티테넌트 격리: 모든 데이터 테이블에 `user_id` 포함 + RLS 정책 검토

### 2.2 테이블 정의

#### users
```sql
CREATE TABLE users (
    id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    email       VARCHAR(255) UNIQUE NOT NULL,
    name        VARCHAR(100) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    tier        VARCHAR(20) NOT NULL DEFAULT 'free'   -- free | pro | max
                CHECK (tier IN ('free', 'pro', 'max')),
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    deleted_at  TIMESTAMPTZ
);
CREATE INDEX idx_users_email ON users(email) WHERE deleted_at IS NULL;
```

#### usage_quotas
```sql
CREATE TABLE usage_quotas (
    id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id     UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    period      DATE NOT NULL,                         -- YYYY-MM-01 (월별 집계)
    used        INTEGER NOT NULL DEFAULT 0,
    limit_count INTEGER NOT NULL DEFAULT 10,           -- 티어별 한도
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE(user_id, period)
);
CREATE INDEX idx_usage_quotas_user_period ON usage_quotas(user_id, period);
```

#### datasources
```sql
CREATE TABLE datasources (
    id           UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id      UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    name         VARCHAR(255) NOT NULL,
    file_type    VARCHAR(20) NOT NULL CHECK (file_type IN ('csv', 'excel', 'json')),
    storage_key  VARCHAR(512) NOT NULL,                -- R2/S3 오브젝트 키
    file_size    BIGINT NOT NULL,
    row_count    INTEGER,
    column_count INTEGER,
    status       VARCHAR(20) NOT NULL DEFAULT 'processing'
                 CHECK (status IN ('processing', 'ready', 'error')),
    error_message TEXT,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at   TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_datasources_user_id ON datasources(user_id);
CREATE INDEX idx_datasources_status ON datasources(status);
```

#### data_schemas
```sql
CREATE TABLE data_schemas (
    id             UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    datasource_id  UUID NOT NULL REFERENCES datasources(id) ON DELETE CASCADE,
    column_name    VARCHAR(255) NOT NULL,
    column_index   INTEGER NOT NULL,
    dtype          VARCHAR(50) NOT NULL,               -- string | int | float | datetime | boolean
    nullable       BOOLEAN NOT NULL DEFAULT TRUE,
    sample_values  JSONB,                              -- 대표 샘플 값 최대 5개
    created_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_data_schemas_datasource ON data_schemas(datasource_id);
```

#### data_summaries
```sql
CREATE TABLE data_summaries (
    id             UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    datasource_id  UUID NOT NULL UNIQUE REFERENCES datasources(id) ON DELETE CASCADE,
    domain         VARCHAR(255),                       -- AI 추정 도메인 (예: "영업 데이터")
    description    TEXT,                               -- AI 생성 요약 텍스트
    statistics     JSONB NOT NULL DEFAULT '{}',        -- 컬럼별 통계 (min, max, mean, nulls 등)
    row_count      INTEGER NOT NULL DEFAULT 0,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
```

#### dashboards
```sql
CREATE TABLE dashboards (
    id            UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id       UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    datasource_id UUID NOT NULL REFERENCES datasources(id),
    name          VARCHAR(255) NOT NULL,
    layout        JSONB NOT NULL DEFAULT '[]',          -- react-grid-layout GridItem[]
    created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    deleted_at    TIMESTAMPTZ
);
CREATE INDEX idx_dashboards_user_id ON dashboards(user_id) WHERE deleted_at IS NULL;
CREATE INDEX idx_dashboards_datasource ON dashboards(datasource_id);
```

#### visualizations
```sql
CREATE TABLE visualizations (
    id           UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    dashboard_id UUID NOT NULL REFERENCES dashboards(id) ON DELETE CASCADE,
    user_id      UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    flow_id      UUID,                                 -- 생성한 AnalysisFlow (nullable: 자동 요약 생성분)
    viz_type     VARCHAR(50) NOT NULL                  -- line | bar | pie | scatter | table | area | histogram | donut
                 CHECK (viz_type IN ('line','bar','pie','scatter','table','area','histogram','donut','hbar')),
    title        VARCHAR(255),
    chart_config JSONB NOT NULL DEFAULT '{}',          -- 차트 데이터 및 축 설정
    style        JSONB NOT NULL DEFAULT '{}',          -- 색상, 폰트 등 스타일
    position     JSONB NOT NULL DEFAULT '{}',          -- {x, y, w, h} grid 위치
    created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at   TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_visualizations_dashboard ON visualizations(dashboard_id);
CREATE INDEX idx_visualizations_user ON visualizations(user_id);
```

#### analysis_flows
```sql
CREATE TABLE analysis_flows (
    id             UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id        UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    datasource_id  UUID NOT NULL REFERENCES datasources(id),
    dashboard_id   UUID REFERENCES dashboards(id),
    question       TEXT NOT NULL,
    status         VARCHAR(20) NOT NULL DEFAULT 'pending'
                   CHECK (status IN ('pending', 'running', 'completed', 'failed')),
    llm_provider   VARCHAR(50),                        -- 사용된 LLM 제공자
    llm_model      VARCHAR(100),
    cot_steps      JSONB NOT NULL DEFAULT '[]',        -- CoT 추론 단계 로그
    tool_calls     JSONB NOT NULL DEFAULT '[]',        -- 내부 분석 도구 호출 기록
    error_message  TEXT,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_analysis_flows_user ON analysis_flows(user_id);
CREATE INDEX idx_analysis_flows_datasource ON analysis_flows(datasource_id);
CREATE INDEX idx_analysis_flows_status ON analysis_flows(status);
```

### 2.3 관계 다이어그램 (요약)

```
users (1) ──< usage_quotas (N)
users (1) ──< datasources (N)
datasources (1) ──< data_schemas (N)
datasources (1) ── data_summaries (1)
users (1) ──< dashboards (N)
datasources (1) ──< dashboards (N)
dashboards (1) ──< visualizations (N)
users (1) ──< analysis_flows (N)
datasources (1) ──< analysis_flows (N)
analysis_flows (1) ──< visualizations (N)
```

### 2.4 Redis 캐시 키 규칙

| 키 패턴 | 값 | TTL | 용도 |
|---------|---|-----|------|
| `quota:{user_id}:{YYYY-MM}` | `{"used": N, "limit": M}` | 1시간 | 사용량 빠른 조회 |
| `datasource:schema:{id}` | JSON 스키마 배열 | 1일 | 스키마 반복 조회 캐시 |
| `session:{token_hash}` | `{"user_id": "uuid"}` | 1시간 | JWT 블랙리스트/세션 관리 |
| `analysis:lock:{datasource_id}:{user_id}` | `1` | 5분 | 동시 분석 요청 제한 |

---

## 3. 컴포넌트 설계 (프론트엔드)

### 3.1 페이지 구조 (Next.js App Router)

```
src/app/
├── (auth)/
│   ├── login/page.tsx           — 로그인 화면
│   └── register/page.tsx        — 회원가입 화면
├── (main)/
│   ├── layout.tsx               — 인증 가드 + 사이드바 레이아웃
│   ├── page.tsx                 — 홈 (데이터소스 목록)
│   ├── upload/page.tsx          — 파일 업로드 화면
│   └── dashboard/
│       ├── page.tsx             — 대시보드 목록
│       └── [dashboardId]/
│           └── page.tsx         — 개별 대시보드 화면
└── api/                         — Next.js Route Handlers (프록시 최소화)
```

### 3.2 컴포넌트 트리

#### 대시보드 화면 (`/dashboard/[dashboardId]`)

```
DashboardPage
├── DashboardHeader
│   ├── DashboardTitle (인라인 편집)
│   └── DashboardActions (다운로드 버튼, 설정)
├── QueryPanel
│   ├── QueryInput            — 자연어 질문 입력 (textarea + 전송 버튼)
│   └── CotProgressPanel      — CoT 추론 단계 실시간 표시
│       └── CotStepItem[]     — 단계별 추론 텍스트 (애니메이션)
├── DashboardCanvas           — react-grid-layout 래퍼
│   └── VisualizationCard[]   — 개별 시각화 카드
│       ├── CardHeader        (제목, 편집/삭제 액션)
│       ├── ChartRenderer     — viz_type에 따라 Recharts 컴포넌트 선택
│       │   ├── LineChartView
│       │   ├── BarChartView
│       │   ├── PieChartView
│       │   ├── ScatterChartView
│       │   ├── AreaChartView
│       │   ├── HistogramView
│       │   └── DataTableView — 표 시각화 (shadcn/ui Table)
│       └── CardFooter        (다운로드 버튼)
└── StyleEditorPanel          — 우측 슬라이드인 패널 (선택된 카드 편집)
    ├── TitleEditor
    ├── ColorSchemeSelector
    ├── FontSizeControl
    └── SizePositionControls  (수동 W/H 입력)
```

#### 업로드 화면 (`/upload`)

```
UploadPage
├── FileDropzone              — react-dropzone 기반 드래그앤드롭
│   ├── DropArea              (파일 선택 또는 드래그)
│   └── FilePreview           (선택된 파일 이름, 크기)
├── UploadProgressPanel       — SSE 연결 후 진행 상태 표시
│   ├── ProgressBar
│   └── StatusMessage         (단계별 메시지)
└── DataSummaryPreview        — 요약 완료 후 스키마 / 통계 미리보기
    ├── SchemaTable
    └── StatisticsCards
```

### 3.3 컴포넌트 Props 인터페이스

#### VisualizationCard

```typescript
interface VisualizationCardProps {
  visualization: Visualization;
  isSelected: boolean;
  onSelect: (id: string) => void;
  onDelete: (id: string) => void;
  onExport: (id: string) => void;
}
```

#### ChartRenderer

```typescript
interface ChartRendererProps {
  type: VizType;           // 'line' | 'bar' | 'pie' | 'scatter' | 'table' | 'area' | 'histogram' | 'donut' | 'hbar'
  config: ChartConfig;     // 차트 데이터 및 축 설정
  style: ChartStyle;       // 색상, 폰트
  width?: number;          // 픽셀 (반응형 시 생략)
  height?: number;
}
```

#### QueryInput

```typescript
interface QueryInputProps {
  datasourceId: string;
  dashboardId: string;
  onSubmit: (question: string) => void;
  isLoading: boolean;
  disabled?: boolean;        // 쿼터 초과 시
}
```

#### DashboardCanvas

```typescript
interface DashboardCanvasProps {
  layout: GridLayout.Layout[];
  visualizations: Visualization[];
  onLayoutChange: (layout: GridLayout.Layout[]) => void;
  onVisualizationSelect: (id: string) => void;
  selectedId: string | null;
}
```

### 3.4 공통 타입 정의

```typescript
// types/index.ts

type VizType = 'line' | 'bar' | 'pie' | 'scatter' | 'table' | 'area' | 'histogram' | 'donut' | 'hbar';

interface ChartConfig {
  data: Record<string, unknown>[];
  xKey?: string;
  yKey?: string | string[];
  labelKey?: string;
  valueKey?: string;
  columns?: { key: string; label: string }[];   // DataTable용
}

interface ChartStyle {
  color_scheme: 'default' | 'blue' | 'green' | 'warm' | 'mono';
  font_size: number;         // 기본 14
  show_legend: boolean;
  show_grid: boolean;
}

interface Visualization {
  id: string;
  dashboard_id: string;
  viz_type: VizType;
  title?: string;
  chart_config: ChartConfig;
  style: ChartStyle;
  position: { x: number; y: number; w: number; h: number };
}

interface GridItem extends ReactGridLayout.Layout {
  i: string;   // visualization.id
}
```

---

## 4. AI 오케스트레이션 상세

### 4.1 내부 분석 도구 목록

AI Agent가 내부적으로 사용하는 pandas 기반 분석 함수 전체 목록.
이 도구들은 사용자 인터페이스(MCP 서버/CLI/브라우저)와 무관하게 Agent가 직접 호출하는 내부 함수입니다.

| 도구 이름 | 설명 | 입력 파라미터 | 반환 형식 |
|-----------|------|-------------|----------|
| `statistics.basic_stats` | 기본 통계 (평균, 중앙값, 표준편차, 결측치) | `datasource_id`, `columns[]` | `{column: stats_obj}` |
| `statistics.distribution` | 컬럼 분포 분석 (히스토그램 데이터) | `datasource_id`, `column`, `bins=10` | `{bins: [], counts: []}` |
| `statistics.value_counts` | 카테고리 컬럼 빈도 집계 | `datasource_id`, `column`, `top_n=10` | `[{value, count, pct}]` |
| `aggregation.group_by` | 그룹별 집계 (합계, 평균 등) | `datasource_id`, `group_col`, `agg_col`, `func=sum` | `[{group, value}]` |
| `aggregation.pivot` | 피벗 테이블 생성 | `datasource_id`, `index`, `columns`, `values`, `aggfunc` | `{index: [], columns: [], data: [[]]}` |
| `timeseries.monthly_trend` | 월별 시계열 집계 | `datasource_id`, `date_col`, `value_col`, `func=sum` | `[{month, value}]` |
| `timeseries.moving_average` | 이동평균 계산 | `datasource_id`, `date_col`, `value_col`, `window=7` | `[{date, value, ma}]` |
| `timeseries.yoy_comparison` | 전년 동기 비교 | `datasource_id`, `date_col`, `value_col` | `[{period, current, previous, growth_rate}]` |
| `correlation.pearson` | 피어슨 상관계수 행렬 | `datasource_id`, `columns[]` | `{matrix: [[]], labels: []}` |
| `correlation.top_correlated` | 특정 컬럼과 상관 높은 컬럼 Top N | `datasource_id`, `target_col`, `top_n=5` | `[{column, r_value}]` |
| `visualization.recommend` | 데이터 특성 기반 차트 유형 추천 | `datasource_id`, `question`, `columns[]` | `{recommended: VizType, reason: string, alternatives: []}` |
| `data.filter_rows` | 조건 기반 행 필터링 후 통계 | `datasource_id`, `conditions[]`, `agg_cols[]` | 필터링된 집계 결과 |
| `data.top_n_rows` | 특정 컬럼 기준 상위 N행 | `datasource_id`, `sort_col`, `n=10`, `ascending=false` | `[{...row}]` |

### 4.2 Chain of Thinking (CoT) 흐름

```
입력: 사용자 자연어 질문 + DataSource 스키마 + 도구 목록

Step 1 — 질문 분석 (Column Identification)
  <thinking>
  질문에서 언급된 측정 지표와 차원을 식별한다.
  - "월별 매출 추이" → date 컬럼(차원), sales 컬럼(지표) 필요
  - 스키마에서 매핑: date_col="order_date", value_col="revenue"
  </thinking>

Step 2 — 도구 선택 (Tool Selection)
  <thinking>
  분석 목적에 맞는 도구를 선택한다.
  - "추이" → timeseries 계열 도구
  - 월별 집계 → timeseries.monthly_trend
  - 이동평균 추가 여부 → 질문에 없으면 생략
  선택: timeseries.monthly_trend(datasource_id, date_col="order_date", value_col="revenue")
  </thinking>

Step 3 — 차트 유형 결정 (Chart Selection)
  <thinking>
  시간 + 수치 조합 → line 차트 또는 area 차트
  "추이"는 라인 차트가 적합
  선택: viz_type = "line"
  </thinking>

Step 4 — 도구 호출 (Internal Tool Call)
  → Agent가 내부 분석 함수를 직접 호출
  → pandas 기반 계산 수행
  → 결과 JSON 수신

Step 5 — 결과 조합 (Result Assembly)
  → chart_config 구성 (data, xKey, yKey)
  → Visualization 엔티티 생성
  → 대시보드에 추가
```

### 4.3 시스템 프롬프트 (Orchestrator)

```
당신은 DeepMetria의 데이터 분석 오케스트레이터입니다.

역할:
- 사용자의 자연어 질문을 분석하여 적절한 분석 도구를 선택하고 호출합니다.
- 직접 계산하지 않습니다. 반드시 제공된 도구 목록 내에서만 선택합니다.
- 각 추론 단계를 <thinking> 태그로 명시합니다.

컨텍스트:
- 데이터소스 스키마: {schema_json}
- 사용 가능한 도구: {tools_json}
- 사용자 질문: {user_question}

추론 단계:
1. 질문에서 필요한 컬럼 식별
2. 적합한 분석 도구 선택 (도구 목록 내에서만)
3. 적합한 차트 유형 결정
4. 도구 호출 실행

제약:
- 도구 목록에 없는 분석은 수행하지 않습니다.
- 데이터가 없는 컬럼을 참조하지 않습니다.
- 불분명한 경우 가장 일반적인 해석을 선택합니다.
```

### 4.4 LLM Router 설계

```python
# infrastructure/llm/router.py

class LLMRouter:
    PROVIDERS = {
        "claude": "anthropic/claude-3-5-sonnet-20241022",
        "gpt4o": "openai/gpt-4o",
        "gemini": "google/gemini-1.5-pro",
    }

    async def chat_with_tools(
        self,
        messages: list[dict],
        tools: list[dict],           # 내부 분석 도구 스키마
        provider: str = "claude",
    ) -> LLMResponse:
        model = self.PROVIDERS[provider]
        response = await litellm.acompletion(
            model=model,
            messages=messages,
            tools=tools,
            tool_choice="auto",
            stream=False,
        )
        return LLMResponse(
            content=response.choices[0].message.content,
            tool_calls=response.choices[0].message.tool_calls or [],
        )

    async def stream_chat(
        self,
        messages: list[dict],
        provider: str = "claude",
    ) -> AsyncGenerator[str, None]:
        """CoT 추론 텍스트 스트리밍"""
        async for chunk in await litellm.acompletion(
            model=self.PROVIDERS[provider],
            messages=messages,
            stream=True,
        ):
            if chunk.choices[0].delta.content:
                yield chunk.choices[0].delta.content
```

### 4.5 데이터 요약 (Summarizer) 흐름

```
파일 업로드 완료
    ↓
1. pandas로 파일 파싱 → DataFrame
2. data_schemas 테이블에 컬럼 메타데이터 저장
3. basic_stats() 내부 분석 함수 호출 → 기본 통계 수집
4. LLM에게 스키마 + 샘플 데이터 전달 → 도메인 추정 + 설명 텍스트 생성
5. data_summaries 테이블 저장
6. 기본 대시보드 생성 (요약 카드 2-3개 자동 배치)
    - 전체 행/컬럼 수 카드
    - 주요 수치 컬럼 기본 통계 테이블
    - 도메인 설명 텍스트 카드
7. SSE: summary_complete 이벤트 전송
```

---

## 5. 파일 업로드/처리 파이프라인

### 5.1 업로드 처리 흐름

```
클라이언트 → POST /api/v1/datasources/upload (multipart)
    ↓
[FastAPI] 파일 수신 (최대 50MB)
    ↓ 검증
- 파일 확장자 확인: .csv | .xlsx | .xls | .json
- Content-Type 확인 (MIME sniffing 방어)
- 파일 크기 확인 (50MB 초과 → 413)
    ↓
[BackgroundTask] 비동기 처리 시작
    ↓
1. R2/S3에 원본 파일 업로드
   - 키: uploads/{user_id}/{datasource_id}/{filename}
   - 파일은 user_id 기반 prefix로 격리
    ↓
2. DataSource 레코드 생성 (status=processing)
    ↓
3. SSE 스트림 시작 응답 반환 (202)
    ↓
[Background Worker]
4. 파일 파싱 (pandas)
   - CSV: pd.read_csv() — 인코딩 자동 감지 (chardet)
   - Excel: pd.read_excel() — openpyxl 엔진
   - JSON: pd.read_json() — orient 자동 감지
    ↓
5. 스키마 추출 → data_schemas 저장
6. AI Summarizer 실행 (4.5 참조)
7. DataSource status=ready 업데이트
8. SSE: summary_complete 이벤트
```

### 5.2 지원 파일 형식 및 제약

| 형식 | 확장자 | 최대 크기 | 최대 행 수 | 인코딩 |
|------|--------|---------|----------|------|
| CSV | .csv | 50MB | 100만 행 | UTF-8, EUC-KR 자동 감지 |
| Excel | .xlsx, .xls | 50MB | 100만 행 | N/A (openpyxl 처리) |
| JSON | .json | 50MB | 100만 행 | UTF-8 |

- 멀티시트 Excel: 첫 번째 시트만 처리 (MVP)
- 헤더 없는 CSV: 자동으로 col_0, col_1... 컬럼명 부여

### 5.3 스토리지 구조 (Cloudflare R2)

```
버킷: deepmetria-uploads
  └── uploads/
      └── {user_id}/
          └── {datasource_id}/
              └── {original_filename}   — 원본 파일 (영구 보관)
```

- presigned URL TTL: 1시간 (다운로드 전용)
- 파일 삭제: DataSource 삭제 시 R2 파일도 동기 삭제 (BackgroundTask)

### 5.4 파싱 오류 처리

| 오류 유형 | 처리 방식 | 사용자 메시지 |
|---------|---------|------------|
| 인코딩 오류 | chardet로 재시도, 실패 시 에러 | "파일 인코딩을 인식할 수 없습니다" |
| 빈 파일 | 즉시 오류 반환 | "파일에 데이터가 없습니다" |
| 헤더만 있는 파일 | 스키마는 생성, 통계 생략 | "데이터 행이 없습니다 (헤더만 존재)" |
| 파싱 중 예외 | status=error, 메시지 저장 | "파일을 처리하는 중 오류가 발생했습니다" |

---

## 6. 대시보드 레이아웃 시스템

### 6.1 react-grid-layout 설정

```typescript
// components/dashboard/DashboardCanvas.tsx

const GRID_COLS = 12;           // 12열 그리드
const ROW_HEIGHT = 80;          // 1 단위 = 80px
const DEFAULT_VIZ_W = 6;        // 기본 너비 (12열 중 6 = 50%)
const DEFAULT_VIZ_H = 4;        // 기본 높이 (4 * 80 = 320px)
const MIN_VIZ_W = 3;            // 최소 너비
const MIN_VIZ_H = 2;            // 최소 높이
const MARGIN: [number, number] = [12, 12];  // 카드 간격 (px)

<ResponsiveGridLayout
  className="layout"
  layouts={layouts}
  breakpoints={{ lg: 1200, md: 996, sm: 768 }}
  cols={{ lg: 12, md: 10, sm: 6 }}
  rowHeight={ROW_HEIGHT}
  margin={MARGIN}
  onLayoutChange={handleLayoutChange}
  draggableHandle=".drag-handle"
  resizeHandles={['se', 's', 'e']}
  isDraggable={true}
  isResizable={true}
>
  {visualizations.map(viz => (
    <div key={viz.id} data-grid={{ ...viz.position, minW: MIN_VIZ_W, minH: MIN_VIZ_H }}>
      <VisualizationCard visualization={viz} />
    </div>
  ))}
</ResponsiveGridLayout>
```

### 6.2 레이아웃 저장 전략

- **Optimistic Update**: 드래그/리사이즈 완료 시 즉시 로컬 상태 반영
- **Debounce 저장**: 500ms debounce 후 `PATCH /dashboards/{id}` 호출 (빈번한 저장 방지)
- **충돌 방지**: 저장 요청 중 추가 변경 발생 시 마지막 상태만 저장

### 6.3 기본 레이아웃 (데이터 요약 후 자동 생성)

```
[데이터 요약 완료 시 자동 배치 예시]

Row 0-1:  [요약 텍스트 카드 (w=12, h=2)]
Row 2-5:  [기본 통계 테이블 (w=6, h=4)] | [컬럼 스키마 테이블 (w=6, h=4)]
```

- 자동 배치 좌표는 `visualization.recommend` 도구 결과와 무관하게 고정 구성

### 6.4 레이아웃 데이터 저장 형식 (dashboards.layout JSONB)

```json
[
  { "i": "viz-uuid-1", "x": 0, "y": 0, "w": 12, "h": 2, "minW": 3, "minH": 2 },
  { "i": "viz-uuid-2", "x": 0, "y": 2, "w": 6,  "h": 4, "minW": 3, "minH": 2 },
  { "i": "viz-uuid-3", "x": 6, "y": 2, "w": 6,  "h": 4, "minW": 3, "minH": 2 }
]
```

---

## 7. 시각화 서식 편집 모델

### 7.1 편집 가능 속성

| 속성 그룹 | 속성 | 타입 | 기본값 | 설명 |
|---------|------|------|-------|------|
| 제목 | `title` | string | AI 생성 제목 | 시각화 카드 헤더 제목 |
| 색상 | `color_scheme` | enum | `default` | default / blue / green / warm / mono |
| 폰트 | `font_size` | number | 14 | 레이블/범례 폰트 크기 (px) |
| 범례 | `show_legend` | boolean | true | 범례 표시 여부 |
| 그리드 | `show_grid` | boolean | true | 차트 배경 그리드 여부 |
| 크기/위치 | `position.w`, `position.h` | number | 6, 4 | react-grid-layout 그리드 단위 |

### 7.2 색상 스킴 정의

```typescript
const COLOR_SCHEMES: Record<string, string[]> = {
  default: ['#6366f1', '#22d3ee', '#f97316', '#a3e635', '#f43f5e'],
  blue:    ['#3b82f6', '#60a5fa', '#93c5fd', '#bfdbfe', '#dbeafe'],
  green:   ['#22c55e', '#4ade80', '#86efac', '#bbf7d0', '#dcfce7'],
  warm:    ['#f97316', '#fb923c', '#fbbf24', '#fde047', '#fef08a'],
  mono:    ['#1e293b', '#475569', '#94a3b8', '#cbd5e1', '#e2e8f0'],
};
```

### 7.3 StyleEditor 동작 방식

1. 사용자가 VisualizationCard를 클릭 → `selectedId` 상태 업데이트
2. 우측 StyleEditorPanel 슬라이드인
3. 편집 시 즉시 로컬 상태(Zustand) 업데이트 → 차트 실시간 리렌더
4. 패널 닫기 또는 3초 idle 시 `PATCH /visualizations/{id}` 저장
5. 저장 실패 시 이전 상태로 롤백 + Toast 오류 알림

### 7.4 Recharts 컴포넌트 매핑

| viz_type | Recharts 컴포넌트 | 특이사항 |
|---------|-----------------|---------|
| `line` | `<LineChart>` | 다중 Y축 지원 |
| `bar` | `<BarChart>` | stacked 옵션 |
| `hbar` | `<BarChart layout="vertical">` | 수평 바 |
| `area` | `<AreaChart>` | fillOpacity 0.3 |
| `pie` | `<PieChart>` | — |
| `donut` | `<PieChart>` | `innerRadius` 설정 |
| `scatter` | `<ScatterChart>` | — |
| `histogram` | `<BarChart>` | 커스텀 데이터 전처리 |
| `table` | `<Table>` (shadcn/ui) | 정렬, 페이지네이션 지원 |

---

## 8. 에러 핸들링 전략

### 8.1 에러 분류

| 레이어 | 에러 유형 | 처리 방식 |
|------|---------|---------|
| 프론트엔드 | 네트워크 오류 | Toast 알림 + 재시도 버튼 |
| 프론트엔드 | SSE 연결 끊김 | 자동 재연결 (최대 3회, exponential backoff) |
| 백엔드 API | 검증 오류 (422) | 필드별 에러 메시지 폼에 표시 |
| 백엔드 API | 인증 오류 (401) | 로그인 페이지로 리다이렉트 |
| 백엔드 API | 쿼터 초과 (402) | 업그레이드 안내 모달 표시 |
| 백엔드 API | 서버 오류 (500) | 일반 오류 Toast + 에러 코드 표시 |
| LLM API | Timeout / Rate Limit | AnalysisFlow status=failed + SSE error 이벤트 |
| 파일 처리 | 파싱 오류 | DataSource status=error + 상세 메시지 |
| 내부 분석 도구 | 함수 실행 오류 | CoT 재시도 1회 → 실패 시 flow 실패 처리 |

### 8.2 백엔드 공통 예외 처리

```python
# api/middleware/error_handler.py

class AppException(Exception):
    def __init__(self, code: str, message: str, status_code: int = 400):
        self.code = code
        self.message = message
        self.status_code = status_code

# 공통 예외 타입
class ValidationError(AppException): ...         # 422
class NotFoundError(AppException): ...           # 404
class UnauthorizedError(AppException): ...       # 401
class QuotaExceededError(AppException): ...      # 402
class LLMProviderError(AppException): ...        # 502
class FileProcessingError(AppException): ...     # 422

@app.exception_handler(AppException)
async def app_exception_handler(request, exc: AppException):
    return JSONResponse(
        status_code=exc.status_code,
        content={"error": {"code": exc.code, "message": exc.message}},
    )
```

### 8.3 SSE 에러 처리

- SSE 연결 중 예외 발생 시 `event: error` 이벤트 전송 후 스트림 종료
- 클라이언트는 `error` 이벤트 수신 시 연결 닫고 Toast 표시
- SSE 자동 재연결: `EventSource` 기본 재연결 사용 (분석 완료 후에는 재연결 차단)

### 8.4 LLM API 장애 대응

```
LLM API 호출 실패 시:
1. 설정된 provider로 1차 시도
2. Timeout(30초) 또는 Rate Limit → 다른 provider로 폴백 (설정된 경우)
3. 모든 provider 실패 → LLMProviderError 발생
4. AnalysisFlow status=failed 업데이트
5. SSE error 이벤트 전송
6. 사용자에게 "AI 서비스 일시 불안정" 메시지 표시 + 재시도 버튼
```

### 8.5 프론트엔드 에러 바운더리

```typescript
// components/ErrorBoundary.tsx
// 각 VisualizationCard를 ErrorBoundary로 감싸 개별 차트 오류가
// 전체 대시보드를 망가뜨리지 않도록 격리
```

---

## 9. 상태 관리 설계

### 9.1 Zustand 스토어 구조

```typescript
// store/dashboardStore.ts
interface DashboardStore {
  currentDashboard: Dashboard | null;
  visualizations: Map<string, Visualization>;
  layout: GridLayout.Layout[];
  selectedVisualizationId: string | null;

  // Actions
  setDashboard: (dashboard: Dashboard) => void;
  addVisualization: (viz: Visualization) => void;
  updateVisualization: (id: string, patch: Partial<Visualization>) => void;
  removeVisualization: (id: string) => void;
  updateLayout: (layout: GridLayout.Layout[]) => void;
  selectVisualization: (id: string | null) => void;
}

// store/analysisStore.ts
interface AnalysisStore {
  activeFlowId: string | null;
  flowStatus: 'idle' | 'pending' | 'running' | 'completed' | 'failed';
  cotSteps: CotStep[];
  isStreaming: boolean;

  // Actions
  startFlow: (flowId: string) => void;
  appendCotStep: (step: CotStep) => void;
  completeFlow: () => void;
  failFlow: (error: string) => void;
  reset: () => void;
}

// store/userStore.ts
interface UserStore {
  user: User | null;
  isAuthenticated: boolean;
  quota: { used: number; limit: number } | null;

  // Actions
  setUser: (user: User) => void;
  clearUser: () => void;
  updateQuota: (quota: { used: number; limit: number }) => void;
}
```

### 9.2 SSE 연결 관리

```typescript
// lib/sse/useSseStream.ts

function useSseStream(streamUrl: string | null) {
  const [isConnected, setConnected] = useState(false);

  useEffect(() => {
    if (!streamUrl) return;
    const es = new EventSource(streamUrl, { withCredentials: true });
    let retryCount = 0;
    const MAX_RETRY = 3;

    es.addEventListener('cot_step', (e) => {
      const step = JSON.parse(e.data);
      useAnalysisStore.getState().appendCotStep(step);
    });

    es.addEventListener('visualization_ready', (e) => {
      const viz = JSON.parse(e.data);
      useDashboardStore.getState().addVisualization(viz);
    });

    es.addEventListener('done', () => {
      useAnalysisStore.getState().completeFlow();
      es.close();
    });

    es.addEventListener('error', (e) => {
      if (retryCount >= MAX_RETRY) {
        useAnalysisStore.getState().failFlow('연결 오류');
        es.close();
      }
      retryCount++;
    });

    setConnected(true);
    return () => es.close();
  }, [streamUrl]);

  return { isConnected };
}
```

### 9.3 서버-클라이언트 상태 동기화 규칙

| 상태 | 저장 위치 | 동기화 방식 |
|------|---------|-----------|
| 대시보드 레이아웃 | Zustand + DB | debounce 500ms 후 PATCH |
| 시각화 스타일 | Zustand + DB | 패널 닫기 또는 3초 idle 후 PATCH |
| CoT 추론 단계 | Zustand only | SSE 수신 시 append, 페이지 이동 시 초기화 |
| 사용자 정보/쿼터 | Zustand + 서버 | 로그인 시 fetch, 분석 완료 시 갱신 |
| 업로드 진행 상태 | Zustand only | SSE 수신 시 업데이트, 완료 시 초기화 |

### 9.4 쿼터 관리 흐름

```
분석 요청 전:
1. userStore.quota 확인 → used >= limit 이면 QueryInput disabled + 업그레이드 안내

분석 완료 후:
1. GET /auth/me 호출 → 최신 quota 갱신
2. userStore.updateQuota(latest)

서버 측:
1. POST /analysis/query 수신 시 Redis quota:{user_id}:{month} 확인
2. 초과 시 402 QuotaExceededError 반환
3. 완료 시 Redis + DB usage_quotas 동시 업데이트
```
