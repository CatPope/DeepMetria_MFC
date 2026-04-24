#include "stdafx.h"
#include "SQLiteDB.h"
#include <sqlite3.h>

// ============================================================
// 싱글턴
// ============================================================

SQLiteDB& SQLiteDB::Instance() {
    static SQLiteDB instance;
    return instance;
}

SQLiteDB::SQLiteDB() : m_pDB(nullptr) {}

SQLiteDB::~SQLiteDB() {
    Close();
}

// ============================================================
// 초기화 / 종료
// ============================================================

BOOL SQLiteDB::Initialize(const CString& dbPath, AppError& outError) {
    outError.Clear();
    m_dbPath = dbPath;

    // UTF-8 경로 변환
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, dbPath, -1, nullptr, 0, nullptr, nullptr);
    std::string utf8Path(utf8Len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, dbPath, -1, &utf8Path[0], utf8Len, nullptr, nullptr);
    utf8Path.resize(utf8Len - 1); // null terminator 제거

    int rc = sqlite3_open(utf8Path.c_str(), &m_pDB);
    if (rc != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_OPEN_FAILED"));
        sqlite3_close(m_pDB);
        m_pDB = nullptr;
        return FALSE;
    }

    // WAL 모드 + 외래키 활성화
    sqlite3_exec(m_pDB, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(m_pDB, "PRAGMA foreign_keys=ON;",  nullptr, nullptr, nullptr);

    return InitSchema(outError);
}

void SQLiteDB::Close() {
    if (m_pDB) {
        sqlite3_close(m_pDB);
        m_pDB = nullptr;
    }
}

// ============================================================
// 스키마 초기화
// ============================================================

BOOL SQLiteDB::InitSchema(AppError& outError) {
    const char* sql =
        // app_settings
        "CREATE TABLE IF NOT EXISTS app_settings ("
        "  key        TEXT PRIMARY KEY,"
        "  value      TEXT NOT NULL,"
        "  updated_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ");"

        // datasources
        "CREATE TABLE IF NOT EXISTS datasources ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name          TEXT    NOT NULL,"
        "  file_path     TEXT    NOT NULL,"
        "  file_type     TEXT    NOT NULL CHECK(file_type IN ('csv','excel','json')),"
        "  file_size     INTEGER NOT NULL,"
        "  row_count     INTEGER,"
        "  column_count  INTEGER,"
        "  status        TEXT    NOT NULL DEFAULT 'processing'"
        "                CHECK(status IN ('processing','ready','error')),"
        "  error_message TEXT,"
        "  created_at    TEXT    NOT NULL DEFAULT (datetime('now')),"
        "  updated_at    TEXT    NOT NULL DEFAULT (datetime('now'))"
        ");"

        // data_schemas
        "CREATE TABLE IF NOT EXISTS data_schemas ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  datasource_id INTEGER NOT NULL REFERENCES datasources(id) ON DELETE CASCADE,"
        "  column_name   TEXT    NOT NULL,"
        "  column_index  INTEGER NOT NULL,"
        "  dtype         TEXT    NOT NULL,"
        "  nullable      INTEGER NOT NULL DEFAULT 1,"
        "  sample_values TEXT,"
        "  created_at    TEXT    NOT NULL DEFAULT (datetime('now'))"
        ");"

        // data_summaries
        "CREATE TABLE IF NOT EXISTS data_summaries ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  datasource_id INTEGER NOT NULL UNIQUE REFERENCES datasources(id) ON DELETE CASCADE,"
        "  domain        TEXT,"
        "  description   TEXT,"
        "  statistics    TEXT    NOT NULL DEFAULT '{}',"
        "  row_count     INTEGER NOT NULL DEFAULT 0,"
        "  created_at    TEXT    NOT NULL DEFAULT (datetime('now')),"
        "  updated_at    TEXT    NOT NULL DEFAULT (datetime('now'))"
        ");"

        // dashboards
        "CREATE TABLE IF NOT EXISTS dashboards ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  datasource_id INTEGER NOT NULL REFERENCES datasources(id) ON DELETE CASCADE,"
        "  name          TEXT    NOT NULL,"
        "  layout        TEXT    NOT NULL DEFAULT '[]',"
        "  created_at    TEXT    NOT NULL DEFAULT (datetime('now')),"
        "  updated_at    TEXT    NOT NULL DEFAULT (datetime('now')),"
        "  deleted_at    TEXT"
        ");"

        // visualizations
        "CREATE TABLE IF NOT EXISTS visualizations ("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  dashboard_id INTEGER NOT NULL REFERENCES dashboards(id) ON DELETE CASCADE,"
        "  flow_id      INTEGER REFERENCES analysis_flows(id) ON DELETE SET NULL,"
        "  viz_type     TEXT    NOT NULL"
        "               CHECK(viz_type IN ('line','bar','pie','scatter','table',"
        "                                  'area','histogram','donut','hbar')),"
        "  title        TEXT,"
        "  chart_config TEXT    NOT NULL DEFAULT '{}',"
        "  style        TEXT    NOT NULL DEFAULT '{}',"
        "  position     TEXT    NOT NULL DEFAULT '{}',"
        "  created_at   TEXT    NOT NULL DEFAULT (datetime('now')),"
        "  updated_at   TEXT    NOT NULL DEFAULT (datetime('now'))"
        ");"

        // analysis_flows
        "CREATE TABLE IF NOT EXISTS analysis_flows ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  datasource_id INTEGER NOT NULL REFERENCES datasources(id) ON DELETE CASCADE,"
        "  dashboard_id  INTEGER REFERENCES dashboards(id) ON DELETE SET NULL,"
        "  question      TEXT    NOT NULL,"
        "  status        TEXT    NOT NULL DEFAULT 'pending'"
        "                CHECK(status IN ('pending','running','completed','failed')),"
        "  llm_provider  TEXT,"
        "  llm_model     TEXT,"
        "  cot_steps     TEXT    NOT NULL DEFAULT '[]',"
        "  tool_calls    TEXT    NOT NULL DEFAULT '[]',"
        "  created_at    TEXT    NOT NULL DEFAULT (datetime('now')),"
        "  updated_at    TEXT    NOT NULL DEFAULT (datetime('now'))"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_pDB, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        SetDBError(outError, errMsg, _T("DB_SCHEMA_FAILED"));
        sqlite3_free(errMsg);
        return FALSE;
    }
    return TRUE;
}

// ============================================================
// 범용 실행
// ============================================================

BOOL SQLiteDB::Execute(const CString& sql, AppError& outError) {
    outError.Clear();
    if (!m_pDB) {
        outError.Set(_T("DB_NOT_OPEN"), _T("데이터베이스가 열려 있지 않습니다."));
        return FALSE;
    }

    // CString → UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, sql, -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, sql, -1, &utf8[0], len, nullptr, nullptr);

    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_pDB, utf8.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        SetDBError(outError, errMsg, _T("DB_EXECUTE_FAILED"));
        sqlite3_free(errMsg);
        return FALSE;
    }
    return TRUE;
}

BOOL SQLiteDB::Query(const CString& sql,
                     std::vector<std::vector<CString>>& outRows,
                     AppError& outError) {
    outError.Clear();
    outRows.clear();
    if (!m_pDB) {
        outError.Set(_T("DB_NOT_OPEN"), _T("데이터베이스가 열려 있지 않습니다."));
        return FALSE;
    }

    int len = WideCharToMultiByte(CP_UTF8, 0, sql, -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, sql, -1, &utf8[0], len, nullptr, nullptr);

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_pDB, utf8.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_PREPARE_FAILED"));
        return FALSE;
    }

    int colCount = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::vector<CString> row;
        row.reserve(colCount);
        for (int i = 0; i < colCount; ++i) {
            const char* text = (const char*)sqlite3_column_text(stmt, i);
            if (text) {
                // UTF-8 → CString
                int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
                CString val;
                MultiByteToWideChar(CP_UTF8, 0, text, -1, val.GetBufferSetLength(wlen), wlen);
                val.ReleaseBuffer();
                row.push_back(val);
            } else {
                row.push_back(_T(""));
            }
        }
        outRows.push_back(row);
    }
    sqlite3_finalize(stmt);
    return TRUE;
}

// ============================================================
// DataSource CRUD
// ============================================================

BOOL SQLiteDB::InsertDataSource(const CString& name, const CString& filePath,
                                const CString& fileType, long long fileSize,
                                AppError& outError, int& outId) {
    outError.Clear();
    outId = 0;
    if (!m_pDB) { outError.Set(_T("DB_NOT_OPEN"), _T("DB가 열려 있지 않습니다.")); return FALSE; }

    const char* sql =
        "INSERT INTO datasources (name, file_path, file_type, file_size) VALUES (?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDB, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_PREPARE_FAILED")); return FALSE;
    }

    // 바인딩 헬퍼: CString → UTF-8
    auto bindText = [&](int idx, const CString& val) {
        int len = WideCharToMultiByte(CP_UTF8, 0, val, -1, nullptr, 0, nullptr, nullptr);
        std::string u(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, val, -1, &u[0], len, nullptr, nullptr);
        sqlite3_bind_text(stmt, idx, u.c_str(), -1, SQLITE_TRANSIENT);
    };

    bindText(1, name);
    bindText(2, filePath);
    bindText(3, fileType);
    sqlite3_bind_int64(stmt, 4, fileSize);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_INSERT_FAILED")); return FALSE;
    }
    outId = (int)sqlite3_last_insert_rowid(m_pDB);
    return TRUE;
}

std::vector<DataSourceInfo> SQLiteDB::GetDataSources(AppError& outError) {
    std::vector<DataSourceInfo> result;
    std::vector<std::vector<CString>> rows;
    if (!Query(_T("SELECT id,name,file_type,status,row_count,created_at FROM datasources ORDER BY created_at DESC;"),
               rows, outError)) return result;

    for (auto& row : rows) {
        if (row.size() < 6) continue;
        DataSourceInfo info;
        info.id        = row[0];
        info.name      = row[1];
        info.fileType  = row[2];
        info.status    = row[3];
        info.rowCount  = _ttoi(row[4]);
        info.createdAt = row[5];
        result.push_back(info);
    }
    return result;
}

BOOL SQLiteDB::DeleteDataSource(int id, AppError& outError) {
    CString sql;
    sql.Format(_T("DELETE FROM datasources WHERE id=%d;"), id);
    return Execute(sql, outError);
}

BOOL SQLiteDB::UpdateDataSourceStatus(int id, const CString& status,
                                      const CString& errorMessage, AppError& outError) {
    if (!m_pDB) { outError.Set(_T("DB_NOT_OPEN"), _T("DB가 열려 있지 않습니다.")); return FALSE; }

    const char* sql =
        "UPDATE datasources SET status=?, error_message=?, updated_at=datetime('now') WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDB, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_PREPARE_FAILED")); return FALSE;
    }

    auto toUTF8 = [](const CString& v) -> std::string {
        int len = WideCharToMultiByte(CP_UTF8, 0, v, -1, nullptr, 0, nullptr, nullptr);
        std::string u(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, v, -1, &u[0], len, nullptr, nullptr);
        return u;
    };

    std::string sStatus = toUTF8(status);
    std::string sErr    = toUTF8(errorMessage);
    sqlite3_bind_text(stmt, 1, sStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, sErr.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  3, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) { SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_UPDATE_FAILED")); return FALSE; }
    return TRUE;
}

BOOL SQLiteDB::UpdateDataSourceCounts(int id, int rowCount, int colCount, AppError& outError) {
    CString sql;
    sql.Format(_T("UPDATE datasources SET row_count=%d, column_count=%d, updated_at=datetime('now') WHERE id=%d;"),
               rowCount, colCount, id);
    return Execute(sql, outError);
}

// ============================================================
// data_schemas
// ============================================================

BOOL SQLiteDB::InsertDataSchema(int datasourceId, const CString& columnName,
                                int columnIndex, const CString& dtype,
                                BOOL nullable, const CString& sampleValues,
                                AppError& outError) {
    if (!m_pDB) { outError.Set(_T("DB_NOT_OPEN"), _T("DB가 열려 있지 않습니다.")); return FALSE; }

    const char* sql =
        "INSERT INTO data_schemas (datasource_id,column_name,column_index,dtype,nullable,sample_values)"
        " VALUES (?,?,?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDB, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_PREPARE_FAILED")); return FALSE;
    }

    auto toUTF8 = [](const CString& v) -> std::string {
        int len = WideCharToMultiByte(CP_UTF8, 0, v, -1, nullptr, 0, nullptr, nullptr);
        std::string u(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, v, -1, &u[0], len, nullptr, nullptr);
        return u;
    };

    sqlite3_bind_int(stmt,  1, datasourceId);
    std::string sName = toUTF8(columnName);
    sqlite3_bind_text(stmt, 2, sName.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  3, columnIndex);
    std::string sDtype = toUTF8(dtype);
    sqlite3_bind_text(stmt, 4, sDtype.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  5, nullable ? 1 : 0);
    std::string sSample = toUTF8(sampleValues);
    sqlite3_bind_text(stmt, 6, sSample.c_str(),-1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) { SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_INSERT_FAILED")); return FALSE; }
    return TRUE;
}

std::vector<ColumnSchema> SQLiteDB::GetDataSchemas(int datasourceId, AppError& outError) {
    std::vector<ColumnSchema> result;
    CString sql;
    sql.Format(_T("SELECT column_name,column_index,dtype,nullable,sample_values FROM data_schemas")
               _T(" WHERE datasource_id=%d ORDER BY column_index;"), datasourceId);

    std::vector<std::vector<CString>> rows;
    if (!Query(sql, rows, outError)) return result;

    for (auto& row : rows) {
        if (row.size() < 5) continue;
        ColumnSchema cs;
        cs.name         = row[0];
        cs.index        = _ttoi(row[1]);
        cs.type         = row[2];
        cs.nullCount    = 0; // DB에 저장되지 않음
        cs.sampleValues = row[4];
        result.push_back(cs);
    }
    return result;
}

// ============================================================
// data_summaries
// ============================================================

BOOL SQLiteDB::InsertOrReplaceSummary(int datasourceId, const CString& domain,
                                      const CString& description,
                                      const CString& statisticsJson,
                                      int rowCount, AppError& outError) {
    if (!m_pDB) { outError.Set(_T("DB_NOT_OPEN"), _T("DB가 열려 있지 않습니다.")); return FALSE; }

    const char* sql =
        "INSERT OR REPLACE INTO data_summaries"
        " (datasource_id,domain,description,statistics,row_count,updated_at)"
        " VALUES (?,?,?,?,?,datetime('now'));";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDB, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_PREPARE_FAILED")); return FALSE;
    }

    auto toUTF8 = [](const CString& v) -> std::string {
        int len = WideCharToMultiByte(CP_UTF8, 0, v, -1, nullptr, 0, nullptr, nullptr);
        std::string u(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, v, -1, &u[0], len, nullptr, nullptr);
        return u;
    };

    sqlite3_bind_int(stmt, 1, datasourceId);
    std::string sDomain = toUTF8(domain);
    sqlite3_bind_text(stmt, 2, sDomain.c_str(),      -1, SQLITE_TRANSIENT);
    std::string sDesc = toUTF8(description);
    sqlite3_bind_text(stmt, 3, sDesc.c_str(),         -1, SQLITE_TRANSIENT);
    std::string sStats = toUTF8(statisticsJson);
    sqlite3_bind_text(stmt, 4, sStats.c_str(),        -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, rowCount);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) { SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_INSERT_FAILED")); return FALSE; }
    return TRUE;
}

DataSummary SQLiteDB::GetSummary(int datasourceId, AppError& outError) {
    DataSummary result;
    CString sql;
    sql.Format(_T("SELECT domain,description,statistics,row_count FROM data_summaries WHERE datasource_id=%d;"),
               datasourceId);
    std::vector<std::vector<CString>> rows;
    if (!Query(sql, rows, outError) || rows.empty()) return result;

    result.aiSummaryText = rows[0].size() > 1 ? rows[0][1] : _T("");
    result.rowCount      = rows[0].size() > 3 ? _ttoi(rows[0][3]) : 0;
    return result;
}

// ============================================================
// Analysis CRUD
// ============================================================

BOOL SQLiteDB::InsertAnalysis(int datasourceId, int dashboardId,
                              const CString& question,
                              const CString& llmProvider, const CString& llmModel,
                              AppError& outError, int& outId) {
    outError.Clear(); outId = 0;
    if (!m_pDB) { outError.Set(_T("DB_NOT_OPEN"), _T("DB가 열려 있지 않습니다.")); return FALSE; }

    const char* sql =
        "INSERT INTO analysis_flows (datasource_id,dashboard_id,question,llm_provider,llm_model)"
        " VALUES (?,?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDB, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_PREPARE_FAILED")); return FALSE;
    }

    auto toUTF8 = [](const CString& v) -> std::string {
        int len = WideCharToMultiByte(CP_UTF8, 0, v, -1, nullptr, 0, nullptr, nullptr);
        std::string u(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, v, -1, &u[0], len, nullptr, nullptr);
        return u;
    };

    sqlite3_bind_int(stmt, 1, datasourceId);
    if (dashboardId > 0) sqlite3_bind_int(stmt, 2, dashboardId);
    else                 sqlite3_bind_null(stmt, 2);
    std::string sQ = toUTF8(question);
    sqlite3_bind_text(stmt, 3, sQ.c_str(),  -1, SQLITE_TRANSIENT);
    std::string sProv = toUTF8(llmProvider);
    sqlite3_bind_text(stmt, 4, sProv.c_str(),-1, SQLITE_TRANSIENT);
    std::string sMod = toUTF8(llmModel);
    sqlite3_bind_text(stmt, 5, sMod.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) { SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_INSERT_FAILED")); return FALSE; }
    outId = (int)sqlite3_last_insert_rowid(m_pDB);
    return TRUE;
}

BOOL SQLiteDB::UpdateAnalysisStatus(int flowId, const CString& status,
                                    const CString& cotStepsJson,
                                    const CString& toolCallsJson,
                                    AppError& outError) {
    if (!m_pDB) { outError.Set(_T("DB_NOT_OPEN"), _T("DB가 열려 있지 않습니다.")); return FALSE; }

    const char* sql =
        "UPDATE analysis_flows SET status=?,cot_steps=?,tool_calls=?,updated_at=datetime('now') WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDB, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_PREPARE_FAILED")); return FALSE;
    }

    auto toUTF8 = [](const CString& v) -> std::string {
        int len = WideCharToMultiByte(CP_UTF8, 0, v, -1, nullptr, 0, nullptr, nullptr);
        std::string u(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, v, -1, &u[0], len, nullptr, nullptr);
        return u;
    };

    std::string sStatus = toUTF8(status);
    std::string sCot    = toUTF8(cotStepsJson);
    std::string sTool   = toUTF8(toolCallsJson);
    sqlite3_bind_text(stmt, 1, sStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, sCot.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, sTool.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  4, flowId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) { SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_UPDATE_FAILED")); return FALSE; }
    return TRUE;
}

std::vector<AnalysisFlowInfo> SQLiteDB::GetAnalyses(int datasourceId, AppError& outError) {
    std::vector<AnalysisFlowInfo> result;
    CString sql;
    sql.Format(_T("SELECT id,status,question FROM analysis_flows WHERE datasource_id=%d ORDER BY created_at DESC;"),
               datasourceId);
    std::vector<std::vector<CString>> rows;
    if (!Query(sql, rows, outError)) return result;

    for (auto& row : rows) {
        if (row.size() < 3) continue;
        AnalysisFlowInfo info;
        info.activeFlowId = row[0];
        info.flowStatus   = row[1];
        result.push_back(info);
    }
    return result;
}

// ============================================================
// Dashboard CRUD
// ============================================================

BOOL SQLiteDB::InsertDashboard(int datasourceId, const CString& name,
                               AppError& outError, int& outId) {
    outError.Clear(); outId = 0;
    if (!m_pDB) { outError.Set(_T("DB_NOT_OPEN"), _T("DB가 열려 있지 않습니다.")); return FALSE; }

    const char* sql = "INSERT INTO dashboards (datasource_id,name) VALUES (?,?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDB, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_PREPARE_FAILED")); return FALSE;
    }

    sqlite3_bind_int(stmt, 1, datasourceId);
    int len = WideCharToMultiByte(CP_UTF8, 0, name, -1, nullptr, 0, nullptr, nullptr);
    std::string sName(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, name, -1, &sName[0], len, nullptr, nullptr);
    sqlite3_bind_text(stmt, 2, sName.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) { SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_INSERT_FAILED")); return FALSE; }
    outId = (int)sqlite3_last_insert_rowid(m_pDB);
    return TRUE;
}

std::vector<DashboardInfo> SQLiteDB::GetDashboards(AppError& outError) {
    std::vector<DashboardInfo> result;
    std::vector<std::vector<CString>> rows;
    if (!Query(_T("SELECT id,name,datasource_id,created_at FROM dashboards WHERE deleted_at IS NULL ORDER BY created_at DESC;"),
               rows, outError)) return result;

    for (auto& row : rows) {
        if (row.size() < 4) continue;
        DashboardInfo info;
        info.id           = row[0];
        info.name         = row[1];
        info.datasourceId = row[2];
        result.push_back(info);
    }
    return result;
}

BOOL SQLiteDB::UpdateDashboardLayout(int dashboardId, const CString& layoutJson,
                                     AppError& outError) {
    if (!m_pDB) { outError.Set(_T("DB_NOT_OPEN"), _T("DB가 열려 있지 않습니다.")); return FALSE; }

    const char* sql = "UPDATE dashboards SET layout=?,updated_at=datetime('now') WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDB, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_PREPARE_FAILED")); return FALSE;
    }

    int len = WideCharToMultiByte(CP_UTF8, 0, layoutJson, -1, nullptr, 0, nullptr, nullptr);
    std::string sLayout(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, layoutJson, -1, &sLayout[0], len, nullptr, nullptr);
    sqlite3_bind_text(stmt, 1, sLayout.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  2, dashboardId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) { SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_UPDATE_FAILED")); return FALSE; }
    return TRUE;
}

BOOL SQLiteDB::DeleteDashboard(int id, AppError& outError) {
    // 소프트 삭제 (deleted_at 설정)
    CString sql;
    sql.Format(_T("UPDATE dashboards SET deleted_at=datetime('now') WHERE id=%d;"), id);
    return Execute(sql, outError);
}

// ============================================================
// Visualization CRUD
// ============================================================

BOOL SQLiteDB::InsertVisualization(int dashboardId, int flowId,
                                   const CString& vizType, const CString& title,
                                   const CString& chartConfigJson,
                                   const CString& styleJson,
                                   const CString& positionJson,
                                   AppError& outError, int& outId) {
    outError.Clear(); outId = 0;
    if (!m_pDB) { outError.Set(_T("DB_NOT_OPEN"), _T("DB가 열려 있지 않습니다.")); return FALSE; }

    const char* sql =
        "INSERT INTO visualizations (dashboard_id,flow_id,viz_type,title,chart_config,style,position)"
        " VALUES (?,?,?,?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDB, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_PREPARE_FAILED")); return FALSE;
    }

    auto toUTF8 = [](const CString& v) -> std::string {
        int len = WideCharToMultiByte(CP_UTF8, 0, v, -1, nullptr, 0, nullptr, nullptr);
        std::string u(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, v, -1, &u[0], len, nullptr, nullptr);
        return u;
    };

    sqlite3_bind_int(stmt, 1, dashboardId);
    if (flowId > 0) sqlite3_bind_int(stmt, 2, flowId);
    else            sqlite3_bind_null(stmt, 2);
    std::string sType  = toUTF8(vizType);
    std::string sTitle = toUTF8(title);
    std::string sConf  = toUTF8(chartConfigJson);
    std::string sStyle = toUTF8(styleJson);
    std::string sPos   = toUTF8(positionJson);
    sqlite3_bind_text(stmt, 3, sType.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, sTitle.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, sConf.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, sStyle.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, sPos.c_str(),   -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) { SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_INSERT_FAILED")); return FALSE; }
    outId = (int)sqlite3_last_insert_rowid(m_pDB);
    return TRUE;
}

std::vector<VisualizationInfo> SQLiteDB::GetVisualizations(int dashboardId, AppError& outError) {
    std::vector<VisualizationInfo> result;
    CString sql;
    sql.Format(_T("SELECT id,dashboard_id,viz_type,title,chart_config,style,position")
               _T(" FROM visualizations WHERE dashboard_id=%d ORDER BY created_at;"), dashboardId);

    std::vector<std::vector<CString>> rows;
    if (!Query(sql, rows, outError)) return result;

    for (auto& row : rows) {
        if (row.size() < 7) continue;
        VisualizationInfo vi;
        vi.id          = row[0];
        vi.dashboardId = row[1];
        vi.vizType     = row[2];
        vi.title       = row[3];
        vi.chartConfig.dataJson = row[4]; // JSON 직렬화 문자열
        result.push_back(vi);
    }
    return result;
}

BOOL SQLiteDB::DeleteVisualization(int id, AppError& outError) {
    CString sql;
    sql.Format(_T("DELETE FROM visualizations WHERE id=%d;"), id);
    return Execute(sql, outError);
}

// ============================================================
// app_settings
// ============================================================

BOOL SQLiteDB::SetSetting(const CString& key, const CString& value, AppError& outError) {
    if (!m_pDB) { outError.Set(_T("DB_NOT_OPEN"), _T("DB가 열려 있지 않습니다.")); return FALSE; }

    const char* sql =
        "INSERT OR REPLACE INTO app_settings (key,value,updated_at) VALUES (?,?,datetime('now'));";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDB, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_PREPARE_FAILED")); return FALSE;
    }

    auto toUTF8 = [](const CString& v) -> std::string {
        int len = WideCharToMultiByte(CP_UTF8, 0, v, -1, nullptr, 0, nullptr, nullptr);
        std::string u(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, v, -1, &u[0], len, nullptr, nullptr);
        return u;
    };

    std::string sKey = toUTF8(key);
    std::string sVal = toUTF8(value);
    sqlite3_bind_text(stmt, 1, sKey.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, sVal.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) { SetDBError(outError, sqlite3_errmsg(m_pDB), _T("DB_INSERT_FAILED")); return FALSE; }
    return TRUE;
}

CString SQLiteDB::GetSetting(const CString& key, const CString& defaultValue) {
    AppError err;
    CString sql;
    sql.Format(_T("SELECT value FROM app_settings WHERE key='%s';"), (LPCTSTR)key);
    std::vector<std::vector<CString>> rows;
    if (!Query(sql, rows, err) || rows.empty()) return defaultValue;
    return rows[0][0];
}

long long SQLiteDB::LastInsertRowId() const {
    return m_pDB ? sqlite3_last_insert_rowid(m_pDB) : 0;
}

// ============================================================
// 내부 헬퍼
// ============================================================

void SQLiteDB::SetDBError(AppError& outError, const char* szMsg, const CString& code) {
    outError.code     = code;
    outError.severity = 2;
    if (szMsg) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, szMsg, -1, nullptr, 0);
        outError.message.GetBufferSetLength(wlen);
        MultiByteToWideChar(CP_UTF8, 0, szMsg, -1, outError.message.GetBuffer(wlen), wlen);
        outError.message.ReleaseBuffer();
    } else {
        outError.message = _T("알 수 없는 데이터베이스 오류");
    }
}
