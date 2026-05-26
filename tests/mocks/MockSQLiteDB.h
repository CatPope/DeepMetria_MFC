#pragma once
// MockSQLiteDB.h — SQLiteDB 목 구현
//
// SQLiteDB 는 싱글턴이므로 직접 상속 목킹이 어렵다.
// ISQLiteDB 인터페이스를 도입하여 의존성 역전(DIP)을 적용한다.
// 프로덕션 코드는 ISQLiteDB& 참조를 주입받고,
// 테스트에서는 MockSQLiteDB 인스턴스를 주입한다.

#include <gmock/gmock.h>
#include "Common/Types.h"
#include <vector>

// ============================================================
// ISQLiteDB — SQLiteDB 의존성 역전 인터페이스
// ============================================================
class ISQLiteDB {
public:
    virtual ~ISQLiteDB() {}

    // ── DB 생명주기 ────────────────────────────────────────
    virtual BOOL Initialize(const CString& dbPath, AppError& outError) = 0;
    virtual void Close() = 0;

    // ── 범용 실행 ──────────────────────────────────────────
    virtual BOOL Execute(const CString& sql, AppError& outError) = 0;

    virtual BOOL Query(const CString& sql,
                       std::vector<std::vector<CString>>& outRows,
                       AppError& outError) = 0;

    // ── DataSource CRUD ────────────────────────────────────
    virtual BOOL InsertDataSource(const CString& name,
                                  const CString& filePath,
                                  const CString& fileType,
                                  long long      fileSize,
                                  AppError&      outError,
                                  int&           outId) = 0;

    virtual std::vector<DataSourceInfo> GetDataSources(AppError& outError) = 0;

    virtual BOOL DeleteDataSource(int id, AppError& outError) = 0;

    virtual BOOL UpdateDataSourceStatus(int            id,
                                        const CString& status,
                                        const CString& errorMessage,
                                        AppError&      outError) = 0;

    virtual BOOL UpdateDataSourceCounts(int id, int rowCount, int colCount,
                                        AppError& outError) = 0;

    // ── data_schemas CRUD ──────────────────────────────────
    virtual BOOL InsertDataSchema(int            datasourceId,
                                  const CString& columnName,
                                  int            columnIndex,
                                  const CString& dtype,
                                  BOOL           nullable,
                                  const CString& sampleValues,
                                  AppError&      outError) = 0;

    virtual std::vector<ColumnSchema> GetDataSchemas(int datasourceId,
                                                     AppError& outError) = 0;

    // ── data_summaries CRUD ────────────────────────────────
    virtual BOOL InsertOrReplaceSummary(int            datasourceId,
                                        const CString& domain,
                                        const CString& description,
                                        const CString& statisticsJson,
                                        int            rowCount,
                                        AppError&      outError) = 0;

    virtual DataSummary GetSummary(int datasourceId, AppError& outError) = 0;

    // ── Analysis CRUD ──────────────────────────────────────
    virtual BOOL InsertAnalysis(int            datasourceId,
                                int            dashboardId,
                                const CString& question,
                                const CString& llmProvider,
                                const CString& llmModel,
                                AppError&      outError,
                                int&           outId) = 0;

    virtual BOOL UpdateAnalysisStatus(int            flowId,
                                      const CString& status,
                                      const CString& cotStepsJson,
                                      const CString& toolCallsJson,
                                      AppError&      outError) = 0;

    virtual std::vector<AnalysisFlowInfo> GetAnalyses(int datasourceId,
                                                      AppError& outError) = 0;

    // ── Dashboard CRUD ─────────────────────────────────────
    virtual BOOL InsertDashboard(int            datasourceId,
                                 const CString& name,
                                 AppError&      outError,
                                 int&           outId) = 0;

    virtual std::vector<DashboardInfo> GetDashboards(AppError& outError) = 0;

    virtual BOOL UpdateDashboardLayout(int            dashboardId,
                                       const CString& layoutJson,
                                       AppError&      outError) = 0;

    virtual BOOL DeleteDashboard(int id, AppError& outError) = 0;

    // ── Visualization CRUD ─────────────────────────────────
    virtual BOOL InsertVisualization(int            dashboardId,
                                     int            flowId,
                                     const CString& vizType,
                                     const CString& title,
                                     const CString& chartConfigJson,
                                     const CString& styleJson,
                                     const CString& positionJson,
                                     AppError&      outError,
                                     int&           outId) = 0;

    virtual std::vector<VisualizationInfo> GetVisualizations(int dashboardId,
                                                             AppError& outError) = 0;

    virtual BOOL DeleteVisualization(int id, AppError& outError) = 0;

    // ── app_settings ───────────────────────────────────────
    virtual BOOL    SetSetting(const CString& key,
                               const CString& value,
                               AppError& outError) = 0;

    virtual CString GetSetting(const CString& key,
                               const CString& defaultValue = _T("")) = 0;
};

// ============================================================
// MockSQLiteDB — ISQLiteDB 전체 메서드 목
// ============================================================
class MockSQLiteDB : public ISQLiteDB {
public:
    // ── DB 생명주기 ────────────────────────────────────────
    MOCK_METHOD(BOOL, Initialize,
        (const CString& dbPath, AppError& outError), (override));
    MOCK_METHOD(void, Close, (), (override));

    // ── 범용 실행 ──────────────────────────────────────────
    MOCK_METHOD(BOOL, Execute,
        (const CString& sql, AppError& outError), (override));
    MOCK_METHOD(BOOL, Query,
        (const CString& sql,
         std::vector<std::vector<CString>>& outRows,
         AppError& outError),
        (override));

    // ── DataSource CRUD ────────────────────────────────────
    MOCK_METHOD(BOOL, InsertDataSource,
        (const CString& name,
         const CString& filePath,
         const CString& fileType,
         long long      fileSize,
         AppError&      outError,
         int&           outId),
        (override));
    MOCK_METHOD(std::vector<DataSourceInfo>, GetDataSources,
        (AppError& outError), (override));
    MOCK_METHOD(BOOL, DeleteDataSource,
        (int id, AppError& outError), (override));
    MOCK_METHOD(BOOL, UpdateDataSourceStatus,
        (int id, const CString& status, const CString& errorMessage,
         AppError& outError),
        (override));
    MOCK_METHOD(BOOL, UpdateDataSourceCounts,
        (int id, int rowCount, int colCount, AppError& outError), (override));

    // ── data_schemas CRUD ──────────────────────────────────
    MOCK_METHOD(BOOL, InsertDataSchema,
        (int datasourceId, const CString& columnName, int columnIndex,
         const CString& dtype, BOOL nullable, const CString& sampleValues,
         AppError& outError),
        (override));
    MOCK_METHOD(std::vector<ColumnSchema>, GetDataSchemas,
        (int datasourceId, AppError& outError), (override));

    // ── data_summaries CRUD ────────────────────────────────
    MOCK_METHOD(BOOL, InsertOrReplaceSummary,
        (int datasourceId, const CString& domain, const CString& description,
         const CString& statisticsJson, int rowCount, AppError& outError),
        (override));
    MOCK_METHOD(DataSummary, GetSummary,
        (int datasourceId, AppError& outError), (override));

    // ── Analysis CRUD ──────────────────────────────────────
    MOCK_METHOD(BOOL, InsertAnalysis,
        (int datasourceId, int dashboardId, const CString& question,
         const CString& llmProvider, const CString& llmModel,
         AppError& outError, int& outId),
        (override));
    MOCK_METHOD(BOOL, UpdateAnalysisStatus,
        (int flowId, const CString& status, const CString& cotStepsJson,
         const CString& toolCallsJson, AppError& outError),
        (override));
    MOCK_METHOD(std::vector<AnalysisFlowInfo>, GetAnalyses,
        (int datasourceId, AppError& outError), (override));

    // ── Dashboard CRUD ─────────────────────────────────────
    MOCK_METHOD(BOOL, InsertDashboard,
        (int datasourceId, const CString& name, AppError& outError, int& outId),
        (override));
    MOCK_METHOD(std::vector<DashboardInfo>, GetDashboards,
        (AppError& outError), (override));
    MOCK_METHOD(BOOL, UpdateDashboardLayout,
        (int dashboardId, const CString& layoutJson, AppError& outError),
        (override));
    MOCK_METHOD(BOOL, DeleteDashboard,
        (int id, AppError& outError), (override));

    // ── Visualization CRUD ─────────────────────────────────
    MOCK_METHOD(BOOL, InsertVisualization,
        (int dashboardId, int flowId, const CString& vizType,
         const CString& title, const CString& chartConfigJson,
         const CString& styleJson, const CString& positionJson,
         AppError& outError, int& outId),
        (override));
    MOCK_METHOD(std::vector<VisualizationInfo>, GetVisualizations,
        (int dashboardId, AppError& outError), (override));
    MOCK_METHOD(BOOL, DeleteVisualization,
        (int id, AppError& outError), (override));

    // ── app_settings ───────────────────────────────────────
    MOCK_METHOD(BOOL, SetSetting,
        (const CString& key, const CString& value, AppError& outError),
        (override));
    MOCK_METHOD(CString, GetSetting,
        (const CString& key, const CString& defaultValue), (override));

    // ── 헬퍼: Initialize 성공 기본값 설정 ─────────────────
    void SetupInitializeSuccess() {
        using ::testing::_;
        using ::testing::Return;
        ON_CALL(*this, Initialize(_, _)).WillByDefault(Return(TRUE));
    }

    // ── 헬퍼: GetSetting 기본값 반환 설정 ─────────────────
    void SetupGetSetting(const CString& key, const CString& returnValue) {
        using ::testing::_;
        using ::testing::Return;
        ON_CALL(*this, GetSetting(key, _)).WillByDefault(Return(returnValue));
    }

    // ── 헬퍼: SetSetting 성공 기본값 설정 ─────────────────
    void SetupSetSettingSuccess() {
        using ::testing::_;
        using ::testing::Return;
        ON_CALL(*this, SetSetting(_, _, _)).WillByDefault(Return(TRUE));
    }
};
