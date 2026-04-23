# DeepMetria 테스트 계획서 — LLM 기반 데이터 분석 SaaS 플랫폼

## 목차
1. 테스트 범위 ................................................ L13
2. 테스트 유형 및 도구 ........................................ L41
3. 테스트 환경 ................................................ L116
4. 테스트 케이스 (FR별) ...................................... L152
5. 합격 기준 .................................................. L294
6. 리스크 및 대응 전략 ........................................ L338

---

## 1. 테스트 범위

### 1.1 테스트 대상

| 범주 | 대상 |
|------|------|
| 기능 요구사항 | FR-01 ~ FR-08 (파일 업로드, AI 요약, 자연어 질문, CoT 분석, 시각화 생성, 서식 편집, 다운로드, 멀티 LLM) |
| API 엔드포인트 | Auth(4개), DataSource(5개), Analysis(3개), Dashboard(5개), Visualization(3개), Export(2개), MCP(관련 tool) |
| 프론트엔드 컴포넌트 | FileDropzone, DataSummaryView, Dashboard, QueryInput, ChartRenderer, FormatEditor, DownloadButton |
| 백엔드 도메인 로직 | 파일 파서(CSV/Excel/JSON), LLM Router, AnalysisTool 13개, 스트리밍(SSE) |
| CLI 명령어 | auth(3개), datasource(4개), analysis(1개), dashboard(2개), viz(1개) |
| MCP Server | tool 등록, tool 호출, 오류 처리 |
| 비기능 요구사항 | NFR-01 성능, NFR-02 사용성, NFR-03 신뢰성 |
| 보안 | OWASP Top 10, JWT 인증/인가, 파일 업로드 검증, 데이터 격리 |

### 1.2 테스트 제외 범위

| 제외 항목 | 이유 |
|-----------|------|
| 과금/결제 시스템 (FR-P03) | post-MVP 범위 외 |
| DB/API 외부 직접 연결 (FR-P01, FR-P02) | post-MVP 범위 외 |
| 온프레미스 배포 (FR-P05) | post-MVP 범위 외 |
| MCP/CLI/Skills 외부 공개 (FR-P04) | post-MVP 범위 외 |
| 실제 LLM API 호출 | 비결정성 및 비용 문제 → mock으로 대체 |
| 실제 R2 스토리지 | 외부 의존성 → mock/localstack으로 대체 |

---

## 2. 테스트 유형 및 도구

### 2.1 테스트 유형 매트릭스

| 유형 | 도구 | 대상 | 커버리지 목표 |
|------|------|------|--------------|
| 백엔드 단위 테스트 | pytest + pytest-asyncio | 도메인 로직, AnalysisTool 13개, 파일 파서, LLM Router | ≥ 80% |
| 프론트엔드 단위 테스트 | vitest + React Testing Library | 컴포넌트, Zustand stores | ≥ 80% |
| CLI 단위 테스트 | pytest + click.testing.CliRunner | 5개 CLI 명령어 그룹 (auth, datasource, analysis, dashboard, viz) | ≥ 80% |
| 백엔드 통합 테스트 | pytest + httpx (TestClient) | FastAPI 엔드포인트 22개 전체 | ≥ 80% |
| E2E 테스트 | Playwright | 핵심 플로우 (업로드→요약→질문→시각화→다운로드) | 핵심 플로우 100% |
| 성능 테스트 | locust 또는 k6 | API 응답시간, 동시 사용자 처리 | NFR-01 기준 충족 |
| 보안 테스트 | OWASP ZAP + 수동 점검 | OWASP Top 10, JWT, 파일 업로드 보안 | 위반 0건 |

### 2.2 백엔드 단위 테스트 세부 대상

```
tests/unit/
├── test_file_parser.py        # CSV, Excel, JSON 파서
├── test_llm_router.py         # LLM Router 추상화 (mock LLM)
├── test_analysis_tools.py     # AnalysisTool 13개
├── test_auth_service.py       # JWT 발급/검증, 비밀번호 해시
├── test_datasource_service.py # DataSource 생성/검증 로직
├── test_streaming.py          # SSE 이벤트 생성 로직
└── test_mcp_server.py         # MCP tool 등록/호출/오류 처리
```

### 2.3 프론트엔드 단위 테스트 세부 대상

```
src/__tests__/
├── components/FileDropzone.test.tsx
├── components/ChartRenderer.test.tsx
├── components/FormatEditor.test.tsx
├── components/DataSummaryView.test.tsx
├── stores/dashboardStore.test.ts
├── stores/analysisStore.test.ts
└── stores/userStore.test.ts
```

### 2.3.1 CLI 단위 테스트 세부 대상

```
tests/cli/
├── test_auth_commands.py      # login, logout, whoami
├── test_datasource_commands.py # upload, datasource list/get/delete
├── test_analysis_commands.py  # analyze (자연어 질문)
├── test_dashboard_commands.py # dashboard list/get
└── test_viz_commands.py       # viz export
```

### 2.4 통합 테스트 세부 대상

```
tests/integration/
├── test_auth_endpoints.py       # POST /auth/register, login, refresh, GET /auth/me
├── test_datasource_endpoints.py # POST /upload, GET list/detail/stream, DELETE
├── test_analysis_endpoints.py   # POST /query, GET /stream, GET /{id}
├── test_dashboard_endpoints.py  # CRUD 5개 엔드포인트
├── test_visualization_endpoints.py
└── test_export_endpoints.py     # GET /export/{id}/image, GET /export/dashboard/{id}
```

### 2.5 E2E 시나리오

```
e2e/
├── upload-to-summary.spec.ts    # 파일 업로드 → 요약 자동 생성
├── query-to-visualization.spec.ts # 자연어 질문 → 시각화 생성
├── format-and-download.spec.ts  # 서식 편집 → 다운로드
└── multi-llm-switch.spec.ts     # LLM 전환 후 기능 동작
```

---

## 3. 테스트 환경

### 3.1 로컬 환경

| 항목 | 구성 |
|------|------|
| 컨테이너 | docker-compose (PostgreSQL 15, Redis 7) |
| 백엔드 | FastAPI (uvicorn, 테스트 모드) |
| 프론트엔드 | Next.js 14 (test 환경) |
| LLM | mock (pytest-mock / vitest mock) — 실제 API 호출 없음 |
| 파일 스토리지 | 로컬 임시 디렉터리 또는 localstack S3 호환 |

### 3.2 CI 환경 (GitHub Actions)

```yaml
# .github/workflows/test.yml 기준
- PostgreSQL 서비스 컨테이너
- Redis 서비스 컨테이너
- Python 3.11 + Node.js 20
- pytest (백엔드) + vitest (프론트엔드) 병렬 실행
- Playwright 헤드리스 모드 (E2E)
```

### 3.3 테스트 데이터 (Fixtures)

| 파일 | 내용 | 용도 |
|------|------|------|
| `tests/fixtures/sample_sales.csv` | 100행 매출 데이터 | 기본 업로드/분석 테스트 |
| `tests/fixtures/sample_data.xlsx` | 멀티시트 Excel | Excel 파서 테스트 |
| `tests/fixtures/sample_data.json` | 중첩 JSON | JSON 파서 테스트 |
| `tests/fixtures/large_file_50mb.csv` | 50MB 경계값 | 파일 크기 제한 테스트 |
| `tests/fixtures/invalid_file.txt` | 지원 불가 형식 | 오류 처리 테스트 |
| `tests/fixtures/malicious_upload.php` | 악성 파일 시도 | 보안 업로드 테스트 |

---

## 4. 테스트 케이스 (FR별)

### 4.1 FR-01: 데이터 파일 업로드

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-01-01 | CSV 파일 정상 업로드 | 통합 | sample_sales.csv | 202, datasource_id 반환, status=processing |
| TC-01-02 | Excel(.xlsx) 파일 정상 업로드 | 통합 | sample_data.xlsx | 202, datasource_id 반환 |
| TC-01-03 | Excel(.xls) 파일 정상 업로드 | 통합 | sample_data.xls | 202, datasource_id 반환 |
| TC-01-04 | JSON 파일 정상 업로드 | 통합 | sample_data.json | 202, datasource_id 반환 |
| TC-01-05 | 지원 불가 형식 업로드 | 통합 | invalid_file.txt | 400 UNSUPPORTED_FILE_TYPE |
| TC-01-06 | 50MB 초과 파일 업로드 | 통합 | large_file_51mb.csv | 413 FILE_TOO_LARGE |
| TC-01-07 | 50MB 경계값 파일 업로드 | 통합 | large_file_50mb.csv | 202, 정상 처리 |
| TC-01-08 | 인증 없이 업로드 시도 | 통합 | 유효 CSV, 토큰 없음 | 401 Unauthorized |
| TC-01-09 | 악성 파일 업로드 시도 | 보안 | malicious_upload.php | 400 UNSUPPORTED_FILE_TYPE |
| TC-01-10 | FileDropzone — 드래그앤드롭 | E2E | CSV 파일 드래그 | 업로드 진행 상태 표시됨 |
| TC-01-11 | FileDropzone — 파일 선택 버튼 | E2E | CSV 파일 선택 | 업로드 진행 상태 표시됨 |

### 4.2 FR-02: AI 데이터 자동 요약

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-02-01 | 업로드 후 SSE 스트림 수신 | 통합 | datasource_id | stage: uploading→parsing→summarizing→done 순서 이벤트 |
| TC-02-02 | summary_complete 이벤트 포함 확인 | 통합 | datasource_id | summary(domain, row_count, statistics), dashboard_id 포함 |
| TC-02-03 | DataSource 상세 조회 시 스키마 포함 | 통합 | GET /datasources/{id} | schema(column, dtype, nullable) 배열 반환 |
| TC-02-04 | 기본 대시보드 자동 생성 확인 | 통합 | summary_complete 후 | dashboard_id로 GET /dashboards/{id} 200 반환 |
| TC-02-05 | 요약 컴포넌트 렌더링 | 단위(FE) | DataSummary 객체 | 컬럼 스키마, 도메인, 통계 항목 렌더링됨 |
| TC-02-06 | LLM mock 요약 생성 도메인 로직 | 단위(BE) | CSV 파서 출력 | domain, statistics, row_count 포함 dict 반환 |

### 4.3 FR-03: 자연어 질문 입력

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-03-01 | 정상 질문 제출 | 통합 | POST /analysis/query, "월별 매출 추이" | 202, flow_id 반환 |
| TC-03-02 | 빈 질문 제출 | 통합 | question="" | 422 EMPTY_QUESTION |
| TC-03-03 | 존재하지 않는 datasource_id | 통합 | 잘못된 UUID | 404 DATASOURCE_NOT_FOUND |
| TC-03-04 | 질문 입력 UI 로딩 상태 표시 | E2E | 질문 제출 후 | 스피너/로딩 인디케이터 표시됨 |
| TC-03-05 | QueryInput 컴포넌트 제출 동작 | 단위(FE) | 텍스트 입력 후 Enter | onSubmit 핸들러 호출됨 |

### 4.4 FR-04: AI 분석 도구 선택 및 호출 (CoT)

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-04-01 | CoT SSE 스트림 — cot_step 이벤트 수신 | 통합 | GET /analysis/{id}/stream | cot_step 이벤트 1개 이상 수신 |
| TC-04-02 | tool_call 이벤트 수신 | 통합 | 분석 스트림 | tool 이름이 정의된 AnalysisTool 목록 내 값 |
| TC-04-03 | tool_result 이벤트 수신 | 통합 | 분석 스트림 | tool_result 이벤트 포함, result 비어있지 않음 |
| TC-04-04 | AnalysisTool 13개 각 도구 단위 테스트 | 단위(BE) | 각 도구별 입력 fixture | 올바른 계산 결과 반환 |
| TC-04-05 | LLM Router — Claude mock 라우팅 | 단위(BE) | provider="claude" | claude mock 호출됨 |
| TC-04-06 | LLM Router — GPT mock 라우팅 | 단위(BE) | provider="openai" | openai mock 호출됨 |
| TC-04-07 | 정의되지 않은 도구 선택 시 오류 처리 | 단위(BE) | 존재하지 않는 tool명 | ToolNotFoundError 발생 |

### 4.5 FR-05: 차트 시각화 생성

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-05-01 | visualization_ready 이벤트 수신 | 통합 | 분석 스트림 완료 | visualization_id, type, config 포함 |
| TC-05-02 | 대시보드에 시각화 추가 확인 | 통합 | GET /dashboards/{id} | visualizations 배열에 신규 항목 포함 |
| TC-05-03 | 라인 차트 렌더링 | 단위(FE) | type="line", config=mock | Recharts LineChart 컴포넌트 렌더링됨 |
| TC-05-04 | 바 차트 렌더링 | 단위(FE) | type="bar", config=mock | Recharts BarChart 컴포넌트 렌더링됨 |
| TC-05-05 | 테이블 시각화 렌더링 | 단위(FE) | type="table", data=mock | 테이블 행/열 올바르게 렌더링됨 |
| TC-05-06 | E2E — 질문 후 차트 대시보드 표시 | E2E | "월별 매출 추이 보여줘" | 차트 컴포넌트가 대시보드에 나타남 |

### 4.6 FR-06: 시각화 서식 편집

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-06-01 | 시각화 크기 변경 API | 통합 | PATCH /visualizations/{id}, size 변경 | 200, 변경된 size 반환 |
| TC-06-02 | 시각화 위치 변경 API | 통합 | PATCH /dashboards/{id} layout 업데이트 | 200, layout 갱신됨 |
| TC-06-03 | 시각화 스타일(색상) 변경 API | 통합 | PATCH /visualizations/{id}, style 변경 | 200, 변경된 style 반환 |
| TC-06-04 | FormatEditor 크기 조절 UI | 단위(FE) | 높이/너비 슬라이더 조작 | onChange 핸들러 호출됨 |
| TC-06-05 | E2E — 드래그로 위치 변경 | E2E | 차트 드래그 | 레이아웃 업데이트 API 호출됨 |
| TC-06-06 | E2E — 서식 변경 즉시 반영 | E2E | 색상 변경 | 대시보드에 즉시 시각적 반영 |

### 4.7 FR-07: 시각화 다운로드

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-07-01 | PNG 다운로드 API | 통합 | GET /export/{id}/image | 200, Content-Type: image/png |
| TC-07-02 | 대시보드 전체 다운로드 API | 통합 | GET /export/dashboard/{id} | 200, 파일 다운로드 응답 |
| TC-07-03 | DownloadButton 클릭 동작 | 단위(FE) | 버튼 클릭 | 다운로드 API 호출 트리거됨 |
| TC-07-04 | E2E — 다운로드 버튼 클릭 → 파일 저장 | E2E | 다운로드 버튼 클릭 | 파일 다운로드 발생 (PNG) |

### 4.8 FR-08: 멀티 LLM 지원

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-08-01 | Claude mock으로 분석 완료 | 단위(BE) | provider="claude" | 정상 분석 결과 반환 |
| TC-08-02 | GPT mock으로 분석 완료 | 단위(BE) | provider="openai" | 정상 분석 결과 반환 |
| TC-08-03 | LLM 전환 후 동일 결과 구조 보장 | 단위(BE) | 두 provider 결과 비교 | 응답 스키마 동일 |
| TC-08-04 | 지원하지 않는 LLM provider | 단위(BE) | provider="unknown" | UnsupportedProviderError 발생 |
| TC-08-05 | LLM API 오류 시 사용자 안내 | 통합 | LLM mock 에러 반환 | SSE error 이벤트, 재시도 안내 메시지 포함 |

### 4.10 CLI 테스트

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-CLI-01 | login 성공 | 단위(CLI) | 유효한 이메일/비밀번호 | config.json에 토큰 저장, 성공 메시지 |
| TC-CLI-02 | login 실패 — 잘못된 비밀번호 | 단위(CLI) | 잘못된 비밀번호 | 에러 메시지, exit code 1 |
| TC-CLI-03 | whoami — 인증됨 | 단위(CLI) | 저장된 토큰 | 사용자 정보 출력 |
| TC-CLI-04 | whoami — 미인증 | 단위(CLI) | 토큰 없음 | "로그인이 필요합니다" 메시지 |
| TC-CLI-05 | upload CSV 파일 | 단위(CLI) | sample_sales.csv 경로 | 업로드 성공 메시지, datasource_id 출력 |
| TC-CLI-06 | analyze 자연어 질문 | 단위(CLI) | "월별 매출 추이" | 분석 결과 스트리밍 출력 |
| TC-CLI-07 | dashboard list | 단위(CLI) | 인증 상태 | 대시보드 목록 테이블 출력 |
| TC-CLI-08 | viz export | 단위(CLI) | visualization_id | PNG 파일 다운로드 |
| TC-CLI-09 | 서버 미응답 시 오류 처리 | 단위(CLI) | 서버 다운 상태 | 연결 오류 메시지, exit code 1 |

### 4.11 MCP Server 테스트

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-MCP-01 | MCP 서버 초기화 | 단위(BE) | 서버 시작 | tool 목록 등록 완료 |
| TC-MCP-02 | 등록된 tool 호출 | 단위(BE) | 유효한 tool 이름 + 파라미터 | 정상 결과 반환 |
| TC-MCP-03 | 미등록 tool 호출 | 단위(BE) | 존재하지 않는 tool 이름 | 적절한 오류 메시지 |
| TC-MCP-04 | 잘못된 파라미터 전달 | 단위(BE) | 유효하지 않은 파라미터 | 파라미터 검증 오류 |

### 4.9 비기능 요구사항 테스트

#### 성능 (NFR-01)

| TC-ID | 테스트 케이스 | 도구 | 기준 |
|-------|-------------|------|------|
| TC-P-01 | 파일 업로드 후 요약 생성 시간 | locust/k6 | ≤ 30초 (중소형 파일) |
| TC-P-02 | 자연어 질문 → 시각화 생성 시간 | locust/k6 | ≤ 60초 (LLM mock 포함) |
| TC-P-03 | 대시보드 페이지 초기 로딩 | Playwright + k6 | ≤ 3초 |
| TC-P-04 | 비-분석 API 응답 시간 (P95) | k6 | ≤ 2초 |
| TC-P-05 | 동시 사용자 10명 처리 | locust | 오류율 < 1% |

#### 보안 (NFR-보안)

| TC-ID | 테스트 케이스 | 도구 | 기준 |
|-------|-------------|------|------|
| TC-S-01 | JWT 없이 보호된 엔드포인트 접근 | 수동/ZAP | 401 반환 |
| TC-S-02 | 타 사용자 데이터 접근 시도 | 수동 | 403 또는 404 반환 |
| TC-S-03 | SQL Injection 시도 | OWASP ZAP | 오류 노출 없음 |
| TC-S-04 | XSS 공격 시도 | OWASP ZAP | 스크립트 실행 안됨 |
| TC-S-05 | 악성 파일 업로드 (MIME 우회) | 수동 | 400 거부 |
| TC-S-06 | LLM API 키 클라이언트 노출 여부 | 수동 | 응답/헤더에 키 미포함 |
| TC-S-07 | HTTPS 전용 통신 확인 | 수동 | HTTP 요청 리다이렉트 또는 거부 |
| TC-S-08 | Expired JWT로 접근 | 수동 | 401 TOKEN_EXPIRED |

---

## 5. 합격 기준

### 5.1 정량적 기준

| 항목 | 합격 기준 |
|------|----------|
| 테스트 통과율 | 100% (모든 TC 통과) |
| 코드 커버리지 | ≥ 80% (백엔드 pytest-cov, 프론트엔드 vitest coverage) |
| Cyclomatic 복잡도 | ≤ 10 (함수 단위, 린터로 CI 게이트) |
| 비-분석 API 응답 시간 (P95) | ≤ 2초 |
| 데이터 요약 생성 시간 | ≤ 30초 (중소형 파일) |
| 자연어 질문 → 시각화 | ≤ 60초 |
| 대시보드 초기 로딩 | ≤ 3초 |
| 크리티컬 버그 | 0건 |
| OWASP Top 10 위반 | 0건 |

### 5.2 게이트 기준 (CI/CD 블로킹)

아래 기준 미충족 시 PR 머지 차단:

1. 백엔드 단위/통합 테스트 전체 통과
2. 프론트엔드 단위 테스트 전체 통과
3. 코드 커버리지 ≥ 80%
4. lsp/타입 오류 0건
5. Cyclomatic 복잡도 초과 함수 0개

E2E 및 성능/보안 테스트는 배포 전 수동 게이트로 운영 (CI에서 선택적 실행).

### 5.3 인수 기준 매핑 (SRS AC)

| AC-ID | 인수 기준 내용 | 대응 TC |
|-------|--------------|---------|
| AC-01 | CSV/Excel/JSON 업로드 후 데이터 요약 자동 생성 | TC-01-01~04, TC-02-01~02 |
| AC-02 | 지원 불가 형식 업로드 시 오류 메시지 | TC-01-05, TC-01-09 |
| AC-03 | 요약에 컬럼 스키마, 도메인, 기본 통계 포함 | TC-02-03, TC-02-06 |
| AC-04 | 업로드 후 기본 대시보드 자동 구성 | TC-02-04 |
| AC-05 | 자연어 질문 UI 및 AI 분석 시작 피드백 | TC-03-04, TC-03-05 |
| AC-06 | AI가 적절한 분석 도구와 차트 유형 선택 | TC-04-01~03 |
| AC-07 | 시각화 대시보드 실시간 반영, 테이블 지원 | TC-05-01~02, TC-05-05 |
| AC-08 | 시각화 크기/위치/스타일 편집 가능 | TC-06-01~06 |
| AC-09 | 이미지(PNG 등)로 다운로드 가능 | TC-07-01, TC-07-04 |

---

## 6. 리스크 및 대응 전략

### 6.1 리스크 매트릭스

| 리스크 | 영향도 | 발생 가능성 | 대응 전략 |
|--------|--------|------------|----------|
| LLM 응답 비결정성 (같은 입력, 다른 출력) | 높음 | 높음 | 모든 LLM 호출을 pytest-mock/vitest mock으로 대체. fixture 응답 고정 |
| R2 스토리지 외부 의존성 | 중간 | 중간 | 단위/통합 테스트에서 localstack 또는 in-memory mock 사용 |
| 대용량 파일(50MB) 경계값 처리 | 중간 | 중간 | TC-01-07 경계값 테스트 전용 fixture 파일 생성 |
| SSE 스트리밍 테스트 어려움 | 중간 | 중간 | httpx AsyncClient + anyio를 사용한 SSE 이벤트 수집 테스트 헬퍼 구현 |
| E2E 테스트 불안정성 (Flaky) | 낮음 | 중간 | Playwright auto-wait 활용, 네트워크 인터셉트로 API mock |
| PostgreSQL 스키마 마이그레이션 불일치 | 중간 | 낮음 | CI에서 Alembic migrate 후 테스트 실행. 테스트 DB 격리(트랜잭션 롤백) |
| 성능 테스트 환경 차이 (로컬 vs 운영) | 낮음 | 높음 | 성능 수치는 참고 기준으로 운영. 운영 배포 후 재측정 |

### 6.2 테스트 실행 순서

```
1. 로컬: docker-compose up (PostgreSQL, Redis)
2. pytest tests/unit/ (백엔드 단위)
3. vitest run (프론트엔드 단위)
4. pytest tests/integration/ (백엔드 통합)
5. playwright test (E2E, 개발 서버 구동 필요)
6. locust / k6 (성능, 별도 실행)
7. OWASP ZAP / 수동 보안 점검 (배포 전)
```

### 6.3 테스트 데이터 관리

- 모든 fixture 파일은 `tests/fixtures/`에 버전 관리
- 테스트 DB는 트랜잭션 롤백으로 격리 (pytest fixture scope=function)
- 민감 데이터(실제 사용자 데이터) 테스트 사용 금지 — 합성 데이터만 사용
