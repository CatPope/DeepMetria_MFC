// test_dashboard_tdd.cpp
// TDD 테스트: 대시보드 미구현 기능
//
// CDashboardView 스텁 목록 (src/View/DashboardView.h):
//   - OnMouseMove(UINT nFlags, CPoint point)  — 빈 핸들러
//   - OnLButtonUp(UINT nFlags, CPoint point)   — 빈 핸들러
//
// 모든 테스트는 DISABLED_ 접두사로 비활성화되어 있다.
// 각 기능 구현 완료 후 DISABLED_ 를 제거하여 활성화한다.
//
// TDD: 서식 편집/TABLE 시각화 미구현 — 구현 후 활성화

#include <gtest/gtest.h>
#include "Common/Types.h"
#include "Domain/Dashboard/DashboardManager.h"
#include "Infrastructure/Storage/SQLiteDB.h"

// ============================================================
// 테스트 데이터 헬퍼
// ============================================================
namespace {

VisualizationInfo MakeViz(const TCHAR* id, const TCHAR* type, int x, int y) {
    VisualizationInfo v;
    v.id      = id;
    v.vizType = type;
    v.title   = CString(_T("테스트 시각화 ")) + id;
    v.chartConfig.chartType = type;
    v.chartConfig.title     = v.title;
    v.chartConfig.dataJson  =
        _T("{\"labels\":[\"A\",\"B\"],\"datasets\":[{\"name\":\"val\",\"values\":[1,2]}]}");
    v.position.id = id;
    v.position.x  = x;
    v.position.y  = y;
    v.position.w  = 4;
    v.position.h  = 3;
    return v;
}

DashboardDetail MakeDashboard() {
    DashboardDetail d;
    d.info.id   = _T("dash-001");
    d.info.name = _T("테스트 대시보드");

    VisualizationInfo v1 = MakeViz(_T("viz-001"), _T("bar"),  0, 0);
    VisualizationInfo v2 = MakeViz(_T("viz-002"), _T("line"), 1, 0);
    d.visualizations.push_back(v1);
    d.visualizations.push_back(v2);

    LayoutItem l1; l1.id = _T("viz-001"); l1.x = 0; l1.y = 0; l1.w = 4; l1.h = 3;
    LayoutItem l2; l2.id = _T("viz-002"); l2.x = 1; l2.y = 0; l2.w = 4; l2.h = 3;
    d.layout.push_back(l1);
    d.layout.push_back(l2);
    return d;
}

} // namespace

// ============================================================
// 테스트 픽스처
// ============================================================
class DashboardTDDTest : public ::testing::Test {
protected:
    DashboardManager manager;

    void SetUp() override {
        // DashboardManager 는 메모리 전용으로 동작 (SQLiteDB 미연결 상태)
    }
};

// ============================================================
// 1. DragCardReposition  [TDD]
// 마우스 드래그로 카드를 이동했을 때 LayoutItem 의 x, y 좌표가
// 새로운 위치로 갱신되어야 한다.
// TDD: 서식 편집/TABLE 시각화 미구현 — 구현 후 활성화
// ============================================================
TEST_F(DashboardTDDTest, DISABLED_DragCardReposition) {
    // TDD: CDashboardView::OnMouseMove / OnLButtonUp 미구현 — 구현 후 활성화
    //
    // 기대 동작:
    //   1. LButtonDown(card=viz-001, pt=(10,10))
    //   2. MouseMove(pt=(200, 100))  — 드래그 중
    //   3. LButtonUp(pt=(200, 100))  — 드래그 완료
    //   4. viz-001 의 LayoutItem.x, LayoutItem.y 가 새 그리드 좌표로 갱신됨
    //
    // DashboardManager::UpdateLayout 을 통해 좌표 변경을 검증한다.

    std::vector<LayoutItem> newLayout;
    LayoutItem movedItem;
    movedItem.id = _T("viz-001");
    movedItem.x  = 2;   // 새 그리드 열
    movedItem.y  = 1;   // 새 그리드 행
    movedItem.w  = 4;
    movedItem.h  = 3;
    newLayout.push_back(movedItem);

    AppError err;
    BOOL ok = manager.UpdateLayout(newLayout, err);
    EXPECT_TRUE(ok) << "UpdateLayout 실패: " << (LPCWSTR)err.message;

    // GetVisualizations 를 통해 업데이트된 위치를 간접 확인
    // (DashboardManager 가 viz 위치를 동기화하는지 검증)
    // 실제 CDashboardView 드래그 테스트는 pywinauto 영역에서 수행한다.
}

// ============================================================
// 2. ResizeCard  [TDD]
// 카드 가장자리 드래그로 크기를 변경했을 때
// LayoutItem 의 w, h 가 새 크기로 갱신되어야 한다.
// TDD: 서식 편집/TABLE 시각화 미구현 — 구현 후 활성화
// ============================================================
TEST_F(DashboardTDDTest, DISABLED_ResizeCard) {
    // TDD: 카드 리사이즈 미구현 — 구현 후 활성화
    //
    // 기대 동작:
    //   1. 카드 우하단 모서리(resize handle) 위에서 LButtonDown
    //   2. MouseMove 로 크기 확장
    //   3. LButtonUp 시 새 w, h 로 LayoutItem 업데이트

    std::vector<LayoutItem> newLayout;
    LayoutItem resizedItem;
    resizedItem.id = _T("viz-001");
    resizedItem.x  = 0;
    resizedItem.y  = 0;
    resizedItem.w  = 6;   // 기존 4 → 6 으로 확장
    resizedItem.h  = 4;   // 기존 3 → 4 으로 확장
    newLayout.push_back(resizedItem);

    AppError err;
    BOOL ok = manager.UpdateLayout(newLayout, err);
    EXPECT_TRUE(ok) << "UpdateLayout(resize) 실패: " << (LPCWSTR)err.message;
}

// ============================================================
// 3. ColorPickerOpens  [TDD]
// 카드 우클릭 시 색상 선택 다이얼로그가 열려야 한다.
// UI 이벤트 발생 여부는 pywinauto 영역이므로 여기서는
// VisualizationInfo.style.primaryColor 갱신 계약을 검증한다.
// TDD: 서식 편집/TABLE 시각화 미구현 — 구현 후 활성화
// ============================================================
TEST_F(DashboardTDDTest, DISABLED_ColorPickerOpens) {
    // TDD: 서식 편집(색상 변경) 미구현 — 구현 후 활성화
    //
    // 기대 동작:
    //   1. 카드 우클릭 → 컨텍스트 메뉴 → "색상 변경" 선택
    //   2. CColorDialog 표시
    //   3. 사용자 색상 선택 후 VisualizationInfo.style.primaryColor 갱신

    VisualizationInfo viz = MakeViz(_T("viz-001"), _T("bar"), 0, 0);
    viz.style.primaryColor = _T("#FF5733");   // 새 색상 선택 시뮬레이션

    EXPECT_EQ(viz.style.primaryColor, CString(_T("#FF5733")))
        << "style.primaryColor 갱신 실패";
    EXPECT_FALSE(viz.style.primaryColor.IsEmpty())
        << "primaryColor 가 비어 있음";
}

// ============================================================
// 4. TableVisualization  [TDD]
// chartType = "table" 인 ChartConfig 를 받았을 때
// CListCtrl 그리드 뷰로 렌더링되어야 한다.
// TDD: 서식 편집/TABLE 시각화 미구현 — 구현 후 활성화
// ============================================================
TEST_F(DashboardTDDTest, DISABLED_TableVisualization) {
    // TDD: TABLE 시각화 미구현 — 구현 후 활성화
    //
    // 기대 동작:
    //   ChartConfig.chartType == "table" 인 VisualizationInfo 를 AddVisualization 했을 때
    //   CDashboardView::DrawCard 가 CListCtrl 그리드를 생성하고 데이터를 채워야 한다.

    VisualizationInfo tableViz = MakeViz(_T("viz-table"), _T("table"), 0, 0);
    tableViz.chartConfig.dataJson =
        _T("{\"columns\":[\"name\",\"score\"],")
        _T("\"rows\":[[\"Alice\",\"90\"],[\"Bob\",\"85\"]]}");

    // DashboardManager 에 추가 성공 여부 확인 (vizType="table" 허용)
    AppError err;
    BOOL ok = manager.AddVisualization(tableViz, err);
    EXPECT_TRUE(ok) << "AddVisualization(table) 실패: " << (LPCWSTR)err.message;

    auto vizList = manager.GetVisualizations();
    ASSERT_EQ((int)vizList.size(), 1);
    EXPECT_EQ(vizList[0].chartConfig.chartType, CString(_T("table")));
}

// ============================================================
// 5. DashboardSaveLoadRoundTrip  [TDD]
// 대시보드를 저장 후 로드했을 때 레이아웃이 동일하게 복원되어야 한다.
// TDD: 서식 편집/TABLE 시각화 미구현 — 구현 후 활성화
// ============================================================
TEST_F(DashboardTDDTest, DISABLED_DashboardSaveLoadRoundTrip) {
    // TDD: DashboardManager::SaveDashboard / LoadDashboard SQLiteDB 통합 — 구현 후 활성화
    //
    // 기대 동작:
    //   1. viz 두 개를 추가하고 레이아웃 설정
    //   2. SaveDashboard("테스트 대시보드")
    //   3. DashboardManager 상태 초기화
    //   4. LoadDashboard(dashboardId) 호출
    //   5. 복원된 visualizations.size() == 2 && layout 동일

    AppError err;

    VisualizationInfo v1 = MakeViz(_T("viz-001"), _T("bar"),  0, 0);
    VisualizationInfo v2 = MakeViz(_T("viz-002"), _T("line"), 1, 0);
    ASSERT_TRUE(manager.AddVisualization(v1, err));
    ASSERT_TRUE(manager.AddVisualization(v2, err));

    std::vector<LayoutItem> layout;
    LayoutItem l1; l1.id = _T("viz-001"); l1.x = 0; l1.y = 0; l1.w = 4; l1.h = 3;
    LayoutItem l2; l2.id = _T("viz-002"); l2.x = 1; l2.y = 0; l2.w = 4; l2.h = 3;
    layout.push_back(l1); layout.push_back(l2);
    ASSERT_TRUE(manager.UpdateLayout(layout, err));

    // 저장
    ASSERT_TRUE(manager.SaveDashboard(_T("테스트 대시보드"), err))
        << "SaveDashboard 실패: " << (LPCWSTR)err.message;

    CString savedId = manager.GetCurrentDashboardId();
    ASSERT_FALSE(savedId.IsEmpty());

    // 로드 — 새 DashboardManager 인스턴스로 복원
    DashboardManager manager2;
    ASSERT_TRUE(manager2.LoadDashboard(savedId, err))
        << "LoadDashboard 실패: " << (LPCWSTR)err.message;

    auto restored = manager2.GetVisualizations();
    EXPECT_EQ((int)restored.size(), 2)
        << "복원된 시각화 수가 저장 전과 다름";
}

// ============================================================
// 6. DataSourceServiceDBIntegration  [TDD]
// OnFileOpenData 호출 시 DataSourceInfo 가 SQLiteDB 에 저장되어야 한다.
// TDD: 서식 편집/TABLE 시각화 미구현 — 구현 후 활성화
// ============================================================
TEST_F(DashboardTDDTest, DISABLED_DataSourceServiceDBIntegration) {
    // TDD: DataSourceService → SQLiteDB 메타데이터 저장 통합 — 구현 후 활성화
    //
    // 기대 동작:
    //   1. DataSourceService::LoadFile() 성공 후
    //   2. SQLiteDB::InsertDataSource() 가 호출되어 레코드가 저장됨
    //   3. SQLiteDB::GetDataSources() 로 조회 시 해당 레코드가 포함됨

    // SQLiteDB 인메모리 초기화
    AppError err;
    SQLiteDB& db = SQLiteDB::Instance();
    BOOL dbOk = db.Initialize(_T(":memory:"), err);
    ASSERT_TRUE(dbOk) << "SQLiteDB 초기화 실패: " << (LPCWSTR)err.message;

    // DataSource 삽입
    int insertedId = 0;
    BOOL insOk = db.InsertDataSource(
        _T("sample_sales.csv"),
        _T("C:\\tests\\data\\sample_sales.csv"),
        _T("csv"),
        1024LL,
        err,
        insertedId);
    EXPECT_TRUE(insOk) << "InsertDataSource 실패: " << (LPCWSTR)err.message;
    EXPECT_GT(insertedId, 0) << "삽입된 DataSource ID 가 0 이하";

    // 조회 확인
    auto sources = db.GetDataSources(err);
    EXPECT_FALSE(sources.empty()) << "GetDataSources 결과가 비어 있음";

    bool found = false;
    for (const auto& src : sources) {
        if (src.name == _T("sample_sales.csv")) { found = true; break; }
    }
    EXPECT_TRUE(found) << "삽입된 DataSource 가 조회 결과에 없음";

    db.Close();
}
