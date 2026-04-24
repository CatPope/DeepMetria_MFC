# 데이터베이스 스키마

## 목차
1. 개요 (Overview) .............. L17
2. ERD (엔티티 관계) .............. L33
3. 테이블 정의 .............. L65
4. 인덱스 .............. L180
5. 스키마 변경 이력 .............. L201






---

## 1. 개요

DeepMetria MFC는 SQLite를 로컬 데이터베이스로 사용합니다. 스키마는 6개 테이블로 구성되며,
로컬 설정, 데이터소스 관리, 대시보드/시각화, AI 분석 플로우를 지원합니다.

**주요 특징:**
- INTEGER PK (AUTOINCREMENT)
- TEXT 타입으로 ISO 8601 타임스탬프 저장
- ON DELETE CASCADE로 고아 행 방지
- TEXT 타입으로 JSON 직렬화 데이터 저장 (레이아웃, 차트 설정, CoT 단계 등)
- CHECK 제약으로 상태값 검증

**SQLite 연결 설정 (ODBC):**
- 드라이버: `SQLite3 ODBC Driver` 또는 직접 `sqlite3.h` 링킹
- DB 파일 경로: `%APPDATA%\DeepMetria\deepmetria.db`
- WAL 모드 활성화: `PRAGMA journal_mode=WAL;`
- 외래키 활성화: `PRAGMA foreign_keys=ON;`

---

## 2. ERD (엔티티 관계)

```
APP_SETTINGS (단독)

DATASOURCES ||--o{ DATA_SCHEMAS : defines
DATASOURCES ||--o{ DATA_SUMMARIES : has
DATASOURCES ||--o{ DASHBOARDS : powers
DATASOURCES ||--o{ ANALYSIS_FLOWS : analyzes
DASHBOARDS ||--o{ VISUALIZATIONS : contains
ANALYSIS_FLOWS ||--o{ VISUALIZATIONS : generates
```

**테이블 목록:**

| 테이블 | 설명 |
|--------|------|
| app_settings | 앱 설정 (API 키, LLM 기본값 등) |
| datasources | 로드된 데이터 파일 메타데이터 |
| data_schemas | 데이터소스 컬럼 메타데이터 |
| data_summaries | 데이터소스 통계 요약 |
| dashboards | 사용자 대시보드 |
| visualizations | 대시보드 내 차트/시각화 |
| analysis_flows | AI 분석 플로우 실행 이력 |

---

## 3. 테이블 정의

### 3.1 app_settings

앱 전역 설정을 key-value 형태로 저장합니다.

```sql
CREATE TABLE app_settings (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL,
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);
```

| 키 예시 | 설명 |
|---------|------|
| `llm_provider` | 기본 LLM 제공자 (anthropic/openai/gemini) |
| `llm_model` | 기본 모델명 |
| `anthropic_api_key` | Anthropic API 키 (암호화 저장) |
| `openai_api_key` | OpenAI API 키 (암호화 저장) |
| `gemini_api_key` | Google Gemini API 키 (암호화 저장) |
| `theme` | UI 테마 (light/dark) |

---

### 3.2 datasources

로드된 데이터 파일 메타데이터를 저장합니다.

```sql
CREATE TABLE datasources (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    name         TEXT    NOT NULL,
    file_path    TEXT    NOT NULL,
    file_type    TEXT    NOT NULL CHECK(file_type IN ('csv','excel','json')),
    file_size    INTEGER NOT NULL,
    row_count    INTEGER,
    column_count INTEGER,
    status       TEXT    NOT NULL DEFAULT 'processing'
                         CHECK(status IN ('processing','ready','error')),
    error_message TEXT,
    created_at   TEXT    NOT NULL DEFAULT (datetime('now')),
    updated_at   TEXT    NOT NULL DEFAULT (datetime('now'))
);
```

---

### 3.3 data_schemas

각 데이터소스의 컬럼 메타데이터를 저장합니다.

```sql
CREATE TABLE data_schemas (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    datasource_id  INTEGER NOT NULL REFERENCES datasources(id) ON DELETE CASCADE,
    column_name    TEXT    NOT NULL,
    column_index   INTEGER NOT NULL,
    dtype          TEXT    NOT NULL,
    nullable       INTEGER NOT NULL DEFAULT 1,
    sample_values  TEXT,
    created_at     TEXT    NOT NULL DEFAULT (datetime('now'))
);
```

---

### 3.4 data_summaries

각 데이터소스의 통계 요약을 저장합니다 (datasource_id당 최대 1행).

```sql
CREATE TABLE data_summaries (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    datasource_id INTEGER NOT NULL UNIQUE REFERENCES datasources(id) ON DELETE CASCADE,
    domain        TEXT,
    description   TEXT,
    statistics    TEXT    NOT NULL DEFAULT '{}',
    row_count     INTEGER NOT NULL DEFAULT 0,
    created_at    TEXT    NOT NULL DEFAULT (datetime('now')),
    updated_at    TEXT    NOT NULL DEFAULT (datetime('now'))
);
```

---

### 3.5 dashboards

사용자가 생성한 대시보드를 저장합니다.

```sql
CREATE TABLE dashboards (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    datasource_id INTEGER NOT NULL REFERENCES datasources(id) ON DELETE CASCADE,
    name          TEXT    NOT NULL,
    layout        TEXT    NOT NULL DEFAULT '[]',
    created_at    TEXT    NOT NULL DEFAULT (datetime('now')),
    updated_at    TEXT    NOT NULL DEFAULT (datetime('now')),
    deleted_at    TEXT
);
```

---

### 3.6 visualizations

대시보드에 포함된 개별 차트/시각화를 저장합니다.

```sql
CREATE TABLE visualizations (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    dashboard_id INTEGER NOT NULL REFERENCES dashboards(id) ON DELETE CASCADE,
    flow_id      INTEGER REFERENCES analysis_flows(id) ON DELETE SET NULL,
    viz_type     TEXT    NOT NULL
                         CHECK(viz_type IN ('line','bar','pie','scatter','table',
                                            'area','histogram','donut','hbar')),
    title        TEXT,
    chart_config TEXT    NOT NULL DEFAULT '{}',
    style        TEXT    NOT NULL DEFAULT '{}',
    position     TEXT    NOT NULL DEFAULT '{}',
    created_at   TEXT    NOT NULL DEFAULT (datetime('now')),
    updated_at   TEXT    NOT NULL DEFAULT (datetime('now'))
);
```

---

### 3.7 analysis_flows

AI 기반 분석 플로우 실행 이력을 저장합니다.

```sql
CREATE TABLE analysis_flows (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    datasource_id INTEGER NOT NULL REFERENCES datasources(id) ON DELETE CASCADE,
    dashboard_id  INTEGER REFERENCES dashboards(id) ON DELETE SET NULL,
    question      TEXT    NOT NULL,
    status        TEXT    NOT NULL DEFAULT 'pending'
                          CHECK(status IN ('pending','running','completed','failed')),
    llm_provider  TEXT,
    llm_model     TEXT,
    cot_steps     TEXT    NOT NULL DEFAULT '[]',
    tool_calls    TEXT    NOT NULL DEFAULT '[]',
    error_message TEXT,
    created_at    TEXT    NOT NULL DEFAULT (datetime('now')),
    updated_at    TEXT    NOT NULL DEFAULT (datetime('now'))
);
```

---

## 4. 인덱스

쿼리 성능 최적화를 위한 인덱스 목록:

```sql
CREATE INDEX idx_datasources_status      ON datasources(status);
CREATE INDEX idx_data_schemas_datasource ON data_schemas(datasource_id);
CREATE INDEX idx_dashboards_datasource   ON dashboards(datasource_id)
    WHERE deleted_at IS NULL;
CREATE INDEX idx_visualizations_dashboard ON visualizations(dashboard_id);
CREATE INDEX idx_analysis_flows_datasource ON analysis_flows(datasource_id);
CREATE INDEX idx_analysis_flows_status     ON analysis_flows(status);
```

---

## 5. 스키마 변경 이력

| 버전 | 설명 | 적용일 |
|------|------|--------|
| 0001 | 초기 스키마 생성 (7개 테이블) | 2026-04-24 |

**초기화 실행 (C++ 코드 내):**
```cpp
// CDeepMetriaApp::InitDatabase()
sqlite3* db = nullptr;
sqlite3_open(dbPath, &db);
sqlite3_exec(db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);
sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
// schema SQL 실행
```
