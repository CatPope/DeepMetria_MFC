// test_chart_renderer.cpp
// ChartConfig / VisualizationInfo / ChartStyle 구조체 단위 테스트
// TC-05-03, TC-05-04, TC-05-05
// GDI+ / 실제 렌더링 없이 데이터 구조 유효성만 검증한다.

// MFC 헤더를 Google Test보다 먼저 포함 (PCH 순서 보장)
#include <windows.h>
#include <atlstr.h>

#include <gtest/gtest.h>

#include "Common/Types.h"
#include "fixtures/TestDataFixture.h"

// ============================================================
// TC-05-03: ChartConfig 구조체 유효성 검증
// ============================================================
class ChartConfigTest : public TestDataFixture {};

// Bar 차트 — chartType, title, dataJson 필드 유효성
TEST_F(ChartConfigTest, BarChartConfig_HasValidChartType) {
    EXPECT_EQ(m_barChartConfig.chartType, CString(_T("bar")))
        << "bar 차트의 chartType은 \"bar\" 이어야 한다";
}

TEST_F(ChartConfigTest, BarChartConfig_TitleIsNotEmpty) {
    EXPECT_FALSE(m_barChartConfig.title.IsEmpty())
        << "bar 차트의 title은 비어 있으면 안 된다";
}

TEST_F(ChartConfigTest, BarChartConfig_DataJsonContainsLabels) {
    EXPECT_NE(m_barChartConfig.dataJson.Find(_T("labels")), -1)
        << "bar 차트 dataJson에 'labels' 키가 있어야 한다";
}

TEST_F(ChartConfigTest, BarChartConfig_DataJsonContainsValues) {
    EXPECT_NE(m_barChartConfig.dataJson.Find(_T("values")), -1)
        << "bar 차트 dataJson에 'values' 키가 있어야 한다";
}

// Line 차트 유효성
TEST_F(ChartConfigTest, LineChartConfig_HasValidChartType) {
    EXPECT_EQ(m_lineChartConfig.chartType, CString(_T("line")))
        << "line 차트의 chartType은 \"line\" 이어야 한다";
}

TEST_F(ChartConfigTest, LineChartConfig_DataJsonContainsLabelsAndValues) {
    EXPECT_NE(m_lineChartConfig.dataJson.Find(_T("labels")), -1)
        << "line 차트 dataJson에 'labels'가 있어야 한다";
    EXPECT_NE(m_lineChartConfig.dataJson.Find(_T("values")), -1)
        << "line 차트 dataJson에 'values'가 있어야 한다";
}

// Pie 차트 유효성
TEST_F(ChartConfigTest, PieChartConfig_HasValidChartType) {
    EXPECT_EQ(m_pieChartConfig.chartType, CString(_T("pie")))
        << "pie 차트의 chartType은 \"pie\" 이어야 한다";
}

TEST_F(ChartConfigTest, PieChartConfig_DataJsonContainsLabels) {
    EXPECT_NE(m_pieChartConfig.dataJson.Find(_T("labels")), -1)
        << "pie 차트 dataJson에 'labels'가 있어야 한다";
}

// Scatter 차트 — 직접 생성하여 유효성 확인
TEST_F(ChartConfigTest, ScatterChartConfig_CanBeConstructedManually) {
    ChartConfig scatter;
    scatter.chartType = _T("scatter");
    scatter.title     = _T("매출 vs 수량 상관관계");
    scatter.xLabel    = _T("매출");
    scatter.yLabel    = _T("수량");
    scatter.dataJson  = _T("{\"labels\":[\"A\",\"B\"],\"values\":[100,200]}");

    EXPECT_EQ(scatter.chartType, CString(_T("scatter")));
    EXPECT_FALSE(scatter.title.IsEmpty());
    EXPECT_FALSE(scatter.dataJson.IsEmpty());
    EXPECT_NE(scatter.dataJson.Find(_T("labels")), -1);
    EXPECT_NE(scatter.dataJson.Find(_T("values")), -1);
}

// xLabel / yLabel 유효성
TEST_F(ChartConfigTest, BarChartConfig_HasXLabelAndYLabel) {
    EXPECT_FALSE(m_barChartConfig.xLabel.IsEmpty())
        << "bar 차트의 xLabel은 비어 있으면 안 된다";
    EXPECT_FALSE(m_barChartConfig.yLabel.IsEmpty())
        << "bar 차트의 yLabel은 비어 있으면 안 된다";
}

TEST_F(ChartConfigTest, LineChartConfig_HasXLabelAndYLabel) {
    EXPECT_FALSE(m_lineChartConfig.xLabel.IsEmpty())
        << "line 차트의 xLabel은 비어 있으면 안 된다";
    EXPECT_FALSE(m_lineChartConfig.yLabel.IsEmpty())
        << "line 차트의 yLabel은 비어 있으면 안 된다";
}

// ============================================================
// TC-05-04: VisualizationInfo 구조체 완전성 검증
// ============================================================
TEST_F(ChartConfigTest, VisualizationInfo_IdIsNotEmpty) {
    EXPECT_FALSE(m_sampleViz.id.IsEmpty())
        << "VisualizationInfo.id는 비어 있으면 안 된다";
}

TEST_F(ChartConfigTest, VisualizationInfo_DashboardIdIsNotEmpty) {
    EXPECT_FALSE(m_sampleViz.dashboardId.IsEmpty())
        << "VisualizationInfo.dashboardId는 비어 있으면 안 된다";
}

TEST_F(ChartConfigTest, VisualizationInfo_VizTypeIsNotEmpty) {
    EXPECT_FALSE(m_sampleViz.vizType.IsEmpty())
        << "VisualizationInfo.vizType은 비어 있으면 안 된다";
}

TEST_F(ChartConfigTest, VisualizationInfo_PositionHasValidWidth) {
    EXPECT_GE(m_sampleViz.position.w, 1)
        << "VisualizationInfo.position.w는 1 이상이어야 한다";
}

TEST_F(ChartConfigTest, VisualizationInfo_PositionHasValidHeight) {
    EXPECT_GE(m_sampleViz.position.h, 1)
        << "VisualizationInfo.position.h는 1 이상이어야 한다";
}

TEST_F(ChartConfigTest, VisualizationInfo_ChartConfigTypeMatchesVizType) {
    // vizType이 "bar_chart"이면 내부 chartConfig.chartType도 "bar"이어야 함
    if (m_sampleViz.vizType == _T("bar_chart")) {
        EXPECT_EQ(m_sampleViz.chartConfig.chartType, CString(_T("bar")))
            << "vizType=bar_chart 이면 chartConfig.chartType은 bar여야 한다";
    }
}

// ============================================================
// TC-05-05: ChartStyle 기본값 검증
// ============================================================
class ChartStyleTest : public ::testing::Test {};

TEST_F(ChartStyleTest, DefaultStyle_FontSizeIs12) {
    ChartStyle style;
    EXPECT_EQ(style.fontSize, 12)
        << "ChartStyle 기본 fontSize는 12이어야 한다";
}

TEST_F(ChartStyleTest, DefaultStyle_ShowLegendIsTrue) {
    ChartStyle style;
    EXPECT_EQ(style.showLegend, TRUE)
        << "ChartStyle 기본 showLegend는 TRUE이어야 한다";
}

TEST_F(ChartStyleTest, DefaultStyle_ShowGridIsTrue) {
    ChartStyle style;
    EXPECT_EQ(style.showGrid, TRUE)
        << "ChartStyle 기본 showGrid는 TRUE이어야 한다";
}

// 컬러 팔레트 — VisualizationInfo 내 스타일 검증
TEST_F(ChartConfigTest, VisualizationInfo_StylePrimaryColorIsNotEmpty) {
    EXPECT_FALSE(m_sampleViz.style.primaryColor.IsEmpty())
        << "VisualizationInfo.style.primaryColor는 비어 있으면 안 된다";
}

TEST_F(ChartConfigTest, VisualizationInfo_StyleFontFamilyIsNotEmpty) {
    EXPECT_FALSE(m_sampleViz.style.fontFamily.IsEmpty())
        << "VisualizationInfo.style.fontFamily는 비어 있으면 안 된다";
}

TEST_F(ChartConfigTest, VisualizationInfo_StyleFontSizeIsPositive) {
    EXPECT_GT(m_sampleViz.style.fontSize, 0)
        << "VisualizationInfo.style.fontSize는 양수여야 한다";
}

// LayoutItem 기본값 검증
TEST_F(ChartStyleTest, LayoutItemDefaults_HasSensibleValues) {
    LayoutItem item;
    EXPECT_GE(item.w, 1) << "LayoutItem 기본 w는 1 이상이어야 한다";
    EXPECT_GE(item.h, 1) << "LayoutItem 기본 h는 1 이상이어야 한다";
}

// ChartConfig 기본 생성자 — 빈 문자열로 초기화됨을 확인
TEST_F(ChartStyleTest, ChartConfigDefault_AllFieldsAreEmpty) {
    ChartConfig cfg;
    EXPECT_TRUE(cfg.chartType.IsEmpty());
    EXPECT_TRUE(cfg.title.IsEmpty());
    EXPECT_TRUE(cfg.xLabel.IsEmpty());
    EXPECT_TRUE(cfg.yLabel.IsEmpty());
    EXPECT_TRUE(cfg.dataJson.IsEmpty());
}
