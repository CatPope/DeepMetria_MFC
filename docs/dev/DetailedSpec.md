# DeepMetria DetailedSpec — LLM 기반 데이터 분석 MFC C++ 데스크톱 앱 상세 설계

## 목차
1. 클래스 메서드 인터페이스 설계 ................................ L17
2. 데이터 모델 설계 (SQLite) ..................................... L267
3. MFC 다이얼로그/뷰 클래스 설계 ................................ L433
4. AI 오케스트레이션 상세 ....................................... L625
5. 파일 업로드/처리 파이프라인 .................................. L774
6. 대시보드 레이아웃 시스템 ..................................... L844
7. 시각화 서식 편집 모델 ........................................ L899
8. 에러 핸들링 전략 ............................................. L955
9. 상태 관리 설계 ............................................... L1044


---

## 1. 클래스 메서드 인터페이스 설계

### 1.1 기본 규칙

- 모든 비즈니스 로직은 C++ 클래스 메서드로 구현
- 인증: Windows 로컬 라이선스 키 기반 (`LicenseManager` 클래스)
- 데이터 교환 형식: 내부 C++ 구조체 (`struct`) 및 `std::vector`
- 오류 반환 공통 형식:
  ```cpp
  struct AppError {
      CString code;       // 오류 코드 (예: "FILE_TOO_LARGE")
      CString message;    // 사용자 친화적 메시지
      int     severity;   // 0=info, 1=warning, 2=error
  };
  ```

### 1.2 인증/라이선스 (LicenseManager)

#### LicenseManager::Register()
- 설명: 신규 라이선스 키 등록 및 사용자 정보 저장
- 시그니처:
  ```cpp
  BOOL LicenseManager::Register(
      const CString& licenseKey,
      const CString& userName,
      AppError&       outError
  );
  ```
- 성공: `TRUE` 반환, 레지스트리에 라이선스 정보 저장
- 실패: `FALSE` 반환, `outError` 설정 (`INVALID_LICENSE`, `ALREADY_REGISTERED`)

#### LicenseManager::Activate()
- 설명: 로컬 라이선스 키 검증 및 세션 활성화
- 시그니처:
  ```cpp
  BOOL LicenseManager::Activate(
      const CString& licenseKey,
      AppError&       outError
  );
  ```
- 성공: `TRUE`, `m_sessionToken` 내부 설정
- 실패: `FALSE` (`INVALID_CREDENTIALS`, `LICENSE_EXPIRED`)

#### LicenseManager::RefreshSession()
- 설명: 세션 토큰 갱신 (만료 1시간 전 자동 호출)
- 시그니처:
  ```cpp
  BOOL LicenseManager::RefreshSession(AppError& outError);
  ```

#### LicenseManager::GetCurrentUser()
- 설명: 현재 활성 사용자 정보 조회
- 시그니처:
  ```cpp
  UserInfo LicenseManager::GetCurrentUser() const;
  // UserInfo: { CString id, name, tier("free|pro|max"), int usedCount, limitCount }
  ```

### 1.3 데이터소스 (DataSourceManager)

#### DataSourceManager::ImportFile()
- 설명: 로컬 파일 가져오기 및 DataSource 생성
- 시그니처:
  ```cpp
  BOOL DataSourceManager::ImportFile(
      const CString&  filePath,       // 전체 경로
      const CString&  name,           // 표시 이름 (빈 문자열 시 파일명 사용)
      CString&        outDatasourceId,
      AppError&       outError
  );
  ```
- 성공: `TRUE`, `outDatasourceId` 설정 (SQLite INTEGER ID를 CString으로 변환)
- 실패: `FALSE` (`UNSUPPORTED_FILE_TYPE`, `FILE_TOO_LARGE`, `PARSE_ERROR`)
- 제한: 최대 파일 크기 50MB, 백그라운드 스레드(`AfxBeginThread`)로 처리

#### DataSourceManager::GetList()
- 설명: 사용자 DataSource 목록 조회
- 시그니처:
  ```cpp
  std::vector<DataSourceInfo> DataSourceManager::GetList(
      int page = 1,
      int pageSize = 20
  );
  // DataSourceInfo: { CString id, name, fileType, status, int rowCount, CTime createdAt }
  ```

#### DataSourceManager::GetDetail()
- 설명: DataSource 상세 조회 (스키마, 요약 포함)
- 시그니처:
  ```cpp
  DataSourceDetail DataSourceManager::GetDetail(const CString& datasourceId);
  // DataSourceDetail: { DataSourceInfo info, vector<ColumnSchema> schema, DataSummary summary }
  ```

#### DataSourceManager::GetImportProgress()
- 설명: 가져오기 진행 상태 폴링 (MFC 타이머 기반)
- 시그니처:
  ```cpp
  ImportProgress DataSourceManager::GetImportProgress(const CString& datasourceId);
  // ImportProgress: { CString stage("importing|parsing|summarizing|done"), float progress, CString message }
  ```

#### DataSourceManager::Remove()
- 설명: DataSource 삭제 (연관 분석 결과, 시각화 포함)
- 시그니처:
  ```cpp
  BOOL DataSourceManager::Remove(const CString& datasourceId, AppError& outError);
  ```

### 1.4 분석 (AnalysisManager)

#### AnalysisManager::RequestAnalysis()
- 설명: 자연어 질문으로 분석 요청 (백그라운드 스레드)
- 시그니처:
  ```cpp
  BOOL AnalysisManager::RequestAnalysis(
      const CString& question,
      const CString& datasourceId,
      const CString& dashboardId,   // 빈 문자열 허용
      CString&       outFlowId,
      AppError&       outError
  );
  ```
- 성공: `TRUE`, 백그라운드 스레드 시작, `WM_ANALYSIS_PROGRESS` 메시지로 진행 통보
- 실패: `FALSE` (`QUOTA_EXCEEDED`, `DATASOURCE_NOT_FOUND`, `EMPTY_QUESTION`)

#### AnalysisManager::GetFlowStatus()
- 설명: 분석 플로우 상태 및 결과 조회
- 시그니처:
  ```cpp
  AnalysisFlowInfo AnalysisManager::GetFlowStatus(const CString& flowId);
  // AnalysisFlowInfo: { CString status, question, vector<CotStep> cotSteps, vector<CString> visualizationIds }
  ```

#### AnalysisManager::CancelFlow()
- 설명: 진행 중인 분석 취소
- 시그니처:
  ```cpp
  BOOL AnalysisManager::CancelFlow(const CString& flowId, AppError& outError);
  ```

### 1.5 대시보드 (DashboardManager)

#### DashboardManager::GetList()
- 시그니처:
  ```cpp
  std::vector<DashboardInfo> DashboardManager::GetList();
  // DashboardInfo: { CString id, name, datasourceId, CTime createdAt }
  ```

#### DashboardManager::Create()
- 시그니처:
  ```cpp
  BOOL DashboardManager::Create(
      const CString& name,
      const CString& datasourceId,
      CString&       outDashboardId,
      AppError&       outError
  );
  ```

#### DashboardManager::GetDetail()
- 시그니처:
  ```cpp
  DashboardDetail DashboardManager::GetDetail(const CString& dashboardId);
  // DashboardDetail: { DashboardInfo info, vector<LayoutItem> layout, vector<VisualizationInfo> visualizations }
  ```

#### DashboardManager::UpdateLayout()
- 설명: 레이아웃 저장 (드래그/리사이즈 완료 후 호출)
- 시그니처:
  ```cpp
  BOOL DashboardManager::UpdateLayout(
      const CString&              dashboardId,
      const std::vector<LayoutItem>& layout,
      AppError&                   outError
  );
  ```

#### DashboardManager::Remove()
- 시그니처:
  ```cpp
  BOOL DashboardManager::Remove(const CString& dashboardId, AppError& outError);
  ```

### 1.6 내부 분석 도구 디스패처 (AnalysisToolDispatcher)

MFC 앱 내부에서 AI Agent가 호출하는 C++ 분석 함수 디스패처.

| 도구 메서드 | 설명 | 입력 파라미터 | 반환 형식 |
|------------|------|-------------|----------|
| `DispatchBasicStats()` | 기본 통계 (평균, 중앙값, 표준편차, 결측치) | `datasourceId`, `columns` | `map<CString, StatsObj>` |
| `DispatchDistribution()` | 컬럼 분포 분석 (히스토그램 데이터) | `datasourceId`, `column`, `bins=10` | `DistributionResult` |
| `DispatchValueCounts()` | 카테고리 컬럼 빈도 집계 | `datasourceId`, `column`, `topN=10` | `vector<ValueCount>` |
| `DispatchGroupBy()` | 그룹별 집계 | `datasourceId`, `groupCol`, `aggCol`, `func` | `vector<GroupResult>` |
| `DispatchPivot()` | 피벗 테이블 생성 | `datasourceId`, `index`, `columns`, `values`, `aggfunc` | `PivotResult` |
| `DispatchMonthlyTrend()` | 월별 시계열 집계 | `datasourceId`, `dateCol`, `valueCol`, `func` | `vector<TrendPoint>` |
| `DispatchMovingAverage()` | 이동평균 계산 | `datasourceId`, `dateCol`, `valueCol`, `window=7` | `vector<MaPoint>` |
| `DispatchYoYComparison()` | 전년 동기 비교 | `datasourceId`, `dateCol`, `valueCol` | `vector<YoYPoint>` |
| `DispatchPearsonCorr()` | 피어슨 상관계수 행렬 | `datasourceId`, `columns` | `CorrMatrix` |
| `DispatchTopCorrelated()` | 특정 컬럼과 상관 높은 컬럼 Top N | `datasourceId`, `targetCol`, `topN=5` | `vector<CorrPair>` |
| `DispatchRecommendViz()` | 데이터 특성 기반 차트 유형 추천 | `datasourceId`, `question`, `columns` | `VizRecommendation` |
| `DispatchFilterRows()` | 조건 기반 행 필터링 후 통계 | `datasourceId`, `conditions`, `aggCols` | `FilterResult` |
| `DispatchTopNRows()` | 특정 컬럼 기준 상위 N행 | `datasourceId`, `sortCol`, `n=10`, `ascending=false` | `vector<RowData>` |

### 1.7 시각화 (VisualizationManager)

#### VisualizationManager::GetDetail()
- 시그니처:
  ```cpp
  VisualizationInfo VisualizationManager::GetDetail(const CString& vizId);
  // VisualizationInfo: {
  //   CString id, dashboardId, vizType, title
  //   ChartConfig chartConfig   // 차트 데이터 및 축 설정 (JSON 문자열)
  //   ChartStyle  style         // 색상, 폰트 등 스타일
  //   LayoutItem  position      // {x, y, w, h} 그리드 위치
  // }
  ```

#### VisualizationManager::UpdateStyle()
- 설명: 서식(위치, 크기, 스타일) 업데이트
- 시그니처:
  ```cpp
  BOOL VisualizationManager::UpdateStyle(
      const CString&  vizId,
      const LayoutItem& position,
      const ChartStyle& style,
      AppError&         outError
  );
  ```

#### VisualizationManager::Remove()
- 시그니처:
  ```cpp
  BOOL VisualizationManager::Remove(const CString& vizId, AppError& outError);
  ```

#### VisualizationManager::ExportToFile()
- 설명: 시각화를 이미지 파일로 내보내기
- 시그니처:
  ```cpp
  BOOL VisualizationManager::ExportToFile(
      const CString& vizId,
      const CString& outputPath,  // 로컬 파일 경로 (.png / .bmp)
      AppError&       outError
  );
  ```

---

## 2. 데이터 모델 설계 (SQLite)

### 2.1 설계 원칙

- SQLite 3 사용, C++ SQLite wrapper (`SQLiteDB` 클래스) 기반
- INTEGER PRIMARY KEY AUTOINCREMENT를 PK로 사용 (로컬 단일 사용자)
- `created_at`, `updated_at` 모든 테이블에 공통 포함 (UNIX timestamp INTEGER)
- 소프트 삭제(`deleted_at`)는 datasource, dashboard 테이블에만 적용
- 사용자 격리: 단일 사용자 앱이므로 `user_id` 생략, 라이선스 정보는 레지스트리 관리

### 2.2 테이블 정의

#### users (라이선스 정보)
```sql
CREATE TABLE IF NOT EXISTS users (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    license_key  TEXT UNIQUE NOT NULL,
    name         TEXT NOT NULL,
    tier         TEXT NOT NULL DEFAULT 'free'   -- free | pro | max
                 CHECK (tier IN ('free', 'pro', 'max')),
    created_at   INTEGER NOT NULL DEFAULT (strftime('%s','now')),
    updated_at   INTEGER NOT NULL DEFAULT (strftime('%s','now'))
);
```

#### usage_quotas
```sql
CREATE TABLE IF NOT EXISTS usage_quotas (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    period      TEXT NOT NULL,                   -- 'YYYY-MM' (월별 집계)
    used        INTEGER NOT NULL DEFAULT 0,
    limit_count INTEGER NOT NULL DEFAULT 10,     -- 티어별 한도
    created_at  INTEGER NOT NULL DEFAULT (strftime('%s','now')),
    updated_at  INTEGER NOT NULL DEFAULT (strftime('%s','now')),
    UNIQUE(period)
);
CREATE INDEX IF NOT EXISTS idx_usage_quotas_period ON usage_quotas(period);
```

#### datasources
```sql
CREATE TABLE IF NOT EXISTS datasources (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    name          TEXT NOT NULL,
    file_type     TEXT NOT NULL CHECK (file_type IN ('csv', 'excel', 'json')),
    local_path    TEXT NOT NULL,                 -- 로컬 파일시스템 경로
    file_size     INTEGER NOT NULL,
    row_count     INTEGER,
    column_count  INTEGER,
    status        TEXT NOT NULL DEFAULT 'processing'
                  CHECK (status IN ('processing', 'ready', 'error')),
    error_message TEXT,
    created_at    INTEGER NOT NULL DEFAULT (strftime('%s','now')),
    updated_at    INTEGER NOT NULL DEFAULT (strftime('%s','now')),
    deleted_at    INTEGER
);
CREATE INDEX IF NOT EXISTS idx_datasources_status ON datasources(status);
```

#### data_schemas
```sql
CREATE TABLE IF NOT EXISTS data_schemas (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    datasource_id  INTEGER NOT NULL REFERENCES datasources(id) ON DELETE CASCADE,
    column_name    TEXT NOT NULL,
    column_index   INTEGER NOT NULL,
    dtype          TEXT NOT NULL,               -- string | int | float | datetime | boolean
    nullable       INTEGER NOT NULL DEFAULT 1,  -- SQLite BOOLEAN (0/1)
    sample_values  TEXT,                        -- JSON 문자열 (최대 5개 샘플)
    created_at     INTEGER NOT NULL DEFAULT (strftime('%s','now'))
);
CREATE INDEX IF NOT EXISTS idx_data_schemas_datasource ON data_schemas(datasource_id);
```

#### data_summaries
```sql
CREATE TABLE IF NOT EXISTS data_summaries (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    datasource_id  INTEGER NOT NULL UNIQUE REFERENCES datasources(id) ON DELETE CASCADE,
    domain         TEXT,                         -- AI 추정 도메인 (예: "영업 데이터")
    description    TEXT,                         -- AI 생성 요약 텍스트
    statistics     TEXT NOT NULL DEFAULT '{}',  -- JSON 문자열: 컬럼별 통계
    row_count      INTEGER NOT NULL DEFAULT 0,
    created_at     INTEGER NOT NULL DEFAULT (strftime('%s','now')),
    updated_at     INTEGER NOT NULL DEFAULT (strftime('%s','now'))
);
```

#### dashboards
```sql
CREATE TABLE IF NOT EXISTS dashboards (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    datasource_id INTEGER NOT NULL REFERENCES datasources(id),
    name          TEXT NOT NULL,
    layout        TEXT NOT NULL DEFAULT '[]',   -- JSON 문자열: LayoutItem[]
    created_at    INTEGER NOT NULL DEFAULT (strftime('%s','now')),
    updated_at    INTEGER NOT NULL DEFAULT (strftime('%s','now')),
    deleted_at    INTEGER
);
CREATE INDEX IF NOT EXISTS idx_dashboards_datasource ON dashboards(datasource_id);
```

#### visualizations
```sql
CREATE TABLE IF NOT EXISTS visualizations (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    dashboard_id INTEGER NOT NULL REFERENCES dashboards(id) ON DELETE CASCADE,
    flow_id      INTEGER,                        -- 생성한 AnalysisFlow (NULL 허용)
    viz_type     TEXT NOT NULL                   -- line | bar | pie | scatter | table | area | histogram | donut | hbar
                 CHECK (viz_type IN ('line','bar','pie','scatter','table','area','histogram','donut','hbar')),
    title        TEXT,
    chart_config TEXT NOT NULL DEFAULT '{}',    -- JSON 문자열: 차트 데이터 및 축 설정
    style        TEXT NOT NULL DEFAULT '{}',    -- JSON 문자열: 색상, 폰트 등 스타일
    position     TEXT NOT NULL DEFAULT '{}',    -- JSON 문자열: {x, y, w, h}
    created_at   INTEGER NOT NULL DEFAULT (strftime('%s','now')),
    updated_at   INTEGER NOT NULL DEFAULT (strftime('%s','now'))
);
CREATE INDEX IF NOT EXISTS idx_visualizations_dashboard ON visualizations(dashboard_id);
```

#### analysis_flows
```sql
CREATE TABLE IF NOT EXISTS analysis_flows (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    datasource_id  INTEGER NOT NULL REFERENCES datasources(id),
    dashboard_id   INTEGER REFERENCES dashboards(id),
    question       TEXT NOT NULL,
    status         TEXT NOT NULL DEFAULT 'pending'
                   CHECK (status IN ('pending', 'running', 'completed', 'failed')),
    llm_provider   TEXT,                         -- 사용된 LLM 제공자
    llm_model      TEXT,
    cot_steps      TEXT NOT NULL DEFAULT '[]',  -- JSON 문자열: CoT 추론 단계 로그
    tool_calls     TEXT NOT NULL DEFAULT '[]',  -- JSON 문자열: 내부 분석 도구 호출 기록
    error_message  TEXT,
    created_at     INTEGER NOT NULL DEFAULT (strftime('%s','now')),
    updated_at     INTEGER NOT NULL DEFAULT (strftime('%s','now'))
);
CREATE INDEX IF NOT EXISTS idx_analysis_flows_datasource ON analysis_flows(datasource_id);
CREATE INDEX IF NOT EXISTS idx_analysis_flows_status ON analysis_flows(status);
```

### 2.3 관계 다이어그램 (요약)

```
users (라이선스 정보) ── usage_quotas (월별 쿼터)
datasources (1) ──< data_schemas (N)
datasources (1) ── data_summaries (1)
datasources (1) ──< dashboards (N)
dashboards (1) ──< visualizations (N)
datasources (1) ──< analysis_flows (N)
analysis_flows (1) ──< visualizations (N)
```

### 2.4 인메모리 캐시 규칙 (std::map 기반)

| 캐시 키 패턴 | 값 타입 | TTL | 용도 |
|-------------|---------|-----|------|
| `quota:{YYYY-MM}` | `QuotaInfo` | 1시간 | 사용량 빠른 조회 |
| `schema:{datasource_id}` | `vector<ColumnSchema>` | 앱 세션 내 영구 | 스키마 반복 조회 캐시 |
| `analysis:lock:{datasource_id}` | `bool` | 5분 | 동시 분석 요청 제한 |

- 캐시는 `CacheManager` 싱글턴에서 `std::map<CString, CacheEntry>` 구조로 관리
- TTL 만료 체크: `CacheManager::Cleanup()` — WM_TIMER 60초 간격 호출

---

## 3. MFC 다이얼로그/뷰 클래스 설계

### 3.1 애플리케이션 구조 (MFC SDI)

```
CDeepMetriaApp (CWinApp)
├── CMainFrame (CFrameWnd)
│   ├── CMenuBar                    — 상단 메뉴 (파일/분석/대시보드/도움말)
│   ├── CToolBar                    — 도구 모음 (가져오기, 분석, 내보내기)
│   └── CSplitterWnd (좌/우 분할)
│       ├── [좌] CNavigatorView     — 데이터소스/대시보드 탐색 트리뷰
│       └── [우] CTabView           — 탭 기반 콘텐츠 영역
│           ├── CDashboardView      — 개별 대시보드 뷰
│           └── CDataSourceView     — 데이터소스 상세 뷰
└── 다이얼로그
    ├── CActivationDlg              — 라이선스 키 등록/활성화
    ├── CImportFileDlg              — 파일 가져오기 (진행 상태 포함)
    ├── CQueryInputDlg              — 자연어 질문 입력
    ├── CStyleEditorDlg             — 시각화 서식 편집
    └── CAboutDlg                   — 앱 정보
```

### 3.2 뷰 클래스 트리

#### CDashboardView (CView 파생)

```
CDashboardView
├── CDashboardHeader (CStatic 영역)
│   ├── CEdit m_titleEdit           — 대시보드 제목 인라인 편집
│   └── CButton m_exportBtn         — 내보내기 버튼
├── CQueryPanel (CDialogBar)
│   ├── CEdit m_questionEdit        — 자연어 질문 입력 (멀티라인)
│   ├── CButton m_submitBtn         — 분석 요청 버튼
│   └── CCotProgressList (CListCtrl) — CoT 추론 단계 실시간 표시
│       └── CCotStepItem[]          — 단계별 추론 텍스트 (텍스트 애니메이션)
├── CDashboardCanvas (CScrollView 파생)  — 가상 캔버스 (드래그/리사이즈 지원)
│   └── CVisualizationCard[]        — 개별 시각화 카드 (CWnd 파생)
│       ├── CCardHeader             — 제목 표시줄 (드래그 핸들)
│       ├── CChartRenderer          — viz_type에 따라 GDI+ 차트 렌더링
│       │   ├── CLineChartView
│       │   ├── CBarChartView
│       │   ├── CPieChartView
│       │   ├── CScatterChartView
│       │   ├── CAreaChartView
│       │   ├── CHistogramView
│       │   └── CDataGridView       — 표 시각화 (CListCtrl 확장)
│       └── CCardFooter             — 내보내기 버튼
└── CStyleEditorPanel (CDialogBar)  — 우측 도킹 패널 (선택된 카드 편집)
    ├── CEdit m_titleEdit
    ├── CComboBox m_colorSchemeCombo
    ├── CSpinButtonCtrl m_fontSizeSpin
    └── CEdit m_widthEdit, m_heightEdit
```

#### CImportFileDlg (CDialog 파생)

```
CImportFileDlg
├── CEdit m_filePathEdit            — 선택된 파일 경로 표시
├── CButton m_browseBtn             — 파일 선택 (CFileDialog 호출)
├── CProgressCtrl m_progressBar     — 가져오기 진행 상태
├── CStatic m_statusMsg             — 단계별 메시지 표시
└── CListCtrl m_schemaPreview       — 요약 완료 후 스키마 미리보기
```

#### CActivationDlg (CDialog 파생)

```
CActivationDlg
├── CEdit m_licenseKeyEdit          — 라이선스 키 입력
├── CEdit m_userNameEdit            — 사용자 이름 입력
├── CButton m_activateBtn           — 활성화 버튼
└── CStatic m_statusMsg             — 활성화 결과 메시지
```

### 3.3 클래스 주요 멤버 및 메시지 맵

#### CVisualizationCard

```cpp
class CVisualizationCard : public CWnd {
public:
    VisualizationInfo  m_vizInfo;    // 현재 시각화 데이터
    BOOL               m_bSelected; // 선택 상태
    CPoint             m_ptDragStart;
    CSize              m_szResize;

    // 메시지 핸들러
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnPaint();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

    // 메서드
    void SetVisualization(const VisualizationInfo& info);
    void RenderChart(CDC* pDC);
    void ExportToClipboard();
};
```

#### CDashboardCanvas

```cpp
class CDashboardCanvas : public CScrollView {
public:
    std::vector<CVisualizationCard*> m_cards;
    DashboardDetail                  m_detail;
    CString                          m_selectedCardId;

    // 메시지 핸들러
    afx_msg LRESULT OnAnalysisProgress(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnVisualizationReady(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnImportProgress(WPARAM wParam, LPARAM lParam);

    // 메서드
    void LoadDashboard(const CString& dashboardId);
    void AddCard(const VisualizationInfo& vizInfo);
    void RemoveCard(const CString& vizId);
    void SaveLayout();              // debounce 500ms 후 DashboardManager::UpdateLayout() 호출
    void SelectCard(const CString& vizId);
};
```

#### CQueryInputDlg

```cpp
class CQueryInputDlg : public CDialog {
public:
    CString  m_datasourceId;
    CString  m_dashboardId;
    BOOL     m_bLoading;
    BOOL     m_bDisabled;          // 쿼터 초과 시 TRUE

    // 메서드
    void OnBnClickedSubmit();
    void SetLoading(BOOL bLoading);
    void SetQuotaExceeded(BOOL bExceeded);
    void AppendCotStep(const CotStep& step);
};
```

### 3.4 공통 C++ 구조체 정의

```cpp
// DeepMetriaTypes.h

enum class VizType {
    Line, Bar, HBar, Pie, Donut, Scatter, Area, Histogram, Table
};

struct ChartConfig {
    CString              data;      // JSON 문자열 (배열)
    CString              xKey;
    CString              yKey;      // 쉼표 구분 다중 키 허용
    CString              labelKey;
    CString              valueKey;
};

struct ChartStyle {
    CString colorScheme;            // "default" | "blue" | "green" | "warm" | "mono"
    int     fontSize = 14;
    BOOL    showLegend = TRUE;
    BOOL    showGrid = TRUE;
};

struct LayoutItem {
    CString id;                     // visualization id
    int x = 0, y = 0;
    int w = 6, h = 4;
    int minW = 3, minH = 2;
};

struct VisualizationInfo {
    CString     id;
    CString     dashboardId;
    VizType     vizType;
    CString     title;
    ChartConfig chartConfig;
    ChartStyle  style;
    LayoutItem  position;
};

struct GridItem {
    CString id;                     // visualization.id
    int x, y, w, h;
};
```

---

## 4. AI 오케스트레이션 상세

### 4.1 내부 분석 도구 목록

AI Agent가 내부적으로 사용하는 C++ 기반 분석 함수 전체 목록.
이 함수들은 MFC UI와 무관하게 `AnalysisToolDispatcher`가 직접 호출하는 내부 메서드입니다.

| 도구 이름 | 설명 | 입력 파라미터 | 반환 형식 |
|-----------|------|-------------|----------|
| `statistics.basic_stats` | 기본 통계 (평균, 중앙값, 표준편차, 결측치) | `datasourceId`, `columns[]` | `map<CString, StatsObj>` |
| `statistics.distribution` | 컬럼 분포 분석 (히스토그램 데이터) | `datasourceId`, `column`, `bins=10` | `DistributionResult` |
| `statistics.value_counts` | 카테고리 컬럼 빈도 집계 | `datasourceId`, `column`, `topN=10` | `vector<ValueCount>` |
| `aggregation.group_by` | 그룹별 집계 (합계, 평균 등) | `datasourceId`, `groupCol`, `aggCol`, `func=sum` | `vector<GroupResult>` |
| `aggregation.pivot` | 피벗 테이블 생성 | `datasourceId`, `index`, `columns`, `values`, `aggfunc` | `PivotResult` |
| `timeseries.monthly_trend` | 월별 시계열 집계 | `datasourceId`, `dateCol`, `valueCol`, `func=sum` | `vector<TrendPoint>` |
| `timeseries.moving_average` | 이동평균 계산 | `datasourceId`, `dateCol`, `valueCol`, `window=7` | `vector<MaPoint>` |
| `timeseries.yoy_comparison` | 전년 동기 비교 | `datasourceId`, `dateCol`, `valueCol` | `vector<YoYPoint>` |
| `correlation.pearson` | 피어슨 상관계수 행렬 | `datasourceId`, `columns[]` | `CorrMatrix` |
| `correlation.top_correlated` | 특정 컬럼과 상관 높은 컬럼 Top N | `datasourceId`, `targetCol`, `topN=5` | `vector<CorrPair>` |
| `visualization.recommend` | 데이터 특성 기반 차트 유형 추천 | `datasourceId`, `question`, `columns[]` | `VizRecommendation` |
| `data.filter_rows` | 조건 기반 행 필터링 후 통계 | `datasourceId`, `conditions[]`, `aggCols[]` | `FilterResult` |
| `data.top_n_rows` | 특정 컬럼 기준 상위 N행 | `datasourceId`, `sortCol`, `n=10`, `ascending=false` | `vector<RowData>` |

### 4.2 Chain of Thinking (CoT) 흐름

```
입력: 사용자 자연어 질문 + DataSource 스키마 + 도구 목록

Step 1 — 질문 분석 (Column Identification)
  <thinking>
  질문에서 언급된 측정 지표와 차원을 식별한다.
  - "월별 매출 추이" → date 컬럼(차원), sales 컬럼(지표) 필요
  - 스키마에서 매핑: dateCol="order_date", valueCol="revenue"
  </thinking>

Step 2 — 도구 선택 (Tool Selection)
  <thinking>
  분석 목적에 맞는 도구를 선택한다.
  - "추이" → timeseries 계열 도구
  - 월별 집계 → timeseries.monthly_trend
  선택: DispatchMonthlyTrend(datasourceId, "order_date", "revenue")
  </thinking>

Step 3 — 차트 유형 결정 (Chart Selection)
  <thinking>
  시간 + 수치 조합 → line 차트 또는 area 차트
  "추이"는 라인 차트가 적합
  선택: vizType = VizType::Line
  </thinking>

Step 4 — 도구 호출 (Internal Tool Call)
  → AnalysisToolDispatcher가 C++ 분석 함수를 직접 호출
  → CSV/SQLite 데이터 기반 계산 수행
  → 결과 구조체 수신
  → WM_COT_STEP 메시지로 UI에 단계별 결과 통보

Step 5 — 결과 조합 (Result Assembly)
  → ChartConfig 구성 (data JSON, xKey, yKey)
  → VisualizationInfo 구조체 생성
  → SQLite visualizations 테이블 저장
  → WM_VISUALIZATION_READY 메시지로 CDashboardCanvas에 통보
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

### 4.4 LLM 클라이언트 설계 (CLlmClient)

```cpp
// LlmClient.h

class CLlmClient {
public:
    enum Provider { Claude, Gpt4o, Gemini };

    struct LLMResponse {
        CString           content;
        vector<ToolCall>  toolCalls;  // AI가 선택한 내부 도구 호출 목록
    };

    // 동기 호출 (백그라운드 스레드에서 실행)
    BOOL ChatWithTools(
        const vector<ChatMessage>& messages,
        const vector<ToolSchema>&  tools,
        Provider                   provider,
        LLMResponse&               outResponse,
        AppError&                  outError
    );

    // 스트리밍 호출: CoT 텍스트를 청크 단위로 콜백
    // pfnCallback: void(const CString& chunk, LPVOID pContext)
    BOOL StreamChat(
        const vector<ChatMessage>& messages,
        Provider                   provider,
        PFNSTREAMCALLBACK          pfnCallback,
        LPVOID                     pContext,
        AppError&                  outError
    );

private:
    static const CString ENDPOINTS[];  // LLM API 엔드포인트
    CString              m_apiKey;     // 라이선스 서버에서 발급
    WinHTTP 기반 HTTP 클라이언트;
};
```

### 4.5 데이터 요약 (Summarizer) 흐름

```
파일 가져오기 완료 (로컬 파일시스템)
    ↓
1. C++ 파서로 파일 파싱 → 인메모리 데이터 테이블
2. data_schemas 테이블에 컬럼 메타데이터 저장
3. DispatchBasicStats() 호출 → 기본 통계 수집
4. CLlmClient::ChatWithTools() — 스키마 + 샘플 데이터 전달 → 도메인 추정 + 설명 텍스트 생성
5. data_summaries 테이블 저장
6. 기본 대시보드 생성 (요약 카드 2-3개 자동 배치)
   - 전체 행/컬럼 수 카드 (CDataGridView)
   - 주요 수치 컬럼 기본 통계 테이블 (CDataGridView)
   - 도메인 설명 텍스트 카드 (CVisualizationCard + CStatic)
7. WM_IMPORT_COMPLETE 메시지 → CDashboardCanvas에 통보
```

---

## 5. 파일 업로드/처리 파이프라인

### 5.1 파일 가져오기 처리 흐름

```
사용자 → CImportFileDlg → CFileDialog 파일 선택
    ↓
[메인 스레드] DataSourceManager::ImportFile() 호출
    ↓ 검증 (동기)
- 파일 확장자 확인: .csv | .xlsx | .xls | .json
- 파일 크기 확인 (50MB 초과 → AppError)
    ↓
[AfxBeginThread] 백그라운드 처리 시작
    ↓
1. 로컬 앱 데이터 디렉토리에 파일 복사
   - 경로: %APPDATA%\DeepMetria\datasources\{datasource_id}\{filename}
   - 원본 파일은 사용자 선택 위치 유지
    ↓
2. DataSource 레코드 생성 (status='processing')
    ↓
3. WM_IMPORT_PROGRESS 메시지 → CImportFileDlg에 진행 상태 통보 (stage="importing")
    ↓
[Background Thread]
4. 파일 파싱 (C++ 파서)
   - CSV: libcsv 또는 자체 구현 CSVParser — UTF-8, EUC-KR 자동 감지
   - Excel: libxlsxwriter / xlsLib 기반 파서
   - JSON: RapidJSON 기반 파서
    ↓
5. 스키마 추출 → data_schemas 저장
6. AI Summarizer 실행 (4.5 참조)
7. DataSource status='ready' 업데이트
8. WM_IMPORT_COMPLETE 메시지 → 메인 윈도우에 통보
```

### 5.2 지원 파일 형식 및 제약

| 형식 | 확장자 | 최대 크기 | 최대 행 수 | 인코딩 |
|------|--------|---------|----------|------|
| CSV | .csv | 50MB | 100만 행 | UTF-8, EUC-KR 자동 감지 |
| Excel | .xlsx, .xls | 50MB | 100만 행 | N/A (라이브러리 처리) |
| JSON | .json | 50MB | 100만 행 | UTF-8 |

- 멀티시트 Excel: 첫 번째 시트만 처리 (MVP)
- 헤더 없는 CSV: 자동으로 col_0, col_1... 컬럼명 부여

### 5.3 로컬 파일시스템 저장 구조

```
%APPDATA%\DeepMetria\
  └── datasources\
      └── {datasource_id}\
          └── {original_filename}   — 복사된 원본 파일 (영구 보관)
  └── exports\
      └── {viz_id}_{timestamp}.png  — 내보낸 시각화 이미지
  └── deepmetria.db                 — SQLite 데이터베이스
```

- 파일 삭제: DataSource 삭제 시 해당 디렉토리도 동기 삭제 (`SHFileOperation`)

### 5.4 파싱 오류 처리

| 오류 유형 | 처리 방식 | 사용자 메시지 |
|---------|---------|------------|
| 인코딩 오류 | 다른 인코딩으로 재시도, 실패 시 에러 | "파일 인코딩을 인식할 수 없습니다" |
| 빈 파일 | 즉시 오류 반환 | "파일에 데이터가 없습니다" |
| 헤더만 있는 파일 | 스키마는 생성, 통계 생략 | "데이터 행이 없습니다 (헤더만 존재)" |
| 파싱 중 예외 | status='error', 메시지 SQLite 저장 | "파일을 처리하는 중 오류가 발생했습니다" |

---

## 6. 대시보드 레이아웃 시스템

### 6.1 CDashboardCanvas 레이아웃 설정

```cpp
// DashboardCanvas.h

const int GRID_COLS    = 12;   // 12열 그리드
const int ROW_HEIGHT   = 80;   // 1단위 = 80px
const int DEFAULT_VIZ_W = 6;   // 기본 너비 (12열 중 6 = 50%)
const int DEFAULT_VIZ_H = 4;   // 기본 높이 (4 * 80 = 320px)
const int MIN_VIZ_W    = 3;    // 최소 너비
const int MIN_VIZ_H    = 2;    // 최소 높이
const int CARD_MARGIN  = 12;   // 카드 간격 (px)

// 레이아웃 계산
CRect CDashboardCanvas::GridToPixel(const LayoutItem& item) {
    int cellW = (m_clientWidth - CARD_MARGIN) / GRID_COLS;
    int x = item.x * cellW + CARD_MARGIN;
    int y = item.y * ROW_HEIGHT + CARD_MARGIN;
    int w = item.w * cellW - CARD_MARGIN;
    int h = item.h * ROW_HEIGHT - CARD_MARGIN;
    return CRect(x, y, x + w, y + h);
}
```

### 6.2 레이아웃 저장 전략

- **즉시 반영**: 드래그/리사이즈 완료 시 즉시 로컬 `m_layout` 벡터 업데이트
- **Debounce 저장**: 500ms 타이머(`SetTimer`) 후 `DashboardManager::UpdateLayout()` 호출 (빈번한 DB 쓰기 방지)
- **충돌 방지**: 저장 중 추가 변경 발생 시 마지막 상태만 저장 (타이머 리셋)

### 6.3 기본 레이아웃 (데이터 요약 후 자동 생성)

```
[데이터 요약 완료 시 자동 배치 예시]

Row 0-1:  [요약 텍스트 카드 (w=12, h=2)]
Row 2-5:  [기본 통계 테이블 (w=6, h=4)] | [컬럼 스키마 테이블 (w=6, h=4)]
```

- 자동 배치 좌표는 `DispatchRecommendViz()` 결과와 무관하게 고정 구성

### 6.4 레이아웃 데이터 저장 형식 (dashboards.layout TEXT/JSON)

```json
[
  { "i": "1", "x": 0, "y": 0, "w": 12, "h": 2, "minW": 3, "minH": 2 },
  { "i": "2", "x": 0, "y": 2, "w": 6,  "h": 4, "minW": 3, "minH": 2 },
  { "i": "3", "x": 6, "y": 2, "w": 6,  "h": 4, "minW": 3, "minH": 2 }
]
```

---

## 7. 시각화 서식 편집 모델

### 7.1 편집 가능 속성

| 속성 그룹 | 속성 | 타입 | 기본값 | 설명 |
|---------|------|------|-------|------|
| 제목 | `title` | CString | AI 생성 제목 | 시각화 카드 헤더 제목 |
| 색상 | `colorScheme` | enum | `default` | default / blue / green / warm / mono |
| 폰트 | `fontSize` | int | 14 | 레이블/범례 폰트 크기 (pt) |
| 범례 | `showLegend` | BOOL | TRUE | 범례 표시 여부 |
| 그리드 | `showGrid` | BOOL | TRUE | 차트 배경 그리드 여부 |
| 크기/위치 | `position.w`, `position.h` | int | 6, 4 | 그리드 단위 (CDashboardCanvas 기준) |

### 7.2 색상 스킴 정의

```cpp
// ChartRenderer.cpp

static const COLORREF COLOR_SCHEMES[][5] = {
    // default
    { RGB(99,102,241), RGB(34,211,238), RGB(249,115,22), RGB(163,230,53), RGB(244,63,94) },
    // blue
    { RGB(59,130,246), RGB(96,165,250), RGB(147,197,253), RGB(191,219,254), RGB(219,234,254) },
    // green
    { RGB(34,197,94),  RGB(74,222,128), RGB(134,239,172), RGB(187,247,208), RGB(220,252,231) },
    // warm
    { RGB(249,115,22), RGB(251,146,60), RGB(251,191,36),  RGB(253,224,71),  RGB(254,240,138) },
    // mono
    { RGB(30,41,59),   RGB(71,85,105),  RGB(148,163,184), RGB(203,213,225), RGB(226,232,240) },
};
```

### 7.3 CStyleEditorPanel 동작 방식

1. 사용자가 CVisualizationCard를 클릭 → `CDashboardCanvas::SelectCard()` 호출
2. CStyleEditorPanel (도킹 패널) 활성화, 선택된 카드 정보 로드
3. 편집 시 즉시 `m_vizInfo.style` 업데이트 → `CVisualizationCard::Invalidate()` 호출 → 실시간 리렌더
4. 패널 닫기 또는 3초 idle 타이머 만료 시 `VisualizationManager::UpdateStyle()` 호출 (SQLite 저장)
5. 저장 실패 시 이전 상태로 롤백 + `AfxMessageBox()` 오류 알림

### 7.4 GDI+ 차트 렌더링 매핑

| vizType | 렌더러 클래스 | 특이사항 |
|---------|-------------|---------|
| `Line` | `CLineChartView` | 다중 Y계열 지원 |
| `Bar` | `CBarChartView` | stacked 옵션, 수직 바 |
| `HBar` | `CBarChartView` (수평 모드) | 수평 바 |
| `Area` | `CAreaChartView` | fillOpacity 0.3 (AlphaBlend) |
| `Pie` | `CPieChartView` | GDI+ Pie 섹터 |
| `Donut` | `CPieChartView` (innerRadius 설정) | 내부 원 마스크 |
| `Scatter` | `CScatterChartView` | 점 크기 옵션 |
| `Histogram` | `CBarChartView` (bin 전처리 포함) | 커스텀 데이터 전처리 |
| `Table` | `CDataGridView` | CListCtrl 확장, 정렬/페이지네이션 |

---

## 8. 에러 핸들링 전략

### 8.1 에러 분류

| 레이어 | 에러 유형 | 처리 방식 |
|------|---------|---------|
| UI (MFC) | 잘못된 입력 | 인라인 메시지 (`CStatic` 색상 변경) |
| UI (MFC) | 분석 진행 중 취소 | `AnalysisManager::CancelFlow()` + 스레드 종료 이벤트 |
| 비즈니스 로직 | 검증 오류 | `AppError` 반환 → `AfxMessageBox()` 또는 인라인 표시 |
| 비즈니스 로직 | 인증 오류 | `CActivationDlg` 재표시 |
| 비즈니스 로직 | 쿼터 초과 | 업그레이드 안내 `CDialog` 표시 |
| 비즈니스 로직 | 내부 처리 오류 | `AppError` 반환 + SQLite status='error' 저장 |
| LLM API | Timeout / Rate Limit | `AnalysisFlow` status='failed' + WM_ANALYSIS_ERROR 메시지 |
| 파일 처리 | 파싱 오류 | `DataSource` status='error' + 상세 메시지 SQLite 저장 |
| 내부 분석 도구 | 함수 실행 오류 | CoT 재시도 1회 → 실패 시 flow 실패 처리 |

### 8.2 공통 예외 처리 클래스

```cpp
// AppException.h

struct AppError {
    CString code;
    CString message;
    int     severity;  // 0=info, 1=warning, 2=error
};

// 공통 오류 코드 상수
namespace ErrorCode {
    constexpr LPCSTR VALIDATION_ERROR       = "VALIDATION_ERROR";
    constexpr LPCSTR NOT_FOUND              = "NOT_FOUND";
    constexpr LPCSTR UNAUTHORIZED           = "UNAUTHORIZED";
    constexpr LPCSTR QUOTA_EXCEEDED         = "QUOTA_EXCEEDED";
    constexpr LPCSTR LLM_PROVIDER_ERROR     = "LLM_PROVIDER_ERROR";
    constexpr LPCSTR FILE_PROCESSING_ERROR  = "FILE_PROCESSING_ERROR";
    constexpr LPCSTR INVALID_LICENSE        = "INVALID_LICENSE";
    constexpr LPCSTR LICENSE_EXPIRED        = "LICENSE_EXPIRED";
}

// 공통 에러 핸들러 헬퍼
void ShowErrorDialog(CWnd* pParent, const AppError& error);
void LogError(const AppError& error, const CString& context);
```

### 8.3 MFC 메시지 기반 비동기 오류 처리

- 백그라운드 스레드에서 오류 발생 시 `PostMessage(WM_ASYNC_ERROR, ...)` 사용 (직접 UI 조작 금지)
- 메인 스레드 핸들러에서 `AppError` 디코딩 후 UI 업데이트

```cpp
// 커스텀 메시지 정의
#define WM_IMPORT_PROGRESS   (WM_USER + 100)  // WPARAM: stage, LPARAM: progress*100
#define WM_IMPORT_COMPLETE   (WM_USER + 101)  // WPARAM: datasource_id
#define WM_ANALYSIS_PROGRESS (WM_USER + 102)  // WPARAM: step_index, LPARAM: CotStep*
#define WM_VISUALIZATION_READY (WM_USER + 103) // WPARAM: viz_id
#define WM_ANALYSIS_COMPLETE (WM_USER + 104)  // WPARAM: flow_id
#define WM_ANALYSIS_ERROR    (WM_USER + 105)  // WPARAM: AppError*
#define WM_ASYNC_ERROR       (WM_USER + 106)  // WPARAM: AppError*
#define WM_COT_STEP          (WM_USER + 107)  // WPARAM: CotStep*
```

### 8.4 LLM API 장애 대응

```
LLM API 호출 실패 시:
1. 설정된 provider로 1차 시도
2. Timeout(30초) 또는 Rate Limit → 다른 provider로 폴백 (설정된 경우)
3. 모든 provider 실패 → LLM_PROVIDER_ERROR 반환
4. AnalysisFlow status='failed' SQLite 업데이트
5. WM_ANALYSIS_ERROR 메시지 → 메인 윈도우에 통보
6. 사용자에게 "AI 서비스 일시 불안정" AfxMessageBox 표시 + 재시도 버튼
```

### 8.5 카드 단위 오류 격리

```cpp
// CVisualizationCard::OnPaint() 내부
// 차트 렌더링 실패 시 해당 카드에만 오류 메시지 표시
// 다른 카드 렌더링에 영향 없음
TRY {
    RenderChart(pDC);
} CATCH(CException, e) {
    pDC->DrawText(_T("차트 렌더링 오류"), rectClient, DT_CENTER | DT_VCENTER);
}
END_CATCH
```

---

## 9. 상태 관리 설계

### 9.1 싱글턴 매니저 클래스 구조

```cpp
// 앱 전역 상태는 각 싱글턴 매니저가 보유

// DashboardState (CDashboardManager 내부)
struct DashboardState {
    DashboardDetail            currentDashboard;
    map<CString, VisualizationInfo> visualizations;
    vector<LayoutItem>         layout;
    CString                    selectedVizId;

    // 메서드 (PostMessage로 CDashboardCanvas에 변경 통보)
    void SetDashboard(const DashboardDetail& detail);
    void AddVisualization(const VisualizationInfo& viz);
    void UpdateVisualization(const CString& id, const VisualizationInfo& patch);
    void RemoveVisualization(const CString& id);
    void UpdateLayout(const vector<LayoutItem>& layout);
    void SelectVisualization(const CString& id);
};

// AnalysisState (CAnalysisManager 내부)
struct AnalysisState {
    CString            activeFlowId;
    CString            flowStatus;  // "idle|pending|running|completed|failed"
    vector<CotStep>    cotSteps;
    BOOL               bStreaming;

    void StartFlow(const CString& flowId);
    void AppendCotStep(const CotStep& step);
    void CompleteFlow();
    void FailFlow(const CString& error);
    void Reset();
};

// UserState (CLicenseManager 내부)
struct UserState {
    UserInfo user;
    BOOL     bActivated;
    QuotaInfo quota;  // { int used, limit }

    void SetUser(const UserInfo& info);
    void ClearUser();
    void UpdateQuota(const QuotaInfo& quota);
};
```

### 9.2 MFC 메시지 기반 비동기 통신

```cpp
// BackgroundThread → MainWindow → CDashboardCanvas 전달 패턴

// 백그라운드 스레드 (분석 실행 중)
UINT AnalysisThreadProc(LPVOID pParam) {
    AnalysisThreadContext* ctx = (AnalysisThreadContext*)pParam;

    // CoT 단계 완료 시
    CotStep* pStep = new CotStep(...);
    ctx->pTargetWnd->PostMessage(WM_COT_STEP, 0, (LPARAM)pStep);
    // 수신 측에서 delete pStep 처리

    // 시각화 준비 완료 시
    ctx->pTargetWnd->PostMessage(WM_VISUALIZATION_READY, vizId, 0);

    // 분석 완료 시
    ctx->pTargetWnd->PostMessage(WM_ANALYSIS_COMPLETE, flowId, 0);
    return 0;
}
```

### 9.3 상태-UI 동기화 규칙

| 상태 | 저장 위치 | 동기화 방식 |
|------|---------|-----------|
| 대시보드 레이아웃 | `DashboardState` + SQLite | debounce 500ms 후 `DashboardManager::UpdateLayout()` |
| 시각화 스타일 | `DashboardState` + SQLite | 패널 닫기 또는 3초 idle 후 `VisualizationManager::UpdateStyle()` |
| CoT 추론 단계 | `AnalysisState` only | WM_COT_STEP 수신 시 append, 뷰 전환 시 초기화 |
| 사용자 정보/쿼터 | `UserState` + SQLite | 활성화 시 로드, 분석 완료 시 갱신 |
| 가져오기 진행 상태 | `CImportFileDlg` 로컬 | WM_IMPORT_PROGRESS 수신 시 업데이트, 완료 시 초기화 |

### 9.4 쿼터 관리 흐름

```
분석 요청 전:
1. UserState.quota 확인 → used >= limit 이면 CQueryInputDlg 비활성화 + 업그레이드 안내

분석 완료 후:
1. usage_quotas 테이블에서 최신 quota 로드
2. UserState::UpdateQuota(latest)
3. WM_USER_QUOTA_UPDATED → CQueryInputDlg 상태 갱신

앱 측:
1. AnalysisManager::RequestAnalysis() 진입 시 SQLite quota 확인
2. 초과 시 AppError(QUOTA_EXCEEDED) 반환
3. 완료 시 SQLite usage_quotas 업데이트 + UserState 갱신
```
