#pragma once
// SQLiteDB.h — SQLite 데이터베이스 싱글턴 래퍼
// db-schema.md 테이블 정의 참조

#include "../../Common/Types.h"
#include <vector>

// sqlite3 forward declaration (vcpkg sqlite3 헤더)
struct sqlite3;
struct sqlite3_stmt;

// ============================================================
// SQLite DB 싱글턴
// ============================================================
class SQLiteDB {
public:
    // 싱글턴 인스턴스 획득
    static SQLiteDB& Instance();

    // DB 파일 생성/열기 + 테이블 스키마 초기화
    BOOL Initialize(const CString& dbPath, AppError& outError);

    // DB 연결 종료
    void Close();

    // ── 범용 실행 ──────────────────────────────────────────
    // INSERT/UPDATE/DELETE 등 결과 집합이 없는 SQL
    BOOL Execute(const CString& sql, AppError& outError);

    // SELECT — 결과를 행×열 문자열 2차원 배열로 반환
    BOOL Query(const CString& sql,
               std::vector<std::vector<CString>>& outRows,
               AppError& outError);

    // ── DataSource CRUD ────────────────────────────────────
    // 새 datasource 행 삽입, 생성된 id 반환
    BOOL InsertDataSource(const CString& name,
                          const CString& filePath,
                          const CString& fileType,
                          long long      fileSize,
                          AppError&      outError,
                          int&           outId);

    // datasource 목록 조회
    std::vector<DataSourceInfo> GetDataSources(AppError& outError);

    // datasource 단건 삭제 (CASCADE로 연관 행 자동 삭제)
    BOOL DeleteDataSource(int id, AppError& outError);

    // datasource status 업데이트
    BOOL UpdateDataSourceStatus(int            id,
                                const CString& status,
                                const CString& errorMessage,
                                AppError&      outError);

    // datasource row_count / column_count 업데이트
    BOOL UpdateDataSourceCounts(int id, int rowCount, int colCount, AppError& outError);

    // ── data_schemas CRUD ──────────────────────────────────
    BOOL InsertDataSchema(int            datasourceId,
                          const CString& columnName,
                          int            columnIndex,
                          const CString& dtype,
                          BOOL           nullable,
                          const CString& sampleValues,
                          AppError&      outError);

    std::vector<ColumnSchema> GetDataSchemas(int datasourceId, AppError& outError);

    // ── data_summaries CRUD ────────────────────────────────
    BOOL InsertOrReplaceSummary(int            datasourceId,
                                const CString& domain,
                                const CString& description,
                                const CString& statisticsJson,
                                int            rowCount,
                                AppError&      outError);

    DataSummary GetSummary(int datasourceId, AppError& outError);

    // ── Analysis CRUD ──────────────────────────────────────
    // analysis_flows 행 삽입, 생성된 id 반환
    BOOL InsertAnalysis(int            datasourceId,
                        int            dashboardId,    // 0 이면 NULL
                        const CString& question,
                        const CString& llmProvider,
                        const CString& llmModel,
                        AppError&      outError,
                        int&           outId);

    // analysis_flows 상태 업데이트 (cot_steps, tool_calls JSON 포함)
    BOOL UpdateAnalysisStatus(int            flowId,
                              const CString& status,
                              const CString& cotStepsJson,
                              const CString& toolCallsJson,
                              AppError&      outError);

    std::vector<AnalysisFlowInfo> GetAnalyses(int datasourceId, AppError& outError);

    // ── Dashboard CRUD ─────────────────────────────────────
    BOOL InsertDashboard(int            datasourceId,
                         const CString& name,
                         AppError&      outError,
                         int&           outId);

    std::vector<DashboardInfo> GetDashboards(AppError& outError);

    BOOL UpdateDashboardLayout(int            dashboardId,
                               const CString& layoutJson,
                               AppError&      outError);

    BOOL DeleteDashboard(int id, AppError& outError);

    // ── Visualization CRUD ─────────────────────────────────
    BOOL InsertVisualization(int            dashboardId,
                             int            flowId,        // 0 이면 NULL
                             const CString& vizType,
                             const CString& title,
                             const CString& chartConfigJson,
                             const CString& styleJson,
                             const CString& positionJson,
                             AppError&      outError,
                             int&           outId);

    std::vector<VisualizationInfo> GetVisualizations(int dashboardId, AppError& outError);

    BOOL DeleteVisualization(int id, AppError& outError);

    // ── app_settings ───────────────────────────────────────
    BOOL SetSetting(const CString& key, const CString& value, AppError& outError);
    CString GetSetting(const CString& key, const CString& defaultValue = _T(""));

    // 마지막으로 삽입된 ROWID 반환
    long long LastInsertRowId() const;

private:
    SQLiteDB();
    ~SQLiteDB();
    SQLiteDB(const SQLiteDB&) = delete;
    SQLiteDB& operator=(const SQLiteDB&) = delete;

    // 스키마 초기화 SQL 실행
    BOOL InitSchema(AppError& outError);

    // 에러 문자열을 AppError로 변환
    void SetDBError(AppError& outError, const char* szMsg, const CString& code = _T("DB_ERROR"));

    sqlite3* m_pDB;     // sqlite3 연결 핸들
    CString  m_dbPath;  // DB 파일 경로
};
