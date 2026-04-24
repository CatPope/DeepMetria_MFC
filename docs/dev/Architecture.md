# DeepMetria Architecture — LLM 기반 데이터 분석 MFC 데스크톱 앱 아키텍처 설계

## 목차
1. 시스템 개요 및 아키텍처 패턴 ........................... L17
2. 기술 스택 확정 .......................................... L65
3. 모듈 / 레이어 구조 ..................................... L112
4. 데이터 흐름 (핵심 플로우) .............................. L192
5. AI 오케스트레이션 아키텍처 ............................. L255
6. 인프라 토폴로지 (배포 기준) ............................ L348
7. 확장 계획 (post-MVP) ................................... L400




---

## 1. 시스템 개요 및 아키텍처 패턴

### 1.1 시스템 개요

DeepMetria는 비개발자 비즈니스 사용자를 대상으로 하는 LLM 기반 데이터 분석 데스크톱 애플리케이션이다.
사용자가 데이터 파일을 불러오면 AI가 자동으로 데이터를 요약하고, 자연어 질문에 따라
적절한 분석 도구와 차트를 선택하여 대시보드 패널에 시각화를 생성한다.

모든 기능은 단일 MFC 프로세스 내에서 동작하며, 별도의 서버나 네트워크 없이 실행된다.
LLM 연동만 외부 API(Claude/GPT)를 libcurl로 직접 호출한다.

```
┌──────────────────────────────────────────────────────┐
│              MFC 애플리케이션 (단일 프로세스)           │
│                                                      │
│  ┌────────────────────────────────────────────────┐  │
│  │           프레젠테이션 레이어 (MFC View)         │  │
│  │  CMainFrame / CSplitterWnd / CChartView        │  │
│  │  CQueryInputView / CDashboardView              │  │
│  └──────────────────┬─────────────────────────────┘  │
│                     ↓                                │
│  ┌────────────────────────────────────────────────┐  │
│  │       도메인 레이어 (Document + Service)         │  │
│  │  CDeepMetriaDoc / CAnalysisOrchestrator        │  │
│  │  CDataSummarizer / CChartSelector              │  │
│  └──────────────────┬─────────────────────────────┘  │
│                     ↓                                │
│  ┌────────────────────────────────────────────────┐  │
│  │           인프라 레이어 (C++ 라이브러리)          │  │
│  │  libcurl (LLM API) / SQLite / CSV파서           │  │
│  │  std::unordered_map (캐시) / GDI+ (렌더링)     │  │
│  └────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────┘
```

### 1.2 아키텍처 패턴

| 패턴 | 적용 근거 |
|------|----------|
| **MFC Document/View 아키텍처** | 데이터(Document)와 표현(View)의 분리. 멀티뷰 지원 |
| **AI 오케스트레이터 패턴** | LLM은 분석 도구 선택만 수행, 실제 계산은 C++ 내부 함수가 담당 |
| **추상화 레이어 (LLM Router)** | Claude/GPT를 단일 C++ 인터페이스로 라우팅 |
| **Splitter Window 대시보드** | CSplitterWnd로 다중 패널 레이아웃 구성 |
| **Worker Thread 비동기 처리** | LLM API 호출 및 대용량 파일 파싱을 별도 스레드에서 처리, UI 블로킹 방지 |
| **로컬 데이터 격리** | SQLite 로컬 DB로 사용자 데이터 관리, 외부 전송 없음 |

---

## 2. 기술 스택 확정

### 2.1 UI / 프레젠테이션

| 항목 | 선택 기술 | 선택 근거 |
|------|-----------|----------|
| UI 프레임워크 | **MFC (Microsoft Foundation Classes)** | Win32 네이티브, SDI/MDI/Splitter 지원 |
| 핵심 윈도우 클래스 | **CFrameWnd, CView, CDialog, CToolBar, CStatusBar** | MFC 표준 계층 구조 |
| 차트 렌더링 | **GDI+ 기반 커스텀 ChartRenderer** | 라이선스 무료, 네이티브 성능 |
| 대시보드 레이아웃 | **CSplitterWnd + 커스텀 패널** | 드래그/리사이즈 지원 가능 |
| 다이얼로그/입력 | **CDialog + CEdit + CComboBox** | 자연어 질문 입력, 설정 패널 |

### 2.2 백엔드 (앱 내 직접 통합)

| 항목 | 선택 기술 | 선택 근거 |
|------|-----------|----------|
| LLM API 호출 | **libcurl (C++)** | Claude API / OpenAI API HTTP 직접 호출 |
| JSON 파싱 | **nlohmann/json 또는 RapidJSON** | LLM 응답 파싱, 경량 헤더 전용 |
| 데이터 처리 | **C++ 자체 구현 (분석 함수군)** | CSV/Excel 파싱, 통계 계산 |
| CSV 파싱 | **fast-cpp-csv-parser 또는 자체 구현** | 대용량 CSV 고속 처리 |
| Excel 파싱 | **libxlsxwriter / OpenXLSX** | xlsx 읽기/쓰기 지원 |

### 2.3 데이터베이스 및 캐시

| 항목 | 선택 기술 | 선택 근거 |
|------|-----------|----------|
| 로컬 DB | **SQLite (sqlite3 C API)** | 설치 불필요, 파일 기반, 오프라인 동작 |
| 외부 DB 연결 | **ODBC (Windows ODBC API)** | PostgreSQL/MySQL/MSSQL 연결 옵션 |
| 캐시 | **std::unordered_map<string, AnalysisResult>** | 인메모리 캐시, 분석 결과 재사용 |

### 2.4 인프라 및 배포

| 항목 | 선택 기술 | 선택 근거 |
|------|-----------|----------|
| 빌드 시스템 | **MSBuild (Visual Studio .sln)** | MFC 표준 빌드 환경 |
| 인스톨러 | **MSI (WiX Toolset) 또는 NSIS** | Windows 표준 설치 패키지 |
| CI/CD | **GitHub Actions (Windows runner)** | 자동 빌드 + MSI 패키징 |

### 2.5 기술 스택 선택 요약

```
UI:       MFC (CFrameWnd/CView/CSplitterWnd) + GDI+ 차트 렌더링
Logic:    C++ 분석 함수군 (통계/집계/시계열/상관관계)
LLM:      libcurl → Claude API / OpenAI API
DB:       SQLite (로컬) + ODBC (외부 선택)
Cache:    std::unordered_map (인메모리)
Deploy:   MSI 인스톨러 + GitHub Actions
```

---

## 3. 모듈 / 레이어 구조

### 3.1 MFC 프로젝트 구조

```
DeepMetria/
├── App/
│   ├── DeepMetriaApp.h/.cpp          # CWinApp 파생 — 앱 진입점, 초기화
│   └── MainFrm.h/.cpp                # CMainFrame — 메인 프레임, 툴바/상태바
├── Document/
│   └── DeepMetriaDoc.h/.cpp          # CDocument 파생 — 데이터 모델 보유
├── View/
│   ├── DashboardView.h/.cpp          # CDashboardView — 대시보드 메인 패널
│   ├── ChartView.h/.cpp              # CChartView — GDI+ 차트 렌더링 뷰
│   ├── QueryInputView.h/.cpp         # CQueryInputView — 자연어 질문 입력 패널
│   └── DataSummaryView.h/.cpp        # CDataSummaryView — 데이터 요약 표시
├── Domain/
│   ├── Orchestrator/
│   │   ├── AnalysisOrchestrator.h/.cpp  # AI 오케스트레이터 (CoT 흐름 관리)
│   │   └── AnalysisFlow.h/.cpp          # AnalysisFlow 상태 관리
│   ├── DataSource/
│   │   ├── DataSourceService.h/.cpp     # 파일 로드, 파싱 조율
│   │   └── DataSummarizer.h/.cpp        # AI 데이터 요약 생성
│   ├── Analysis/
│   │   ├── AnalysisTools.h/.cpp         # 내부 분석 도구 함수군
│   │   └── ChartSelector.h/.cpp         # 차트 유형 자동 선택
│   └── Dashboard/
│       └── DashboardManager.h/.cpp      # 패널 배치, 시각화 카드 관리
├── Infrastructure/
│   ├── LLM/
│   │   ├── LLMRouter.h/.cpp             # LLM 추상화 — Claude/GPT 라우팅
│   │   ├── ClaudeProvider.h/.cpp        # Claude API (libcurl)
│   │   └── OpenAIProvider.h/.cpp        # OpenAI API (libcurl)
│   ├── Parser/
│   │   ├── CSVParser.h/.cpp             # CSV 파일 파싱
│   │   └── ExcelParser.h/.cpp           # xlsx 파일 파싱
│   ├── Storage/
│   │   ├── SQLiteDB.h/.cpp              # SQLite CRUD 래퍼
│   │   └── ODBCConnector.h/.cpp         # 외부 DB ODBC 연결
│   └── Cache/
│       └── AnalysisCache.h/.cpp         # std::unordered_map 기반 캐시
├── Renderer/
│   ├── ChartRenderer.h/.cpp             # GDI+ 차트 렌더링 엔진
│   ├── BarChart.h/.cpp                  # 바 차트
│   ├── LineChart.h/.cpp                 # 라인 차트
│   ├── PieChart.h/.cpp                  # 파이/도넛 차트
│   └── ScatterChart.h/.cpp              # 산점도
├── Dialog/
│   ├── FileOpenDialog.h/.cpp            # 파일 열기 다이얼로그
│   ├── SettingsDialog.h/.cpp            # LLM API 키 설정
│   └── ExportDialog.h/.cpp              # 차트 내보내기 (PNG/BMP)
└── Resources/
    ├── DeepMetria.rc                     # MFC 리소스 파일
    └── res/                              # 아이콘, 툴바 비트맵
```

---

## 4. 데이터 흐름 (핵심 플로우)

### 4.1 파일 열기 → 데이터 요약 플로우 (FR-01, FR-02)

```
[CMainFrame 메뉴: 파일 열기]
    → CFileDialog (MFC) → 파일 경로 선택
    → CDeepMetriaDoc::LoadFile(path)
        → CSVParser / ExcelParser → DataTable 객체 생성
        → SQLiteDB에 DataSource 메타 저장
    → CStatusBar: "파일 로드 완료, 분석 시작..."
    → Worker Thread 생성 (AfxBeginThread)
        → CDataSummarizer::Summarize(dataTable)
            → AnalysisTools::BasicStats(dataTable)   ← 기본 통계 수집
            → LLMRouter::Chat(schemaPrompt)          ← 스키마 요약 요청
            → CoT 추론 → DataSummary 생성
        → PostMessage(WM_ANALYSIS_DONE, summary)
    → CDataSummaryView 갱신 (UpdateAllViews)
    → CDashboardView: 기본 패널 구성
```

### 4.2 자연어 질문 → 시각화 플로우 (FR-03, FR-04, FR-05)

```
[CQueryInputView: 질문 텍스트 입력 + 실행 버튼]
    → CAnalysisOrchestrator::Query(question, dataTable)
    → AnalysisFlow 객체 생성 (status: pending)
    → Worker Thread 생성
        → CStatusBar: "CoT 추론 중..."

    [CAnalysisOrchestrator 내부 — Worker Thread]
        → LLMRouter::Chat(cotPrompt + toolSchema)
            Step 1: 필요한 데이터 컬럼 식별
            Step 2: 내부 분석 도구 선택
            Step 3: 차트 유형 결정
        → LLM 응답(JSON) 파싱 → 도구 이름 + 파라미터 추출

        → AnalysisTools 함수 직접 호출
            예: AnalysisTools::MonthlyTrend(col="sales", table)
        → 분석 결과 → AnalysisCache에 저장
        → CChartSelector::Recommend(result) → ChartType 확정
        → ChartConfig 객체 생성 (type, data, columns)
        → SQLiteDB에 Visualization 저장

        → PostMessage(WM_VISUALIZATION_DONE, chartConfig)

    [메인 스레드 WM_VISUALIZATION_DONE 핸들러]
        → CDashboardView::AddChart(chartConfig)
        → CChartView 패널 추가 → ChartRenderer 렌더링
        → CDeepMetriaDoc::SetModifiedFlag()
```

### 4.3 차트 편집 → 내보내기 플로우 (FR-06, FR-07)

```
[CDashboardView: 패널 드래그/리사이즈]
    → DashboardManager::UpdateLayout(panelId, rect)
    → SQLiteDB Visualization 위치/크기 업데이트
    → CChartView::Invalidate() → 즉시 재렌더링

[툴바 내보내기 버튼]
    → CExportDialog: 형식 선택 (PNG/BMP/SVG)
    → ChartRenderer::ExportToFile(path, format)
        → GDI+ Image::Save() → 파일 저장
    → CStatusBar: "내보내기 완료"
```

---

## 5. AI 오케스트레이션 아키텍처

### 5.1 AI 오케스트레이터 구조

AI는 직접 계산하지 않는다. LLM은 오케스트레이터 역할만 수행하며,
C++ 내부 분석 함수(AnalysisTools)를 선택하고 실행 결과를 해석한다.

```
[LLMRouter]
    ↓ libcurl HTTP POST
[LLM Provider]  ←→  Claude API / OpenAI API
    ↓ Chain of Thinking 추론 (JSON 응답)
[도구 선택 결정]
    ↓ JSON 파싱 → 함수명 + 파라미터 추출
[C++ AnalysisTools 함수 직접 호출]
    ├── BasicStats(table)           — 평균, 중앙값, 표준편차
    ├── Distribution(col, table)   — 분포 분석
    ├── GroupBy(keyCol, valCol)    — 그룹 집계
    ├── PivotTable(rows, cols)     — 피벗 테이블
    ├── MonthlyTrend(col, table)   — 월별 추이
    ├── MovingAverage(col, window) — 이동평균
    ├── PearsonCorrelation(c1, c2) — 피어슨 상관계수
    └── RecommendChart(result)     — 차트 유형 추천
```

### 5.2 LLMRouter (멀티 LLM 추상화)

libcurl을 사용하여 Claude / GPT를 단일 C++ 인터페이스로 추상화한다.

```cpp
// Infrastructure/LLM/LLMRouter.h (개념 구조)
class LLMRouter {
public:
    LLMRouter(LLMProvider provider, const std::string& model);
    LLMResponse Chat(const std::vector<Message>& messages,
                     const ToolSchema& tools);
private:
    std::unique_ptr<ILLMProvider> m_provider;  // Claude 또는 OpenAI
};

// 비동기 호출 — Worker Thread에서 실행
LLMResponse ClaudeProvider::Chat(messages, tools) {
    // libcurl로 https://api.anthropic.com/v1/messages 호출
    // JSON 직렬화(nlohmann/json) → HTTP POST → 응답 파싱
}
```

지원 제공자:
- `claude-3-5-sonnet-20241022` (기본값, CoT 성능 우수)
- `gpt-4o` (대체 옵션)

API 키는 Windows 레지스트리 또는 앱 로컬 설정 파일에 암호화 저장.

### 5.3 Chain of Thinking (CoT) 프롬프트 전략

```
시스템 프롬프트:
  "당신은 데이터 분석 오케스트레이터입니다.
   사용자의 질문을 분석하여 다음 단계를 명시적으로 추론하세요:
   1. 필요한 데이터 컬럼 식별
   2. 적합한 분석 도구 선택 (제공된 내부 도구 목록 내에서만)
   3. 적합한 차트 유형 결정
   각 단계를 <thinking> 태그로 명시하세요.
   최종 출력은 JSON: { tool, params, chart_type }"

도구 목록: AnalysisTools 함수 스키마를 JSON으로 직렬화하여 LLM에 전달
```

### 5.4 Worker Thread 비동기 처리

LLM API 호출은 네트워크 대기가 발생하므로 반드시 Worker Thread에서 처리한다.

```
메인 스레드 (UI)
    → AfxBeginThread(AnalysisThreadProc, param)
    → CStatusBar 진행 표시

Worker Thread
    → LLMRouter::Chat(...)          ← HTTP 대기 (블로킹)
    → AnalysisTools 함수 실행
    → ::PostMessage(hWnd, WM_ANALYSIS_DONE, ...)

메인 스레드 WM_ANALYSIS_DONE 핸들러
    → UpdateAllViews() → 화면 갱신
```

### 5.5 차트 자동 선택 로직

AI가 데이터 특성과 사용자 질문을 분석하여 차트 유형을 자동 결정한다.

| 데이터 특성 | 기본 차트 후보 | Renderer 클래스 |
|------------|--------------|----------------|
| 시간 + 수치 | 라인 차트, 영역 차트 | LineChart |
| 카테고리 + 수치 | 바 차트, 수평 바 차트 | BarChart |
| 비율/구성 | 파이 차트, 도넛 차트 | PieChart |
| 두 수치 간 관계 | 산점도 | ScatterChart |
| 테이블 형태 필요 | CListCtrl 그리드 | (MFC 내장) |

---

## 6. 인프라 토폴로지 (배포 기준)

### 6.1 설치형 앱 배포 구성

```
[개발 머신 (Visual Studio)]
    → MSBuild (Release x64)
    → WiX Toolset → DeepMetria-x.y.z.msi 생성
        └── 포함 항목:
            ├── DeepMetria.exe (MFC 앱)
            ├── sqlite3.dll
            ├── 필요한 VC++ 런타임
            └── 기본 설정 파일

[사용자 PC]
    → MSI 설치 → C:\Program Files\DeepMetria\
    → 앱 실행 → SQLite DB 자동 생성 (~/.deepmetria/data.db)
    → 설정 다이얼로그에서 LLM API 키 입력 → 레지스트리 저장
```

### 6.2 환경별 구성

| 환경 | 빌드 구성 | DB | LLM |
|------|-----------|-----|-----|
| 개발 (Debug) | VS Debug 빌드 | SQLite (로컬 파일) | API 키 환경변수 |
| 테스트 (Release) | VS Release 빌드 | SQLite (임시 DB) | Mock LLM 응답 |
| 배포 (Installer) | MSI 패키징 | SQLite (사용자 홈) | 설정 다이얼로그 입력 |

### 6.3 보안 설계

| 위협 | 대응 방안 |
|------|----------|
| API 키 노출 | Windows CryptProtectData API로 레지스트리 암호화 저장 |
| 파일 경로 인젝션 | 파일 확장자 검증(magic bytes), 경로 정규화 |
| 전송 암호화 | libcurl TLS 1.2+ (CURLOPT_SSLVERSION 강제) |
| 로컬 DB 접근 | SQLite 파일 사용자 디렉터리 격리, 앱 전용 접근 |
| 메모리 안전성 | RAII 패턴, smart pointer 사용, 버퍼 크기 검증 |

### 6.4 CI/CD 파이프라인 (GitHub Actions)

```
[GitHub 코드 푸시]
    → GitHub Actions (windows-latest runner)
        ├── MSBuild (x64 Release) — 컴파일 및 링크
        ├── 단위 테스트 실행 (Google Test)
        ├── WiX Toolset → MSI 패키지 생성
        └── GitHub Releases에 MSI 업로드 (태그 푸시 시)
```

---

## 7. 확장 계획 (post-MVP)

### 7.1 DataSource 확장 (외부 DB / API 연결)

DataSource 추상화 인터페이스를 통해 새로운 데이터 원천을 추가한다.

```
IDataSource (abstract)
    ├── FileDataSource (MVP)       — CSV, Excel 파일 열기
    ├── SQLiteDataSource (MVP)     — 로컬 SQLite DB 직접 쿼리
    ├── ODBCDataSource (post-MVP)  — PostgreSQL/MySQL/MSSQL ODBC 연결
    └── RESTDataSource (post-MVP)  — REST API JSON 데이터 가져오기
```

### 7.2 렌더링 확장 (post-MVP)

| 단계 | 내용 |
|------|------|
| MVP | GDI+ 기반 기본 차트 (바/라인/파이/산점도) |
| post-MVP | OpenGL 또는 Direct2D 기반 GPU 가속 렌더링, 인터랙티브 줌/팬 |
| post-MVP | SVG 내보내기 (벡터 그래픽) |

### 7.3 다국어 지원 (post-MVP)

MFC 리소스 DLL 방식으로 다국어 UI 문자열 분리.
현재 한국어 전용 → post-MVP에서 영어/일본어 리소스 DLL 추가.

### 7.4 확장성 고려 사항

| 항목 | MVP | post-MVP |
|------|-----|----------|
| 동시 분석 | Worker Thread 1개 | Thread Pool (std::thread) |
| 파일 크기 | 50MB 제한 (메모리 로드) | 청크 스트리밍 파싱 |
| 차트 종류 | 4종 (바/라인/파이/산점도) | 히스토그램, 박스플롯, 히트맵 추가 |
| LLM 모델 | Claude + GPT | Gemini, 로컬 LLM (llama.cpp) 추가 |
| 모니터링 | 로컬 로그 파일 | 앱 텔레메트리 (선택적 opt-in) |
