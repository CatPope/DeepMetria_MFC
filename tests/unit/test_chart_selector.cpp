// test_chart_selector.cpp
// ChartSelector 단위 테스트
// Google Test 기반, 규칙 기반 차트 유형 선택 로직 검증

#include <windows.h>
#include <atlstr.h>

#include <gtest/gtest.h>

#include "Domain/Analysis/ChartSelector.h"

// ============================================================
// 공통 픽스처 — 최소한의 DataTable (파일명만 사용됨)
// ============================================================
class ChartSelectorTest : public ::testing::Test {
protected:
    DataTable data;

    void SetUp() override {
        data.fileName = _T("sample_sales.csv");
        data.rowCount = 5;

        // ChartSelector는 data.fileName 이외에는 columns를 직접 참조하지 않으나
        // 실제 서명대로 DataTable을 전달해야 하므로 최소 컬럼 1개 구성
        DataColumn col;
        col.name = _T("매출");
        col.type = _T("numeric");
        col.values = { _T("100"), _T("200"), _T("300"), _T("400"), _T("500") };
        data.columns = { col };
        data.colCount = 1;
    }

    // 헬퍼: SelectChart 호출 후 chartType 반환
    CString GetChartType(const CString& toolName,
                         const CString& analysisResult = _T("{}")) {
        ChartConfig cfg;
        ChartSelector::SelectChart(toolName, analysisResult, data, cfg);
        return cfg.chartType;
    }

    // 헬퍼: SelectChart 호출 후 ChartConfig 전체 반환
    ChartConfig GetConfig(const CString& toolName,
                          const CString& analysisResult = _T("{}")) {
        ChartConfig cfg;
        ChartSelector::SelectChart(toolName, analysisResult, data, cfg);
        return cfg;
    }
};

// ============================================================
// 1. BasicStats → bar 차트
// ============================================================
TEST_F(ChartSelectorTest, BasicStats_SelectsBarChart) {
    CString type = GetChartType(_T("BasicStats"));
    EXPECT_EQ(type, CString(_T("bar")));
}

TEST_F(ChartSelectorTest, BasicStats_ConfigHasTitleAndLabels) {
    ChartConfig cfg = GetConfig(_T("BasicStats"),
        _T("{\"stats\":[{\"column\":\"매출\",\"mean\":100.0}]}"));

    EXPECT_FALSE(cfg.title.IsEmpty())  << "title이 비어 있음";
    EXPECT_EQ(cfg.chartType, CString(_T("bar")));
    // bar 차트이면 xLabel/yLabel이 설정되어야 함
    EXPECT_FALSE(cfg.xLabel.IsEmpty()) << "xLabel이 비어 있음";
    EXPECT_FALSE(cfg.yLabel.IsEmpty()) << "yLabel이 비어 있음";
}

// ============================================================
// 2. TimeSeriesAnalysis → line 차트
// ============================================================
TEST_F(ChartSelectorTest, TimeSeriesAnalysis_SelectsLineChart) {
    CString type = GetChartType(_T("TimeSeriesAnalysis"));
    EXPECT_EQ(type, CString(_T("line")));
}

TEST_F(ChartSelectorTest, TimeSeriesAnalysis_ConfigHasLineLabels) {
    ChartConfig cfg = GetConfig(_T("TimeSeriesAnalysis"));

    EXPECT_EQ(cfg.chartType, CString(_T("line")));
    EXPECT_FALSE(cfg.xLabel.IsEmpty());
    EXPECT_FALSE(cfg.yLabel.IsEmpty());
}

// ============================================================
// 3. FrequencyDistribution — 항목 5개 이하 → pie
// ============================================================
TEST_F(ChartSelectorTest, FrequencyDistribution_FewItems_SelectsPieChart) {
    // total_unique:3 → pie
    CString analysisResult =
        _T("{\"column\":\"카테고리\",\"total_unique\":3,\"items\":[")
        _T("{\"value\":\"전자\",\"count\":4},")
        _T("{\"value\":\"의류\",\"count\":3},")
        _T("{\"value\":\"식품\",\"count\":3}]}");

    CString type = GetChartType(_T("FrequencyDistribution"), analysisResult);
    EXPECT_EQ(type, CString(_T("pie")));
}

// ============================================================
// 4. FrequencyDistribution — 항목 6개 이상 → bar
// ============================================================
TEST_F(ChartSelectorTest, FrequencyDistribution_ManyItems_SelectsBarChart) {
    // total_unique:6 → bar
    CString analysisResult =
        _T("{\"column\":\"지역\",\"total_unique\":6,\"items\":[")
        _T("{\"value\":\"A\",\"count\":2},{\"value\":\"B\",\"count\":2},")
        _T("{\"value\":\"C\",\"count\":2},{\"value\":\"D\",\"count\":2},")
        _T("{\"value\":\"E\",\"count\":1},{\"value\":\"F\",\"count\":1}]}");

    CString type = GetChartType(_T("FrequencyDistribution"), analysisResult);
    EXPECT_EQ(type, CString(_T("bar")));
}

TEST_F(ChartSelectorTest, FrequencyDistribution_ExactlyFiveItems_SelectsPieChart) {
    // 경계값: total_unique:5 → pie (5 이하)
    CString analysisResult =
        _T("{\"column\":\"지역\",\"total_unique\":5,\"items\":[]}");

    CString type = GetChartType(_T("FrequencyDistribution"), analysisResult);
    EXPECT_EQ(type, CString(_T("pie")));
}

// ============================================================
// 5. CorrelationMatrix → scatter 차트
// ============================================================
TEST_F(ChartSelectorTest, CorrelationMatrix_SelectsScatterChart) {
    CString type = GetChartType(_T("CorrelationMatrix"));
    EXPECT_EQ(type, CString(_T("scatter")));
}

TEST_F(ChartSelectorTest, CorrelationMatrix_ConfigHasScatterLabels) {
    ChartConfig cfg = GetConfig(_T("CorrelationMatrix"));

    EXPECT_EQ(cfg.chartType, CString(_T("scatter")));
    // scatter 차트이면 x/y 레이블이 "X", "Y"로 설정됨
    EXPECT_EQ(cfg.xLabel, CString(_T("X")));
    EXPECT_EQ(cfg.yLabel, CString(_T("Y")));
}

// ============================================================
// 6. GroupByAggregate → bar 차트
// ============================================================
TEST_F(ChartSelectorTest, GroupByAggregate_SelectsBarChart) {
    CString type = GetChartType(_T("GroupByAggregate"));
    EXPECT_EQ(type, CString(_T("bar")));
}

// ============================================================
// 7. MovingAverage → line 차트
// ============================================================
TEST_F(ChartSelectorTest, MovingAverage_SelectsLineChart) {
    CString type = GetChartType(_T("MovingAverage"));
    EXPECT_EQ(type, CString(_T("line")));
}

// ============================================================
// 8. UnknownTool → 기본값 bar 차트
// ============================================================
TEST_F(ChartSelectorTest, UnknownTool_DefaultsToBarChart) {
    CString type = GetChartType(_T("NonExistentTool"));
    EXPECT_EQ(type, CString(_T("bar")));
}

TEST_F(ChartSelectorTest, EmptyToolName_DefaultsToBarChart) {
    CString type = GetChartType(_T(""));
    EXPECT_EQ(type, CString(_T("bar")));
}

// ============================================================
// 9. 추가 도구 차트 유형 검증
// ============================================================
TEST_F(ChartSelectorTest, CrossTabulation_SelectsBarChart) {
    CString type = GetChartType(_T("CrossTabulation"));
    EXPECT_EQ(type, CString(_T("bar")));
}

TEST_F(ChartSelectorTest, PivotTable_SelectsBarChart) {
    CString type = GetChartType(_T("PivotTable"));
    EXPECT_EQ(type, CString(_T("bar")));
}

TEST_F(ChartSelectorTest, TopN_SelectsBarChart) {
    CString type = GetChartType(_T("TopN"));
    EXPECT_EQ(type, CString(_T("bar")));
}

TEST_F(ChartSelectorTest, Percentile_SelectsBarChart) {
    CString type = GetChartType(_T("Percentile"));
    EXPECT_EQ(type, CString(_T("bar")));
}

TEST_F(ChartSelectorTest, OutlierDetection_SelectsScatterChart) {
    CString type = GetChartType(_T("OutlierDetection"));
    EXPECT_EQ(type, CString(_T("scatter")));
}

TEST_F(ChartSelectorTest, DateGroupAggregate_SelectsLineChart) {
    CString type = GetChartType(_T("DateGroupAggregate"));
    EXPECT_EQ(type, CString(_T("line")));
}

// ============================================================
// 10. dataJson 필드가 비어 있지 않음을 검증
// ============================================================
TEST_F(ChartSelectorTest, SelectChart_AlwaysFillsDataJson) {
    // BasicStats 결과로 dataJson이 채워지는지 확인
    CString analysisResult =
        _T("{\"stats\":[{\"column\":\"매출\",\"count\":5,\"null_count\":0,")
        _T("\"mean\":300.0000,\"median\":300.0000,\"std\":158.1139,")
        _T("\"min\":100.0000,\"max\":500.0000}]}");

    ChartConfig cfg;
    ChartSelector::SelectChart(_T("BasicStats"), analysisResult, data, cfg);

    // dataJson은 변환 결과이므로 비어 있지 않아야 함
    EXPECT_FALSE(cfg.dataJson.IsEmpty()) << "dataJson이 비어 있음";
}

TEST_F(ChartSelectorTest, SelectChart_TitleIsNotEmpty) {
    // 알려진 도구명이면 기본 제목이 설정되어야 함
    ChartConfig cfg = GetConfig(_T("BasicStats"));
    EXPECT_EQ(cfg.title, CString(_T("기본 통계")));
}

TEST_F(ChartSelectorTest, SelectChart_UnknownTool_TitleIsToolName) {
    // 알 수 없는 도구명이면 도구명 자체가 제목
    ChartConfig cfg = GetConfig(_T("MyCustomTool"));
    EXPECT_EQ(cfg.title, CString(_T("MyCustomTool")));
}
