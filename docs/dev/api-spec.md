# C++ 클래스 인터페이스 명세서

## 목차
1. 개요 (Overview) ..................... L19
2. DataSourceManager .................. L46
3. AnalysisEngine ..................... L109
4. DashboardManager ................... L172
5. LLMClient .......................... L224
6. 공통 타입 및 에러 코드 .............. L289





---

## 1. 개요 (Overview)

DeepMetria MFC는 REST API 대신 C++ 클래스 인터페이스를 통해 기능을 제공합니다.
모든 클래스는 싱글톤 또는 매니저 패턴으로 구현되며, MFC 메인 스레드와 워커 스레드 간
통신은 `PostMessage` / `CEvent` 기반 비동기 패턴을 사용합니다.

**네임스페이스:** `DeepMetria::`

**공통 규칙:**
- 반환 타입 `BOOL` → TRUE 성공 / FALSE 실패 (세부 에러는 `GetLastError()`)
- 문자열: `CString` (MFC) 또는 `std::wstring`
- JSON 직렬화: `nlohmann::json`
- HTTP 요청 (LLM API): `libcurl` 기반 `LLMClient`
- SQLite 접근: `sqlite3` C API 직접 사용

**의존 라이브러리:**

| 라이브러리 | 용도 | 버전 |
|------------|------|------|
| sqlite3 | 로컬 DB | 3.45+ |
| libcurl | LLM HTTP 요청 | 8.x |
| nlohmann/json | JSON 파싱/직렬화 | 3.11+ |
| OpenSSL | API 키 암호화, HTTPS | 3.x |

---

## 2. DataSourceManager

파일 로드, 스키마 분석, 데이터 요약 생성을 담당합니다.

### 헤더: `src/data/DataSourceManager.h`

```cpp
namespace DeepMetria {

class DataSourceManager {
public:
    // 싱글톤 접근
    static DataSourceManager& GetInstance();

    // 파일 로드 (CSV/Excel/JSON) — 워커 스레드에서 실행
    // filePath: 로컬 파일 전체 경로
    // 완료 시 WM_DM_DATASOURCE_READY 메시지 전송
    BOOL LoadFile(const CString& filePath, HWND hNotifyWnd);

    // 전체 데이터소스 목록 조회
    std::vector<DataSourceInfo> GetAllDataSources() const;

    // 데이터소스 상세 조회 (스키마 + 요약 포함)
    BOOL GetDataSource(int datasourceId, DataSourceDetail& outDetail) const;

    // 데이터소스 삭제 (연관 레코드 CASCADE 삭제)
    BOOL DeleteDataSource(int datasourceId);

    // 마지막 에러 문자열 반환
    CString GetLastErrorMsg() const;

private:
    DataSourceManager() = default;
    BOOL ParseCSV(const CString& filePath, int datasourceId);
    BOOL ParseExcel(const CString& filePath, int datasourceId);
    BOOL ParseJSON(const CString& filePath, int datasourceId);
    BOOL ComputeStatistics(int datasourceId);
};

} // namespace DeepMetria
```

### 구조체

```cpp
struct DataSourceInfo {
    int     id;
    CString name;
    CString filePath;
    CString fileType;   // "csv" | "excel" | "json"
    INT64   fileSize;
    int     rowCount;
    int     columnCount;
    CString status;     // "processing" | "ready" | "error"
    CString errorMessage;
    CString createdAt;
};

struct ColumnSchema {
    int     id;
    CString columnName;
    int     columnIndex;
    CString dtype;      // "string" | "int" | "float" | "datetime" | "boolean"
    BOOL    nullable;
    std::vector<CString> sampleValues;
};

struct DataSourceDetail : public DataSourceInfo {
    std::vector<ColumnSchema> schema;
    nlohmann::json            statistics; // mean/std/min/max per column
    CString                   domain;
    CString                   description;
};
```

### 알림 메시지

| 메시지 상수 | wParam | lParam | 설명 |
|-------------|--------|--------|------|
| `WM_DM_DATASOURCE_READY` | datasourceId | 0 | 파일 로드/분석 완료 |
| `WM_DM_DATASOURCE_ERROR` | datasourceId | errorCode | 로드 실패 |

---

## 3. AnalysisEngine

자연어 질문 → LLM 분석 → 시각화 생성 파이프라인을 담당합니다.

### 헤더: `src/analysis/AnalysisEngine.h`

```cpp
namespace DeepMetria {

class AnalysisEngine {
public:
    static AnalysisEngine& GetInstance();

    // 분석 시작 — 워커 스레드에서 LLM 호출
    // 반환: flowId (음수면 실패)
    int StartAnalysis(const AnalysisRequest& req, HWND hNotifyWnd);

    // 진행 중인 분석 취소
    BOOL CancelAnalysis(int flowId);

    // 분석 결과 조회 (completed 상태일 때만 유효)
    BOOL GetAnalysisResult(int flowId, AnalysisResult& outResult) const;

    // 분석 이력 목록
    std::vector<AnalysisFlowInfo> GetFlowHistory(int datasourceId) const;

    CString GetLastErrorMsg() const;

private:
    AnalysisEngine() = default;
    BOOL BuildPrompt(const AnalysisRequest& req, std::string& outPrompt) const;
    BOOL ParseLLMResponse(const std::string& response, AnalysisResult& out) const;
};

} // namespace DeepMetria
```

### 구조체

```cpp
struct AnalysisRequest {
    int     datasourceId;
    int     dashboardId;  // 0이면 미지정
    CString question;     // 자연어 질문 (최대 2000자)
};

struct CotStep {
    int     step;
    CString description;
};

struct ToolCall {
    CString tool;
    CString function;
    nlohmann::json params;
};

struct AnalysisResult {
    int                    flowId;
    CString                status;  // "pending"|"running"|"completed"|"failed"
    CString                llmProvider;
    CString                llmModel;
    std::vector<CotStep>   cotSteps;
    std::vector<ToolCall>  toolCalls;
    nlohmann::json         vizConfig;  // 생성된 시각화 설정
    CString                errorMessage;
};

struct AnalysisFlowInfo {
    int     flowId;
    int     datasourceId;
    CString question;
    CString status;
    CString createdAt;
};
```

### 알림 메시지

| 메시지 상수 | wParam | lParam | 설명 |
|-------------|--------|--------|------|
| `WM_DM_ANALYSIS_COT_STEP` | flowId | stepIndex | CoT 단계 진행 |
| `WM_DM_ANALYSIS_COMPLETE` | flowId | 0 | 분석 완료 |
| `WM_DM_ANALYSIS_ERROR` | flowId | errorCode | 분석 실패 |

---

## 4. DashboardManager

대시보드 및 시각화 CRUD를 담당합니다.

### 헤더: `src/dashboard/DashboardManager.h`

```cpp
namespace DeepMetria {

class DashboardManager {
public:
    static DashboardManager& GetInstance();

    // 대시보드 CRUD
    int  CreateDashboard(const CString& name, int datasourceId);
    BOOL GetDashboard(int dashboardId, DashboardDetail& outDetail) const;
    std::vector<DashboardInfo> GetAllDashboards() const;
    BOOL UpdateDashboard(int dashboardId, const CString& name,
                         const nlohmann::json& layout);
    BOOL DeleteDashboard(int dashboardId);  // 소프트 삭제

    // 시각화 CRUD
    int  AddVisualization(const VizCreateParams& params);
    BOOL GetVisualization(int vizId, VizDetail& outDetail) const;
    BOOL UpdateVisualization(int vizId, const VizUpdateParams& params);
    BOOL DeleteVisualization(int vizId);

    // 시각화 PNG 내보내기
    // outFilePath: 저장할 경로 (비워두면 임시 경로 자동 지정)
    BOOL ExportVizAsPNG(int vizId, CString& outFilePath) const;

    CString GetLastErrorMsg() const;
};

} // namespace DeepMetria
```

### 구조체

```cpp
struct DashboardInfo {
    int     id;
    int     datasourceId;
    CString name;
    CString createdAt;
    CString updatedAt;
};

struct DashboardDetail : public DashboardInfo {
    nlohmann::json          layout;
    std::vector<VizDetail>  visualizations;
};

struct VizCreateParams {
    int            dashboardId;
    int            flowId;     // 0이면 수동 생성
    CString        vizType;    // "line"|"bar"|"pie" 등
    CString        title;
    nlohmann::json chartConfig;
    nlohmann::json style;
    nlohmann::json position;
};

struct VizUpdateParams {
    CString        title;
    nlohmann::json chartConfig;
    nlohmann::json style;
    nlohmann::json position;
};

struct VizDetail {
    int            id;
    int            dashboardId;
    int            flowId;
    CString        vizType;
    CString        title;
    nlohmann::json chartConfig;
    nlohmann::json style;
    nlohmann::json position;
    CString        createdAt;
    CString        updatedAt;
};
```

---

## 5. LLMClient

libcurl 기반 LLM API HTTP 클라이언트입니다.

### 헤더: `src/llm/LLMClient.h`

```cpp
namespace DeepMetria {

enum class LLMProvider { Anthropic, OpenAI, Gemini };

struct LLMRequest {
    LLMProvider provider;
    CString     model;
    CString     systemPrompt;
    CString     userMessage;
    int         maxTokens;   // 기본값 4096
    float       temperature; // 기본값 0.0f
};

struct LLMResponse {
    BOOL    success;
    CString content;         // 응답 텍스트
    int     inputTokens;
    int     outputTokens;
    CString errorMessage;
};

class LLMClient {
public:
    static LLMClient& GetInstance();

    // 동기 호출 (워커 스레드에서 사용)
    LLMResponse Call(const LLMRequest& req);

    // API 키 설정 (app_settings에서 로드)
    void SetApiKey(LLMProvider provider, const CString& apiKey);

    // 연결 테스트
    BOOL TestConnection(LLMProvider provider, CString& outError);

    // 타임아웃 설정 (초, 기본 60)
    void SetTimeoutSeconds(long seconds);

private:
    LLMClient();
    ~LLMClient();
    CURL*   m_curl;
    long    m_timeoutSec = 60;
    std::map<LLMProvider, CString> m_apiKeys;

    LLMResponse CallAnthropic(const LLMRequest& req);
    LLMResponse CallOpenAI(const LLMRequest& req);
    LLMResponse CallGemini(const LLMRequest& req);
};

} // namespace DeepMetria
```

**libcurl 초기화 (앱 시작 시):**
```cpp
// CDeepMetriaApp::InitInstance()
curl_global_init(CURL_GLOBAL_ALL);
// 앱 종료 시
curl_global_cleanup();
```

---

## 6. 공통 타입 및 에러 코드

### 에러 코드 열거형

```cpp
namespace DeepMetria {

enum class ErrorCode : int {
    Success             = 0,
    FileNotFound        = 1001,
    UnsupportedFormat   = 1002,
    FileTooLarge        = 1003,   // 기본 한도: 200MB
    ParseError          = 1004,
    DatabaseError       = 2001,
    RecordNotFound      = 2002,
    LLMApiKeyMissing    = 3001,
    LLMRequestFailed    = 3002,
    LLMTimeout          = 3003,   // 기본 60초
    LLMQuotaExceeded    = 3004,
    UnknownError        = 9999,
};

} // namespace DeepMetria
```

### WM_DM_* 메시지 등록

```cpp
// src/common/Messages.h
#define WM_DM_DATASOURCE_READY  (WM_APP + 100)
#define WM_DM_DATASOURCE_ERROR  (WM_APP + 101)
#define WM_DM_ANALYSIS_COT_STEP (WM_APP + 110)
#define WM_DM_ANALYSIS_COMPLETE (WM_APP + 111)
#define WM_DM_ANALYSIS_ERROR    (WM_APP + 112)
```
