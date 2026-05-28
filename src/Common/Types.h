#pragma once

// DeepMetria 공용 타입 정의
// DetailedSpec §1.1, §9 참조

// CString, BOOL 등 MFC 기본 타입은 stdafx.h(afxwin.h)가 먼저 로드됨
// PCH 경계 이후 afxwin.h 재포함 금지 — stdafx.h에서 Types.h를 include하여 순서 보장
#include <vector>
#include <map>
#include <string>
#include <ctime>

// ============================================================
// WM_ 커스텀 메시지 상수
// ============================================================
#define WM_ANALYSIS_PROGRESS    (WM_USER + 100) // WPARAM: 진행률(0-100), LPARAM: 단계 문자열 포인터
#define WM_ANALYSIS_DONE        (WM_USER + 101) // WPARAM: flowId, LPARAM: 0
#define WM_VISUALIZATION_READY  (WM_USER + 102) // WPARAM: vizId, LPARAM: 0
#define WM_DATA_LOADED          (WM_USER + 103) // WPARAM: 0, LPARAM: DataTable 포인터
#define WM_LLM_RESPONSE         (WM_USER + 104) // WPARAM: 0, LPARAM: CString 포인터 (응답 텍스트)
#define WM_EXPORT_COMPLETE      (WM_USER + 105) // WPARAM: 성공(1)/실패(0), LPARAM: 0
#define WM_COT_STEP             (WM_USER + 106) // WPARAM: 0, LPARAM: CotStep 포인터 (수신 측에서 delete)
#define WM_IMPORT_PROGRESS      (WM_USER + 108) // WPARAM: 진행률(0-100), LPARAM: 0
#define WM_USER_QUOTA_UPDATED   (WM_USER + 109) // WPARAM: 0, LPARAM: 0
#define WM_IMPORT_COMPLETE      (WM_USER + 110) // WPARAM: datasourceId, LPARAM: 0
#define WM_ANALYSIS_NEW_QUERY   (WM_USER + 111) // WPARAM: 0, LPARAM: 0 (QueryInputView 포커스)

// ============================================================
// 공통 컨테이너 타입
// ============================================================
typedef std::vector<CString>         DataRow;
typedef std::vector<DataRow>         DataRows;
typedef std::vector<CString>         StringVector;

// ============================================================
// 오류 구조체 (DetailedSpec §1.1)
// ============================================================
struct AppError {
    CString code;       // 오류 코드 (예: "FILE_TOO_LARGE")
    CString message;    // 사용자 친화적 메시지
    int     severity;   // 0=info, 1=warning, 2=error

    AppError() : severity(0) {}
    AppError(const CString& c, const CString& m, int s = 2)
        : code(c), message(m), severity(s) {}

    // 오류 설정
    void Set(const CString& c, const CString& msg, int sev = 2) {
        code = c; message = msg; severity = sev;
    }
    // 초기화
    void Clear() { code.Empty(); message.Empty(); severity = 0; }
    // 오류 여부
    BOOL IsError() const { return severity >= 2; }
};

// ============================================================
// 데이터 테이블 구조체 (DeepMetriaDoc 보유)
// ============================================================
struct DataColumn {
    CString              name;      // 컬럼 명칭
    CString              type;      // "numeric", "text", "date"
    std::vector<CString> values;    // 셀 값 목록
};

struct DataTable {
    CString                  fileName;  // 원본 파일 경로
    std::vector<DataColumn>  columns;   // 컬럼 목록 (컬럼 기반 접근)
    int                      rowCount;  // 행 수

    // 파서(CSVParser/ExcelParser)가 채우는 행 기반 필드
    StringVector             headers;   // 헤더 행 (컬럼명 순서대로)
    DataRows                 rows;      // 데이터 행
    int                      colCount;  // 컬럼 수

    DataTable() : rowCount(0), colCount(0) {}
};

// ============================================================
// 데이터 요약 구조체
// ============================================================
struct ColumnSchema {
    CString name;
    CString type;       // "numeric", "text", "date"
    int     nullCount;
    int     index;      // 컬럼 인덱스 (0-based)
    CString sampleValues; // 콤마 구분 샘플
    int     uniqueCount;  // 고유값 수
    CString minValue;     // 최솟값 (numeric/date)
    CString maxValue;     // 최댓값 (numeric/date)

    ColumnSchema() : nullCount(0), index(0), uniqueCount(0) {}
};

struct DataSummary {
    CString                    datasourceId;
    int                        rowCount;
    int                        columnCount;
    std::vector<ColumnSchema>  schema;
    CString                    aiSummaryText; // LLM이 생성한 자연어 요약

    DataSummary() : rowCount(0), columnCount(0) {}
};

// ============================================================
// 레이아웃 / 시각화 구조체
// ============================================================
struct LayoutItem {
    CString id;     // 시각화 ID
    int     x;      // 그리드 열 위치
    int     y;      // 그리드 행 위치
    int     w;      // 너비 (그리드 단위)
    int     h;      // 높이 (그리드 단위)

    LayoutItem() : x(0), y(0), w(4), h(3) {}
};

struct ChartStyle {
    CString primaryColor;   // 기본 색상 (hex)
    CString fontFamily;     // 폰트
    int     fontSize;       // 폰트 크기
    BOOL    showLegend;
    BOOL    showGrid;

    ChartStyle() : fontSize(12), showLegend(TRUE), showGrid(TRUE) {}
};

struct ChartConfig {
    CString chartType;      // "bar", "line", "pie", "scatter"
    CString title;
    CString xLabel;
    CString yLabel;
    CString dataJson;       // 차트 데이터 (JSON 직렬화 문자열)
};

struct VisualizationInfo {
    CString     id;
    CString     dashboardId;
    CString     vizType;        // "bar_chart", "line_chart" 등
    CString     title;
    ChartConfig chartConfig;
    ChartStyle  style;
    LayoutItem  position;
};

// ============================================================
// 사용자 / 라이선스 구조체
// ============================================================
struct UserInfo {
    CString id;
    CString name;
    CString tier;       // "free", "pro", "max"
    int     usedCount;
    int     limitCount;

    UserInfo() : usedCount(0), limitCount(0) {}
};

struct QuotaInfo {
    int used;
    int limit;

    QuotaInfo() : used(0), limit(0) {}
};

// ============================================================
// 분석 플로우 구조체
// ============================================================
struct CotStep {
    CString stepType;   // "thinking", "tool_call", "tool_result", "answer"
    CString content;
    int     index;

    CotStep() : index(0) {}
};

struct AnalysisFlowInfo {
    CString             activeFlowId;
    CString             flowStatus;     // "idle|pending|running|completed|failed"
    std::vector<CotStep> cotSteps;
    BOOL                bStreaming;

    AnalysisFlowInfo() : bStreaming(FALSE) {}
};

// ============================================================
// 데이터소스 구조체
// ============================================================
struct DataSourceInfo {
    CString id;
    CString name;
    CString fileType;   // "csv", "xlsx"
    CString status;     // "importing", "ready", "error"
    int     rowCount;
    time_t  createdAt;  // Unix timestamp (CTime 대신 — afxwin.h 파싱 순서 무관)

    DataSourceInfo() : rowCount(0), createdAt(0) {}
};

struct DataSourceDetail {
    DataSourceInfo          info;
    std::vector<ColumnSchema> schema;
    DataSummary             summary;
};

struct ImportProgress {
    CString stage;      // "importing|parsing|summarizing|done"
    float   progress;   // 0.0 ~ 1.0
    CString message;

    ImportProgress() : progress(0.0f) {}
};

// ============================================================
// 대시보드 구조체
// ============================================================
struct DashboardInfo {
    CString id;
    CString name;
    CString datasourceId;
    time_t  createdAt;  // Unix timestamp (CTime 대신 — afxwin.h 파싱 순서 무관)

    DashboardInfo() : createdAt(0) {}
};

struct DashboardDetail {
    DashboardInfo                   info;
    std::vector<LayoutItem>         layout;
    std::vector<VisualizationInfo>  visualizations;
};

// ============================================================
// LLM 관련 구조체
// ============================================================
struct ChatMessage {
    CString role;       // "system" | "user" | "assistant"
    CString content;
};

struct ToolCall {
    CString name;       // 도구 이름
    CString arguments;  // JSON 문자열
};

// ============================================================
// 캐시 결과 구조체
// ============================================================
struct CachedResult {
    CString key;
    CString data;       // JSON 직렬화된 분석 결과
    CString vizType;    // 추천 시각화 타입
    time_t  createdAt;  // 생성 시각 (Unix timestamp)

    CachedResult() : createdAt(0) {}
};

// ============================================================
// 상태 관리 구조체 (DetailedSpec §9)
// ============================================================
struct DashboardState {
    DashboardDetail                         currentDashboard;
    std::map<CString, VisualizationInfo>    visualizations;
    std::vector<LayoutItem>                 layout;
    CString                                 selectedVizId;
};

struct AnalysisState {
    CString              activeFlowId;
    CString              flowStatus;    // "idle|pending|running|completed|failed"
    std::vector<CotStep> cotSteps;
    BOOL                 bStreaming;

    AnalysisState() : bStreaming(FALSE), flowStatus(_T("idle")) {}
};

struct UserState {
    UserInfo user;
    BOOL     bActivated;
    QuotaInfo quota;

    UserState() : bActivated(FALSE) {}
};

// std::hash<CString> 특수화는 stdafx.h 하단으로 이동됨
// MFC 환경에서 Types.h 안에 namespace std 재열기를 두면
// MSVC 파서가 ToolCall 구조체 컨텍스트 오염을 일으키므로 분리함
