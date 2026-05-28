// test_dashboard_autoconfig.cpp — 대시보드 자동 구성 단위 테스트
// TC-02-04: 데이터 요약 기반 자동 대시보드 구성 검증

#include <gtest/gtest.h>
#include <windows.h>
#include <atlstr.h>

#include "Common/Types.h"
#include "Domain/Dashboard/DashboardManager.h"
#include "fixtures/TestDataFixture.h"

// ============================================================
// TC-02-04: AutoConfigLayout 테스트
// ============================================================
class DashboardAutoConfigTest : public TestDataFixture {
protected:
    DashboardManager manager_;
};

// TC-02-04-001: AutoConfigLayout이 성공을 반환한다
TEST_F(DashboardAutoConfigTest, AutoConfig_ReturnsTrue) {
    DashboardDetail dashboard;
    AppError error;

    BOOL ok = manager_.AutoConfigLayout(m_sampleSummary, dashboard, error);

    EXPECT_TRUE(ok) << "AutoConfigLayout 실패: " << (LPCWSTR)error.message;
    EXPECT_FALSE(error.IsError());
}

// TC-02-04-002: 자동 구성 결과에 시각화가 1개 이상 포함된다
TEST_F(DashboardAutoConfigTest, AutoConfig_CreatesVisualizations) {
    DashboardDetail dashboard;
    AppError error;

    manager_.AutoConfigLayout(m_sampleSummary, dashboard, error);

    EXPECT_GE(dashboard.visualizations.size(), 1u)
        << "자동 구성에 시각화가 없다";
}

// TC-02-04-003: 자동 구성 결과에 레이아웃이 포함된다
TEST_F(DashboardAutoConfigTest, AutoConfig_CreatesLayout) {
    DashboardDetail dashboard;
    AppError error;

    manager_.AutoConfigLayout(m_sampleSummary, dashboard, error);

    EXPECT_EQ(dashboard.layout.size(), dashboard.visualizations.size())
        << "레이아웃 항목 수와 시각화 수가 일치해야 한다";
}

// TC-02-04-004: 날짜 컬럼이 있으면 line 차트가 포함된다
TEST_F(DashboardAutoConfigTest, AutoConfig_DateColumn_CreatesLineChart) {
    DashboardDetail dashboard;
    AppError error;

    manager_.AutoConfigLayout(m_sampleSummary, dashboard, error);

    bool hasLineChart = false;
    for (const auto& viz : dashboard.visualizations) {
        if (viz.chartConfig.chartType == _T("line")) {
            hasLineChart = true;
            break;
        }
    }
    EXPECT_TRUE(hasLineChart)
        << "날짜 컬럼이 있으므로 line 차트가 생성되어야 한다";
}

// TC-02-04-005: 숫자 컬럼이 있으면 bar 차트가 포함된다
TEST_F(DashboardAutoConfigTest, AutoConfig_NumericColumn_CreatesBarChart) {
    DashboardDetail dashboard;
    AppError error;

    manager_.AutoConfigLayout(m_sampleSummary, dashboard, error);

    bool hasBarChart = false;
    for (const auto& viz : dashboard.visualizations) {
        if (viz.chartConfig.chartType == _T("bar")) {
            hasBarChart = true;
            break;
        }
    }
    EXPECT_TRUE(hasBarChart)
        << "숫자 컬럼이 있으므로 bar 차트가 생성되어야 한다";
}

// TC-02-04-006: 텍스트 컬럼이 있으면 pie 차트가 포함된다
TEST_F(DashboardAutoConfigTest, AutoConfig_TextColumn_CreatesPieChart) {
    DashboardDetail dashboard;
    AppError error;

    manager_.AutoConfigLayout(m_sampleSummary, dashboard, error);

    bool hasPieChart = false;
    for (const auto& viz : dashboard.visualizations) {
        if (viz.chartConfig.chartType == _T("pie")) {
            hasPieChart = true;
            break;
        }
    }
    EXPECT_TRUE(hasPieChart)
        << "텍스트 컬럼이 있으므로 pie 차트가 생성되어야 한다";
}

// TC-02-04-007: 빈 DataSummary에 대해 빈 대시보드를 반환한다
TEST_F(DashboardAutoConfigTest, AutoConfig_EmptySummary_ReturnsEmptyDashboard) {
    DataSummary empty;
    empty.rowCount = 0;
    empty.columnCount = 0;

    DashboardDetail dashboard;
    AppError error;

    BOOL ok = manager_.AutoConfigLayout(empty, dashboard, error);

    EXPECT_TRUE(ok);
    EXPECT_EQ(dashboard.visualizations.size(), 0u);
}

// TC-02-04-008: 레이아웃 항목의 위치가 겹치지 않는다
TEST_F(DashboardAutoConfigTest, AutoConfig_LayoutPositions_DoNotOverlap) {
    DashboardDetail dashboard;
    AppError error;

    manager_.AutoConfigLayout(m_sampleSummary, dashboard, error);

    for (size_t i = 0; i < dashboard.layout.size(); ++i) {
        for (size_t j = i + 1; j < dashboard.layout.size(); ++j) {
            const auto& a = dashboard.layout[i];
            const auto& b = dashboard.layout[j];

            bool xOverlap = (a.x < b.x + b.w) && (a.x + a.w > b.x);
            bool yOverlap = (a.y < b.y + b.h) && (a.y + a.h > b.y);

            EXPECT_FALSE(xOverlap && yOverlap)
                << "레이아웃 항목 " << i << "과 " << j << "이 겹친다";
        }
    }
}
