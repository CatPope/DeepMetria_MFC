# DeepMetria 상세설계서

> **프로젝트명**: DeepMetria — LLM 기반 데이터 분석 MFC 데스크톱 애플리케이션  
> **작성일**: 2026-05-12  
> **버전**: v1.0 (초안)

---

## 목차

1. [시스템 구조 설계](#1-시스템-구조-설계)
   - 1.1 시스템 구조도
   - 1.2 시스템 개요
2. [프로그램 설계](#2-프로그램-설계)
   - 2.1 전체 프로세스
   - 2.2 데이터 파일 불러오기, AI 요약, 저장 기능 프로세스
   - 2.3 자연어 분석, 시각화, 서식 편집 기능 프로세스
3. [사용자 인터페이스 설계](#3-사용자-인터페이스-설계) → Part 2
   - 3.1 데이터 파일 불러오기 기능
   - 3.2 AI 데이터 요약 기능
   - 3.3 자연어 분석 질문 기능
   - 3.4 시각화 서식 편집 기능
   - 3.5 차트 내보내기 기능

---

## 1. 시스템 구조 설계

### 1.1 시스템 구조도

#### 1.1.1 전체 시스템 아키텍처

```mermaid
graph TB
    subgraph Presentation["프레젠테이션 레이어 (MFC View)"]
        MF[CMainFrame<br/>메인 프레임 윈도우]
        DV[CDashboardView<br/>대시보드 뷰]
        QIV[CQueryInputView<br/>질문 입력 뷰]
        DSV[CDataSummaryView<br/>데이터 요약 뷰]
        CV[CChartView<br/>차트 뷰]
    end

    subgraph Domain["도메인 레이어 (비즈니스 로직)"]
        AO[AnalysisOrchestrator<br/>AI 오케스트레이터]
        AT[AnalysisTools<br/>분석 도구 함수군]
        CS[ChartSelector<br/>차트 유형 선택]
        DSS[DataSourceService<br/>데이터소스 관리]
        DSM[DataSummarizer<br/>AI 데이터 요약]
        DM[DashboardManager<br/>대시보드 관리]
    end

    subgraph Infrastructure["인프라 레이어 (C++ 라이브러리)"]
        LR[LLMRouter<br/>멀티 LLM 라우터]
        CP[ClaudeProvider]
        OP[OpenAIProvider]
        CSV[CSVParser]
        XLS[ExcelParser]
        DB[(SQLiteDB<br/>로컬 데이터베이스)]
        AC[AnalysisCache<br/>인메모리 캐시]
    end

    subgraph Renderer["렌더링 레이어 (GDI+)"]
        CR[CChartRenderer<br/>차트 렌더링 팩토리]
        BC[BarChart]
        LC[LineChart]
        PC[PieChart]
        SC[ScatterChart]
    end

    MF --> DV & QIV & DSV
    DV --> CV
    QIV -->|자연어 질문| AO
    AO -->|CoT 추론| LR
    AO -->|도구 호출| AT
    AO -->|차트 결정| CS
    DSS -->|파일 파싱| CSV & XLS
    DSS -->|요약 요청| DSM
    DSM -->|LLM 호출| LR
    LR --> CP & OP
    DM -->|CRUD| DB
    AT -->|결과 캐시| AC
    CV -->|렌더링| CR
    CR --> BC & LC & PC & SC

    style Presentation fill:#4F46E5,color:#fff
    style Domain fill:#059669,color:#fff
    style Infrastructure fill:#D97706,color:#fff
    style Renderer fill:#DC2626,color:#fff
```

#### 1.1.2 MFC Document/View 구조

```mermaid
graph TB
    APP[CDeepMetriaApp<br/>CWinApp 파생<br/>앱 진입점]
    DOC[CDeepMetriaDoc<br/>CDocument 파생<br/>데이터 모델 보유]
    FRAME[CMainFrame<br/>CFrameWnd 파생<br/>툴바/상태바/Splitter]

    APP --> FRAME
    APP --> DOC

    subgraph Splitter["CSplitterWnd 3분할"]
        LEFT[CDataSummaryView<br/>좌측 패널<br/>데이터 요약 표시]
        TOP_RIGHT[CQueryInputView<br/>우상단 패널<br/>질문 입력 폼]
        BOTTOM_RIGHT[CDashboardView<br/>우하단 패널<br/>시각화 대시보드]
    end

    FRAME --> Splitter
    DOC -->|UpdateAllViews| LEFT & TOP_RIGHT & BOTTOM_RIGHT
```

#### 1.1.3 클래스 다이어그램

```mermaid
classDiagram
    class CDeepMetriaApp {
        +ULONG_PTR m_gdiplusToken
        +InitInstance() BOOL
        +ExitInstance() int
        +OnAppAbout() void
    }

    class CMainFrame {
        -CToolBar m_wndToolBar
        -CStatusBar m_wndStatusBar
        -CSplitterWnd m_wndSplitter
        +SetStatusText(text) void
        +SetProgressValue(percent) void
        +OnAnalysisProgress() LRESULT
        +OnVisualizationReady() LRESULT
    }

    class CDeepMetriaDoc {
        -DataTable m_dataTable
        -DataSummary m_dataSummary
        -vector~VisualizationInfo~ m_visualizations
        +LoadFile(filePath, outError) BOOL
        +AddVisualization(viz) void
        +RemoveVisualization(vizId) void
        +GetDataTable() DataTable
        +GetDataSummary() DataSummary
    }

    class CDashboardView {
        -vector~VisualizationInfo~ m_visualizations
        -CString m_selectedVizId
        +AddVisualization(info) void
        +RemoveVisualization(vizId) void
        +SetDashboardDetail(detail) void
        -GetCardRect(index) CRect
        -DrawCard(pDC, index, rect) void
    }

    class CQueryInputView {
        -CEdit m_editQuery
        -CButton m_btnAnalyze
        -CProgressCtrl m_progressBar
        -BOOL m_bAnalyzing
        +SetDashboardView(pView) void
        +SetDataSourceId(id) void
        -StartAnalysis() void
    }

    class AnalysisOrchestrator {
        -atomic~BOOL~ m_bCancelled
        -atomic~AnalysisFlowState~ m_state
        +AnalyzeQuestion(data, summary, question, hWnd) BOOL
        +CancelAnalysis() void
        -Step1_Plan() BOOL
        -Step2_Execute() BOOL
        -Step3_Interpret() BOOL
        -Step4_Visualize() BOOL
    }

    class AnalysisTools {
        +BasicStats(data) CString
        +GroupByAggregate(data, groupCol, valueCol, aggFunc) CString
        +TimeSeriesAnalysis(data, dateCol, valueCol) CString
        +CorrelationMatrix(data, numericCols) CString
        +TopN(data, col, n, ascending) CString
        +FrequencyDistribution(data, col) CString
        +PivotTable(data, rowCol, colCol, valueCol, aggFunc) CString
        +OutlierDetection(data, col, threshold) CString
    }

    class LLMRouter {
        -Provider m_activeProvider
        -unique_ptr~ILLMProvider~ m_pClaude
        -unique_ptr~ILLMProvider~ m_pOpenAI
        +Instance() LLMRouter
        +Chat(systemPrompt, userMessage, outResponse, outError) BOOL
        +ChatAsync(systemPrompt, userMessage, hWnd, pContext) void
        +SetProvider(provider) void
        +SetApiKey(provider, apiKey) void
    }

    class SQLiteDB {
        -sqlite3* m_pDB
        +Instance() SQLiteDB
        +Initialize(dbPath, outError) BOOL
        +InsertDataSource() BOOL
        +InsertVisualization() BOOL
        +GetVisualizations(dashboardId) vector
    }

    class CChartRenderer {
        +Render(pDC, rect, config) void
        +RenderToFile(filePath, width, height, config) void
        +DrawTitle(g, rect, title) void
        +GetColorPalette() vector~Color~
    }

    CDeepMetriaApp --> CMainFrame
    CDeepMetriaApp --> CDeepMetriaDoc
    CMainFrame --> CDashboardView
    CMainFrame --> CQueryInputView
    CDeepMetriaDoc --> CDashboardView
    CQueryInputView --> AnalysisOrchestrator
    AnalysisOrchestrator --> LLMRouter
    AnalysisOrchestrator --> AnalysisTools
    CDashboardView --> CChartRenderer
    AnalysisOrchestrator --> SQLiteDB
```

#### 1.1.4 데이터베이스 ER 다이어그램

```mermaid
erDiagram
    users ||--o{ usage_quotas : "월별 쿼터"
    datasources ||--|{ data_schemas : "컬럼 스키마"
    datasources ||--|| data_summaries : "AI 요약"
    datasources ||--o{ dashboards : "대시보드"
    datasources ||--o{ analysis_flows : "분석 플로우"
    dashboards ||--o{ visualizations : "시각화"
    analysis_flows ||--o{ visualizations : "생성"

    users {
        INTEGER id PK
        TEXT license_key UK
        TEXT name
        TEXT tier "free|pro|max"
    }

    datasources {
        INTEGER id PK
        TEXT name
        TEXT file_type "csv|excel|json"
        TEXT local_path
        INTEGER file_size
        INTEGER row_count
        TEXT status "processing|ready|error"
    }

    data_schemas {
        INTEGER id PK
        INTEGER datasource_id FK
        TEXT column_name
        INTEGER column_index
        TEXT dtype "string|int|float|datetime"
    }

    data_summaries {
        INTEGER id PK
        INTEGER datasource_id FK
        TEXT domain
        TEXT description
        TEXT statistics "JSON"
    }

    dashboards {
        INTEGER id PK
        INTEGER datasource_id FK
        TEXT name
        TEXT layout "JSON LayoutItem[]"
    }

    visualizations {
        INTEGER id PK
        INTEGER dashboard_id FK
        INTEGER flow_id FK
        TEXT viz_type "line|bar|pie|scatter|table"
        TEXT title
        TEXT chart_config "JSON"
        TEXT style "JSON"
        TEXT position "JSON {x,y,w,h}"
    }

    analysis_flows {
        INTEGER id PK
        INTEGER datasource_id FK
        INTEGER dashboard_id FK
        TEXT question
        TEXT status "pending|running|completed|failed"
        TEXT cot_steps "JSON"
        TEXT tool_calls "JSON"
    }
```

### 1.2 시스템 개요

#### 1.2.1 시스템 목적

DeepMetria는 **비개발자 비즈니스 사용자**를 대상으로 하는 LLM 기반 데이터 분석 Windows 데스크톱 애플리케이션이다. 사용자가 데이터 파일(CSV/Excel/JSON)을 불러오면 AI가 자동으로 데이터를 요약하고, 자연어 질문에 따라 적절한 분석 도구와 차트를 선택하여 대시보드에 시각화를 생성한다.

#### 1.2.2 핵심 기술 스택

| 계층 | 기술 | 용도 |
|------|------|------|
| UI | MFC (CFrameWnd/CView/CSplitterWnd) | 윈도우 UI 프레임워크 |
| 렌더링 | GDI+ | 차트 렌더링 |
| LLM | WinHTTP (Claude API / OpenAI API) | AI 추론 |
| DB | SQLite 3 | 로컬 데이터 저장 |
| 캐시 | std::unordered_map | 분석 결과 캐시 |
| 빌드 | MSBuild (Visual Studio) | 컴파일/링크 |

#### 1.2.3 운영 환경

- **OS**: Windows 10 이상
- **배포**: MSI 인스톨러 (단일 프로세스 실행)
- **네트워크**: LLM API 호출 시에만 인터넷 필요
- **로컬 저장**: `%APPDATA%\DeepMetria\` 하위 SQLite DB 및 파일

---

## 2. 프로그램 설계

### 2.1 전체 프로세스

```mermaid
flowchart TD
    START([앱 실행]) --> INIT[GDI+ 초기화<br/>SQLiteDB 초기화<br/>API 키 로드]
    INIT --> LICENSE{라이선스<br/>검증}
    LICENSE -->|실패| ACT_DLG[CActivationDlg<br/>라이선스 입력]
    ACT_DLG --> LICENSE
    LICENSE -->|성공| MAIN[CMainFrame 생성<br/>3분할 Splitter 구성]

    MAIN --> IDLE[대기 상태]

    IDLE -->|파일 열기| FLOW_A[파일 불러오기 프로세스<br/>§2.2]
    IDLE -->|자연어 질문| FLOW_B[분석/시각화 프로세스<br/>§2.3]
    IDLE -->|서식 편집| FLOW_C[서식 편집 프로세스<br/>§2.3]
    IDLE -->|내보내기| FLOW_D[내보내기 프로세스]
    IDLE -->|설정| SETTINGS[CSettingsDialog<br/>API 키/모델 설정]

    FLOW_A --> IDLE
    FLOW_B --> IDLE
    FLOW_C --> IDLE
    FLOW_D --> IDLE
    SETTINGS --> IDLE

    IDLE -->|종료| SHUTDOWN[SQLiteDB 닫기<br/>GDI+ 해제<br/>앱 종료]
    SHUTDOWN --> END_APP([종료])

    style START fill:#4F46E5,color:#fff
    style END_APP fill:#DC2626,color:#fff
    style FLOW_A fill:#059669,color:#fff
    style FLOW_B fill:#D97706,color:#fff
```

### 2.2 데이터 파일 불러오기, AI 요약, 저장 기능 프로세스

#### 2.2.1 파일 불러오기 → AI 자동 요약 플로우

```mermaid
sequenceDiagram
    actor User as 사용자
    participant MF as CMainFrame
    participant FD as CFileDialog
    participant DOC as CDeepMetriaDoc
    participant DSS as DataSourceService
    participant Parser as CSVParser/<br/>ExcelParser
    participant DB as SQLiteDB
    participant DSM as DataSummarizer
    participant LLM as LLMRouter
    participant AT as AnalysisTools
    participant DSV as CDataSummaryView
    participant DV as CDashboardView

    User->>MF: 메뉴: 파일 열기
    MF->>FD: CFileDialog 표시
    FD-->>MF: filePath 반환

    MF->>DOC: LoadFile(filePath, outError)

    Note over DOC,Parser: 메인 스레드 — 파일 검증
    DOC->>DOC: 확장자 검증 (.csv/.xlsx/.json)
    DOC->>DOC: 파일 크기 확인 (≤50MB)

    DOC->>DB: InsertDataSource(name, path, type, size)
    DB-->>DOC: datasourceId

    Note over DOC,DSM: Worker Thread 생성 (AfxBeginThread)
    DOC->>Parser: Parse(filePath) → DataTable
    Parser-->>DOC: DataTable (headers, rows, columns)

    DOC->>DB: InsertDataSchema(각 컬럼 메타데이터)
    DOC->>DB: UpdateDataSourceCounts(rowCount, colCount)

    DOC->>DSM: Summarize(dataTable, outSummary, hWnd)

    Note over DSM,LLM: Worker Thread 내부 — AI 요약
    DSM->>AT: BasicStats(dataTable) → 기본 통계 JSON
    DSM->>DSM: BuildSummaryPrompt(data, statsJson)
    DSM->>LLM: Chat(systemPrompt, userMessage)
    LLM-->>DSM: AI 요약 텍스트 (도메인, 설명)

    DSM->>DB: InsertOrReplaceSummary(datasourceId, domain, description, stats)
    DSM->>DB: UpdateDataSourceStatus(id, "ready")

    DSM-->>MF: PostMessage(WM_DATA_LOADED)

    Note over MF,DV: 메인 스레드 — UI 갱신
    MF->>DOC: UpdateAllViews()
    DOC->>DSV: OnUpdate() → 스키마/통계 표시
    DOC->>DV: OnUpdate() → 기본 대시보드 구성
```

#### 2.2.2 파일 파싱 상세 흐름

```mermaid
flowchart TD
    INPUT[파일 경로 입력] --> EXT{확장자<br/>확인}
    EXT -->|.csv| CSV_P[CSVParser::Parse]
    EXT -->|.xlsx/.xls| XLS_P[ExcelParser::Parse]
    EXT -->|.json| JSON_P[RapidJSON 파싱]
    EXT -->|기타| ERR_TYPE[AppError:<br/>UNSUPPORTED_FILE_TYPE]

    CSV_P --> ENC{인코딩<br/>감지}
    ENC -->|UTF-8| PARSE_UTF8[UTF-8 파싱]
    ENC -->|EUC-KR| PARSE_EUC[EUC-KR 파싱]
    ENC -->|실패| ERR_ENC[인코딩 오류 처리]

    PARSE_UTF8 --> HEADER{헤더<br/>존재?}
    PARSE_EUC --> HEADER
    XLS_P --> HEADER
    JSON_P --> HEADER

    HEADER -->|있음| BUILD[DataTable 구성]
    HEADER -->|없음| AUTO_H[자동 헤더 생성<br/>col_0, col_1...]
    AUTO_H --> BUILD

    BUILD --> SCHEMA[컬럼 타입 추론<br/>numeric/text/date]
    SCHEMA --> DB_SAVE[SQLiteDB에<br/>스키마 저장]
    DB_SAVE --> DONE[파싱 완료]

    style ERR_TYPE fill:#DC2626,color:#fff
    style ERR_ENC fill:#DC2626,color:#fff
    style DONE fill:#059669,color:#fff
```

### 2.3 자연어 분석, 시각화, 서식 편집 기능 프로세스

#### 2.3.1 자연어 질문 → AI 분석 → 시각화 생성 플로우

```mermaid
sequenceDiagram
    actor User as 사용자
    participant QIV as CQueryInputView
    participant AO as AnalysisOrchestrator
    participant LLM as LLMRouter
    participant AT as AnalysisTools
    participant CS as ChartSelector
    participant DB as SQLiteDB
    participant AC as AnalysisCache
    participant MF as CMainFrame
    participant DV as CDashboardView
    participant CR as CChartRenderer

    User->>QIV: 자연어 질문 입력 + 분석 버튼
    QIV->>QIV: SetAnalyzingState(TRUE)
    QIV->>AO: AnalyzeQuestion(data, summary, question, hWnd)

    Note over AO,LLM: Worker Thread — Step 1: 분석 계획
    AO->>AO: BuildSchemaJson(summary)
    AO->>LLM: Chat(CoT 시스템 프롬프트 + 스키마 + 질문)
    LLM-->>AO: JSON 응답 (toolName, toolParams, chartType)
    AO->>AO: ParsePlanResponse() → 도구명 + 파라미터
    AO-->>MF: PostMessage(WM_ANALYSIS_PROGRESS, step=1)

    Note over AO,AT: Worker Thread — Step 2: 도구 실행
    AO->>AO: DispatchTool(toolName, toolParamsJson, data)
    AO->>AT: 해당 분석 함수 호출<br/>(예: GroupByAggregate)
    AT-->>AO: 분석 결과 JSON
    AO->>AC: 결과 캐시 저장
    AO-->>MF: PostMessage(WM_ANALYSIS_PROGRESS, step=2)

    Note over AO,CS: Worker Thread — Step 3: 인사이트 생성
    AO->>LLM: Chat(결과 데이터 + 원본 질문)
    LLM-->>AO: 인사이트 텍스트
    AO-->>MF: PostMessage(WM_ANALYSIS_PROGRESS, step=3)

    Note over AO,CS: Worker Thread — Step 4: 시각화 결정
    AO->>CS: SelectChart(analysisResult, dataType)
    CS-->>AO: ChartConfig (chartType, xKey, yKey)
    AO->>DB: InsertVisualization(dashboardId, vizType, config)
    AO-->>MF: PostMessage(WM_ANALYSIS_DONE, flowId)

    Note over MF,CR: 메인 스레드 — UI 갱신
    MF->>QIV: SetAnalyzingState(FALSE)
    MF->>DV: AddVisualization(vizInfo)
    DV->>CR: Render(pDC, rect, chartConfig)
    CR-->>DV: GDI+ 차트 렌더링 완료
```

#### 2.3.2 CoT(Chain-of-Thought) 4단계 분석 흐름

```mermaid
flowchart TD
    Q[사용자 자연어 질문] --> S1

    subgraph S1["Step 1: 분석 계획 (Plan)"]
        S1A[질문 + 데이터 스키마<br/>+ 도구 목록 → LLM 전송]
        S1B["LLM CoT 추론:<br/>1. 필요 컬럼 식별<br/>2. 분석 도구 선택<br/>3. 차트 유형 결정"]
        S1C[JSON 응답 파싱<br/>toolName + toolParams]
        S1A --> S1B --> S1C
    end

    S1 --> S2

    subgraph S2["Step 2: 도구 실행 (Execute)"]
        S2A[AnalysisTools 디스패치]
        S2B["C++ 분석 함수 호출<br/>(BasicStats, GroupBy,<br/>TimeSeries 등 13종)"]
        S2C[분석 결과 JSON 생성<br/>+ AnalysisCache 저장]
        S2A --> S2B --> S2C
    end

    S2 --> S3

    subgraph S3["Step 3: 인사이트 생성 (Interpret)"]
        S3A[결과 데이터 + 원본 질문<br/>→ LLM 전송]
        S3B[LLM이 자연어<br/>인사이트 텍스트 생성]
        S3A --> S3B
    end

    S3 --> S4

    subgraph S4["Step 4: 시각화 결정 (Visualize)"]
        S4A[ChartSelector::<br/>SelectChart 호출]
        S4B[ChartConfig 구성<br/>+ SQLite 저장]
        S4C["PostMessage<br/>(WM_ANALYSIS_DONE)"]
        S4A --> S4B --> S4C
    end

    S4 --> RENDER[CDashboardView<br/>차트 렌더링]

    style S1 fill:#4F46E5,color:#fff
    style S2 fill:#059669,color:#fff
    style S3 fill:#D97706,color:#fff
    style S4 fill:#DC2626,color:#fff
```

#### 2.3.3 시각화 서식 편집 프로세스

```mermaid
sequenceDiagram
    actor User as 사용자
    participant DV as CDashboardView
    participant Card as CVisualizationCard
    participant SEP as CStyleEditorPanel
    participant VM as VisualizationManager
    participant DB as SQLiteDB
    participant CR as CChartRenderer

    User->>DV: 시각화 카드 클릭
    DV->>DV: SelectCard(vizId)
    DV->>SEP: 선택된 카드 정보 로드

    alt 서식 변경
        User->>SEP: 색상/폰트/범례 변경
        SEP->>Card: m_vizInfo.style 업데이트
        Card->>Card: Invalidate() → 즉시 리렌더
        Card->>CR: RenderChart(pDC) 재호출
        Note over SEP,DB: 3초 idle 타이머 만료 시
        SEP->>VM: UpdateStyle(vizId, position, style)
        VM->>DB: UPDATE visualizations SET style=...
    end

    alt 크기/위치 변경
        User->>Card: 드래그/리사이즈
        Card->>DV: 레이아웃 변경 통보
        Note over DV,DB: Debounce 500ms 후
        DV->>DB: UpdateDashboardLayout(layoutJson)
    end
```

#### 2.3.4 차트 내보내기 프로세스

```mermaid
flowchart TD
    START[내보내기 버튼 클릭] --> DLG[CExportDialog 표시]
    DLG --> FMT{형식 선택}

    FMT -->|PNG| SIZE_PNG[이미지 크기 설정<br/>너비/높이]
    FMT -->|BMP| SIZE_BMP[이미지 크기 설정<br/>너비/높이]
    FMT -->|CSV| PATH_CSV[저장 경로 선택]

    SIZE_PNG --> PATH_PNG[CFileDialog<br/>저장 경로 선택]
    SIZE_BMP --> PATH_BMP[CFileDialog<br/>저장 경로 선택]

    PATH_PNG --> RENDER_PNG[CChartRenderer::<br/>RenderToFile<br/>GDI+ Image::Save]
    PATH_BMP --> RENDER_BMP[CChartRenderer::<br/>RenderToFile<br/>GDI+ Image::Save]
    PATH_CSV --> EXPORT_CSV[차트 데이터를<br/>CSV 텍스트로 직렬화]

    RENDER_PNG --> RESULT{성공?}
    RENDER_BMP --> RESULT
    EXPORT_CSV --> RESULT

    RESULT -->|성공| MSG_OK["PostMessage<br/>(WM_EXPORT_COMPLETE, 1)"]
    RESULT -->|실패| MSG_ERR["AfxMessageBox<br/>오류 메시지"]

    MSG_OK --> DONE[상태바: 내보내기 완료]

    style START fill:#4F46E5,color:#fff
    style DONE fill:#059669,color:#fff
    style MSG_ERR fill:#DC2626,color:#fff
```

---

> **Part 2 (사용자 인터페이스 설계)는 `상세설계서_part2.md`에서 계속됩니다.**
