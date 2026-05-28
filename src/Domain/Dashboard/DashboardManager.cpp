#include "stdafx.h"
#include "DashboardManager.h"

// Infrastructure 레이어 — .cpp에서만 include
#include "../../Infrastructure/Storage/SQLiteDB.h"

// ============================================================
// DashboardManager 구현
// Architecture §3 / DetailedSpec §1.5, §6 참조
// ============================================================

DashboardManager::DashboardManager()
{
}

DashboardManager::~DashboardManager()
{
}

BOOL DashboardManager::AddVisualization(const VisualizationInfo& viz,
                                         AppError&                outError)
{
    // 메모리에 추가
    m_visualizations.push_back(viz);

    // 레이아웃에 위치 정보 추가
    m_layout.push_back(viz.position);

    // SQLiteDB에 저장
    SQLiteDB& db = SQLiteDB::Instance();
    (void)outError; // Initialize는 앱 시작 시 완료됨

    CString vizJson = SerializeVizInfo(viz);

    // SQL 인젝션 방지: 문자열 값의 작은따옴표를 '' 로 이스케이프
    auto EscapeSql = [](const CString& s) -> CString {
        CString r = s;
        r.Replace(_T("'"), _T("''"));
        return r;
    };

    CString sql;
    sql.Format(
        _T("INSERT OR REPLACE INTO visualizations (id, dashboard_id, viz_type, title, chart_config, position_x, position_y, position_w, position_h) ")
        _T("VALUES ('%s', '%s', '%s', '%s', '%s', %d, %d, %d, %d)"),
        (LPCTSTR)EscapeSql(viz.id),
        (LPCTSTR)EscapeSql(viz.dashboardId),
        (LPCTSTR)EscapeSql(viz.vizType),
        (LPCTSTR)EscapeSql(viz.title),
        (LPCTSTR)EscapeSql(viz.chartConfig.dataJson),
        viz.position.x, viz.position.y,
        viz.position.w, viz.position.h
    );

    return db.Execute(sql, outError);
}

BOOL DashboardManager::RemoveVisualization(const CString& vizId,
                                            AppError&      outError)
{
    // 메모리에서 제거
    auto it = std::find_if(m_visualizations.begin(), m_visualizations.end(),
        [&vizId](const VisualizationInfo& v) { return v.id == vizId; });
    if (it != m_visualizations.end())
        m_visualizations.erase(it);

    // 레이아웃에서 제거
    auto lit = std::find_if(m_layout.begin(), m_layout.end(),
        [&vizId](const LayoutItem& l) { return l.id == vizId; });
    if (lit != m_layout.end())
        m_layout.erase(lit);

    // DB에서 제거
    SQLiteDB& db = SQLiteDB::Instance();
    (void)outError; // Initialize는 앱 시작 시 완료됨

    // SQL 인젝션 방지: 작은따옴표 이스케이프
    CString safeVizId = vizId;
    safeVizId.Replace(_T("'"), _T("''"));

    CString sql;
    sql.Format(_T("DELETE FROM visualizations WHERE id = '%s'"), (LPCTSTR)safeVizId);
    return db.Execute(sql, outError);
}

std::vector<VisualizationInfo> DashboardManager::GetVisualizations() const
{
    return m_visualizations;
}

BOOL DashboardManager::SaveDashboard(const CString& name, AppError& outError)
{
    SQLiteDB& db = SQLiteDB::Instance();
    (void)outError; // Initialize는 앱 시작 시 완료됨

    CString layoutJson = SerializeLayout(m_layout);

    // SQL 인젝션 방지: 작은따옴표 이스케이프
    auto EscapeSql = [](const CString& s) -> CString {
        CString r = s;
        r.Replace(_T("'"), _T("''"));
        return r;
    };

    CString sql;
    if (m_currentDashboardId.IsEmpty()) {
        // 신규 대시보드 생성
        sql.Format(
            _T("INSERT INTO dashboards (name, layout) VALUES ('%s', '%s')"),
            (LPCTSTR)EscapeSql(name), (LPCTSTR)EscapeSql(layoutJson)
        );
        if (!db.Execute(sql, outError))
            return FALSE;

        // 생성된 ID 조회
        CString idSql = _T("SELECT last_insert_rowid()");
        std::vector<std::vector<CString>> idRows;
        if (db.Query(idSql, idRows, outError) && !idRows.empty() && !idRows[0].empty())
            m_currentDashboardId = idRows[0][0];
    } else {
        // 기존 대시보드 업데이트
        sql.Format(
            _T("UPDATE dashboards SET name = '%s', layout = '%s' WHERE id = '%s'"),
            (LPCTSTR)EscapeSql(name), (LPCTSTR)EscapeSql(layoutJson),
            (LPCTSTR)EscapeSql(m_currentDashboardId)
        );
        if (!db.Execute(sql, outError))
            return FALSE;
    }

    return TRUE;
}

BOOL DashboardManager::LoadDashboard(const CString& dashboardId,
                                      AppError&      outError)
{
    SQLiteDB& db = SQLiteDB::Instance();
    (void)outError; // Initialize는 앱 시작 시 완료됨

    // SQL 인젝션 방지: 작은따옴표 이스케이프
    CString safeDashboardId = dashboardId;
    safeDashboardId.Replace(_T("'"), _T("''"));

    // 대시보드 기본 정보 로드
    CString sql;
    sql.Format(_T("SELECT layout FROM dashboards WHERE id = '%s'"),
               (LPCTSTR)safeDashboardId);

    std::vector<std::vector<CString>> layoutResult;
    CString layoutJson;
    if (db.Query(sql, layoutResult, outError) && !layoutResult.empty() && !layoutResult[0].empty())
        layoutJson = layoutResult[0][0];
    if (layoutJson.IsEmpty()) {
        outError = AppError(_T("DASHBOARD_NOT_FOUND"),
                            _T("대시보드를 찾을 수 없습니다."), 2);
        return FALSE;
    }

    if (!DeserializeLayout(layoutJson, m_layout)) {
        outError = AppError(_T("LAYOUT_PARSE_ERROR"),
                            _T("레이아웃 데이터를 파싱할 수 없습니다."), 1);
        // 레이아웃 파싱 실패는 치명적이지 않음 — 빈 레이아웃으로 진행
        m_layout.clear();
    }

    // 시각화 목록 로드
    m_visualizations.clear();
    sql.Format(
        _T("SELECT id, viz_type, title, chart_config, position_x, position_y, position_w, position_h ")
        _T("FROM visualizations WHERE dashboard_id = '%s' ORDER BY rowid"),
        (LPCTSTR)safeDashboardId
    );

    std::vector<std::vector<CString>> rows;
    db.Query(sql, rows, outError);
    for (const auto& row : rows) {
        if (row.size() < 8) continue;
        VisualizationInfo viz;
        viz.id              = row[0];
        viz.dashboardId     = dashboardId;
        viz.vizType         = row[1];
        viz.title           = row[2];
        viz.chartConfig.dataJson = row[3];
        viz.position.id     = row[0];
        viz.position.x      = _ttoi(row[4]);
        viz.position.y      = _ttoi(row[5]);
        viz.position.w      = _ttoi(row[6]);
        viz.position.h      = _ttoi(row[7]);
        m_visualizations.push_back(viz);
    }

    m_currentDashboardId = dashboardId;
    return TRUE;
}

BOOL DashboardManager::UpdateLayout(const std::vector<LayoutItem>& layout,
                                     AppError&                      outError)
{
    m_layout = layout;

    if (m_currentDashboardId.IsEmpty()) {
        outError = AppError(_T("NO_DASHBOARD"),
                            _T("현재 로드된 대시보드가 없습니다."), 1);
        return FALSE;
    }

    SQLiteDB& db = SQLiteDB::Instance();
    (void)outError; // Initialize는 앱 시작 시 완료됨

    CString layoutJson = SerializeLayout(layout);

    // SQL 인젝션 방지: 작은따옴표 이스케이프
    CString safeLayout = layoutJson;
    safeLayout.Replace(_T("'"), _T("''"));
    CString safeDashId = m_currentDashboardId;
    safeDashId.Replace(_T("'"), _T("''"));

    CString sql;
    sql.Format(_T("UPDATE dashboards SET layout = '%s' WHERE id = '%s'"),
               (LPCTSTR)safeLayout, (LPCTSTR)safeDashId);

    return db.Execute(sql, outError);
}

std::vector<DashboardInfo> DashboardManager::GetDashboardList(AppError& outError)
{
    std::vector<DashboardInfo> list;

    SQLiteDB& db = SQLiteDB::Instance();

    CString sql = _T("SELECT id, name, datasource_id FROM dashboards ORDER BY rowid DESC");
    std::vector<std::vector<CString>> rows;
    db.Query(sql, rows, outError);

    for (const auto& row : rows) {
        if (row.size() < 3) continue;
        DashboardInfo info;
        info.id           = row[0];
        info.name         = row[1];
        info.datasourceId = row[2];
        list.push_back(info);
    }

    return list;
}

BOOL DashboardManager::RemoveDashboard(const CString& dashboardId,
                                        AppError&      outError)
{
    SQLiteDB& db = SQLiteDB::Instance();
    (void)outError; // Initialize는 앱 시작 시 완료됨

    // SQL 인젝션 방지: 작은따옴표 이스케이프
    CString safeDashboardId = dashboardId;
    safeDashboardId.Replace(_T("'"), _T("''"));

    // 연관 시각화 먼저 삭제
    CString sql;
    sql.Format(_T("DELETE FROM visualizations WHERE dashboard_id = '%s'"),
               (LPCTSTR)safeDashboardId);
    if (!db.Execute(sql, outError))
        return FALSE;

    // 대시보드 삭제
    sql.Format(_T("DELETE FROM dashboards WHERE id = '%s'"), (LPCTSTR)safeDashboardId);
    if (!db.Execute(sql, outError))
        return FALSE;

    // 현재 로드된 대시보드이면 상태 초기화
    if (m_currentDashboardId == dashboardId) {
        m_currentDashboardId.Empty();
        m_visualizations.clear();
        m_layout.clear();
    }

    return TRUE;
}

// ============================================================
// 직렬화 / 역직렬화 헬퍼
// ============================================================

CString DashboardManager::SerializeLayout(const std::vector<LayoutItem>& layout)
{
    // 간단한 JSON 배열 직렬화
    CString json = _T("[");
    for (size_t i = 0; i < layout.size(); ++i) {
        if (i > 0) json += _T(",");
        const LayoutItem& item = layout[i];
        CString entry;
        entry.Format(_T("{\"i\":\"%s\",\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d}"),
                     (LPCTSTR)item.id, item.x, item.y, item.w, item.h);
        json += entry;
    }
    json += _T("]");
    return json;
}

BOOL DashboardManager::DeserializeLayout(const CString&          json,
                                          std::vector<LayoutItem>& outLayout)
{
    outLayout.clear();
    if (json.IsEmpty() || json[0] != _T('[')) return FALSE;

    // 각 객체 {…} 블록을 순서대로 파싱
    int pos = 1;
    int len = json.GetLength();

    while (pos < len) {
        // '{' 찾기
        int brace = json.Find(_T('{'), pos);
        if (brace < 0) break;
        int end = json.Find(_T('}'), brace);
        if (end < 0) break;

        CString block = json.Mid(brace + 1, end - brace - 1);
        LayoutItem item;

        // 간단한 키-값 파싱 (형식: "key":value 또는 "key":"value")
        auto parseStrVal = [&](const CString& key) -> CString {
            CString search = _T("\"") + key + _T("\":\"");
            int p = block.Find(search);
            if (p < 0) return _T("");
            p += search.GetLength();
            int q = block.Find(_T("\""), p);
            return (q >= 0) ? block.Mid(p, q - p) : _T("");
        };
        auto parseIntVal = [&](const CString& key) -> int {
            CString search = _T("\"") + key + _T("\":");
            int p = block.Find(search);
            if (p < 0) return 0;
            p += search.GetLength();
            // 문자열 값이면 건너뜀
            if (p < block.GetLength() && block[p] == _T('"')) return 0;
            return _ttoi(block.Mid(p));
        };

        item.id = parseStrVal(_T("i"));
        item.x  = parseIntVal(_T("x"));
        item.y  = parseIntVal(_T("y"));
        item.w  = parseIntVal(_T("w"));
        item.h  = parseIntVal(_T("h"));

        outLayout.push_back(item);
        pos = end + 1;
    }

    return TRUE;
}

// ============================================================
// AutoConfigLayout — 데이터 요약 기반 자동 대시보드 구성
// ============================================================

BOOL DashboardManager::AutoConfigLayout(const DataSummary& summary,
                                         DashboardDetail&   outDashboard,
                                         AppError&          outError)
{
    (void)outError;

    outDashboard.visualizations.clear();
    outDashboard.layout.clear();

    int gridX = 0;
    int gridY = 0;

    for (const auto& col : summary.schema) {
        VisualizationInfo viz;

        // 고유 ID 생성 (컬럼 인덱스 기반)
        CString vizId;
        vizId.Format(_T("auto_viz_%d"), col.index);
        viz.id          = vizId;
        viz.dashboardId = summary.datasourceId;
        viz.title       = col.name;

        // 컬럼 타입에 따라 차트 타입 결정
        if (col.type == _T("date")) {
            viz.vizType              = _T("line_chart");
            viz.chartConfig.chartType = _T("line");
        } else if (col.type == _T("numeric")) {
            viz.vizType              = _T("bar_chart");
            viz.chartConfig.chartType = _T("bar");
        } else {
            // text — 범주형: 파이 차트
            viz.vizType              = _T("pie_chart");
            viz.chartConfig.chartType = _T("pie");
        }

        viz.chartConfig.title  = col.name;
        viz.chartConfig.xLabel = col.name;
        viz.chartConfig.yLabel = _T("값");

        // 그리드 배치 (4열 기준, 각 카드 너비 4, 높이 3)
        LayoutItem pos;
        pos.id = vizId;
        pos.x  = gridX;
        pos.y  = gridY;
        pos.w  = 4;
        pos.h  = 3;
        viz.position = pos;

        outDashboard.visualizations.push_back(viz);
        outDashboard.layout.push_back(pos);

        // 다음 위치 계산 (2열 배치)
        gridX += 4;
        if (gridX >= 8) {
            gridX = 0;
            gridY += 3;
        }
    }

    outDashboard.info.datasourceId = summary.datasourceId;

    return TRUE;
}

CString DashboardManager::SerializeVizInfo(const VisualizationInfo& viz)
{
    CString json;
    json.Format(
        _T("{\"id\":\"%s\",\"type\":\"%s\",\"title\":\"%s\","
           "\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d}"),
        (LPCTSTR)viz.id, (LPCTSTR)viz.vizType, (LPCTSTR)viz.title,
        viz.position.x, viz.position.y,
        viz.position.w, viz.position.h
    );
    return json;
}
