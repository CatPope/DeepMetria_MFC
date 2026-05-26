// test_chart_generation.cpp
// 통합 테스트: 차트 생성 흐름
// 분석 결과 → ChartSelector → ChartConfig → CChartRenderer 전 과정 검증

#include <gtest/gtest.h>
#include "Domain/Analysis/ChartSelector.h"
#include "Domain/Analysis/AnalysisTools.h"
#ifndef DEEPMETRIA_UNIT_TEST
#include "Renderer/ChartRenderer.h"
#endif
#include <windows.h>   // DeleteFileW (TDD: 파일 생성 검증용)

// ============================================================
// 헬퍼: 테스트용 DataTable 빌더
// ============================================================
namespace {

DataTable MakeGroupByTable() {
    DataTable t;
    t.fileName = _T("sales.csv");
    t.rowCount = 4;
    t.colCount = 2;
    t.headers.push_back(_T("category"));
    t.headers.push_back(_T("revenue"));

    auto addRow = [&](const TCHAR* cat, const TCHAR* rev) {
        DataRow r; r.push_back(cat); r.push_back(rev);
        t.rows.push_back(r);
    };
    addRow(_T("A"), _T("100"));
    addRow(_T("B"), _T("200"));
    addRow(_T("A"), _T("150"));
    addRow(_T("C"), _T("300"));

    DataColumn c1; c1.name = _T("category"); c1.type = _T("text");
    DataColumn c2; c2.name = _T("revenue");  c2.type = _T("numeric");
    t.columns.push_back(c1);
    t.columns.push_back(c2);
    return t;
}

DataTable MakeTimeSeriesTable() {
    DataTable t;
    t.fileName = _T("timeseries.csv");
    t.rowCount = 3;
    t.colCount = 2;
    t.headers.push_back(_T("date"));
    t.headers.push_back(_T("value"));

    auto addRow = [&](const TCHAR* d, const TCHAR* v) {
        DataRow r; r.push_back(d); r.push_back(v);
        t.rows.push_back(r);
    };
    addRow(_T("2024-01-01"), _T("10"));
    addRow(_T("2024-01-02"), _T("20"));
    addRow(_T("2024-01-03"), _T("15"));

    DataColumn c1; c1.name = _T("date");  c1.type = _T("date");
    DataColumn c2; c2.name = _T("value"); c2.type = _T("numeric");
    t.columns.push_back(c1);
    t.columns.push_back(c2);
    return t;
}

DataTable MakeFrequencyTable() {
    DataTable t;
    t.fileName = _T("freq.csv");
    t.rowCount = 5;
    t.colCount = 1;
    t.headers.push_back(_T("status"));

    auto addRow = [&](const TCHAR* s) {
        DataRow r; r.push_back(s);
        t.rows.push_back(r);
    };
    addRow(_T("active"));
    addRow(_T("inactive"));
    addRow(_T("active"));
    addRow(_T("pending"));
    addRow(_T("active"));

    DataColumn c1; c1.name = _T("status"); c1.type = _T("text");
    t.columns.push_back(c1);
    return t;
}

} // namespace

// ============================================================
// 테스트 픽스처
// ============================================================
class ChartGenerationTest : public ::testing::Test {
protected:
    void SetUp() override {
        groupByTable     = MakeGroupByTable();
        timeSeriesTable  = MakeTimeSeriesTable();
        frequencyTable   = MakeFrequencyTable();
    }

    DataTable groupByTable;
    DataTable timeSeriesTable;
    DataTable frequencyTable;
};

// ============================================================
// 1. BarChartFromGroupBy
// GroupByAggregate 결과는 "bar" 차트 타입으로 선택되어야 한다.
// ============================================================
TEST_F(ChartGenerationTest, BarChartFromGroupBy) {
    CString result = AnalysisTools::GroupByAggregate(
        groupByTable, _T("category"), _T("revenue"), _T("sum"));

    ChartConfig config;
    ChartSelector::SelectChart(_T("GroupByAggregate"), result, groupByTable, config);

    EXPECT_EQ(config.chartType, CString(_T("bar")))
        << "GroupByAggregate 결과의 차트 타입이 'bar' 가 아님: "
        << (LPCWSTR)config.chartType;
}

// ============================================================
// 2. LineChartFromTimeSeries
// TimeSeriesAnalysis 결과는 "line" 차트 타입으로 선택되어야 한다.
// ============================================================
TEST_F(ChartGenerationTest, LineChartFromTimeSeries) {
    CString result = AnalysisTools::TimeSeriesAnalysis(
        timeSeriesTable, _T("date"), _T("value"));

    ChartConfig config;
    ChartSelector::SelectChart(_T("TimeSeriesAnalysis"), result, timeSeriesTable, config);

    EXPECT_EQ(config.chartType, CString(_T("line")))
        << "TimeSeriesAnalysis 결과의 차트 타입이 'line' 이 아님: "
        << (LPCWSTR)config.chartType;
}

// ============================================================
// 3. PieChartFromFrequency
// FrequencyDistribution 결과는 고유값이 적을 때 "pie" 차트로 선택되어야 한다.
// ============================================================
TEST_F(ChartGenerationTest, PieChartFromFrequency) {
    CString result = AnalysisTools::FrequencyDistribution(
        frequencyTable, _T("status"));

    ChartConfig config;
    ChartSelector::SelectChart(_T("FrequencyDistribution"), result, frequencyTable, config);

    // 3개 고유값(active, inactive, pending) → pie 또는 bar 허용 (SelectFreqChartType 규칙 기반)
    EXPECT_TRUE(
        config.chartType == _T("pie") || config.chartType == _T("bar"))
        << "FrequencyDistribution 결과의 차트 타입이 'pie' 또는 'bar' 가 아님: "
        << (LPCWSTR)config.chartType;
}

// ============================================================
// 4. ChartConfigHasValidData
// SelectChart 호출 후 dataJson 필드가 유효한 JSON 형식이어야 한다.
// 최소한 '{' 또는 '[' 로 시작해야 한다.
// ============================================================
TEST_F(ChartGenerationTest, ChartConfigHasValidData) {
    CString result = AnalysisTools::GroupByAggregate(
        groupByTable, _T("category"), _T("revenue"), _T("sum"));

    ChartConfig config;
    ChartSelector::SelectChart(_T("GroupByAggregate"), result, groupByTable, config);

    ASSERT_FALSE(config.dataJson.IsEmpty())
        << "ChartConfig.dataJson 가 비어 있음";

    // JSON 시작 문자 확인 ('{' 또는 '[')
    TCHAR firstChar = config.dataJson.GetAt(0);
    EXPECT_TRUE(firstChar == _T('{') || firstChar == _T('['))
        << "dataJson 이 올바른 JSON 형식으로 시작하지 않음: "
        << (LPCWSTR)config.dataJson.Left(50);
}

// ============================================================
// 5. ChartTitleGenerated
// SelectChart 호출 후 title 필드가 비어 있지 않아야 한다.
// ============================================================
TEST_F(ChartGenerationTest, ChartTitleGenerated) {
    CString result = AnalysisTools::GroupByAggregate(
        groupByTable, _T("category"), _T("revenue"), _T("sum"));

    ChartConfig config;
    ChartSelector::SelectChart(_T("GroupByAggregate"), result, groupByTable, config);

    EXPECT_FALSE(config.title.IsEmpty())
        << "ChartConfig.title 이 비어 있음 — BuildDefaultTitle 확인 필요";
}

// ============================================================
// 6. RenderToFile  [TDD — CChartRenderer::RenderToFile 스텁 여부 확인]
// CChartRenderer::RenderToFile 이 실제 파일을 생성해야 한다.
// GDI+ 가 테스트 환경에서 초기화되지 않을 수 있으므로
// 파일 생성 성공 여부로만 검증한다.
// ============================================================
#ifndef DEEPMETRIA_UNIT_TEST
TEST_F(ChartGenerationTest, DISABLED_RenderToFile) {
    BOOL gdiOk = CChartRenderer::InitGdiplus();
    ASSERT_TRUE(gdiOk) << "GDI+ 초기화 실패 — 테스트 환경 확인 필요";

    CString result = AnalysisTools::GroupByAggregate(
        groupByTable, _T("category"), _T("revenue"), _T("sum"));
    ChartConfig config;
    ChartSelector::SelectChart(_T("GroupByAggregate"), result, groupByTable, config);

    wchar_t tempDir[MAX_PATH] = {};
    ::GetTempPathW(MAX_PATH, tempDir);
    CString outPath(tempDir);
    outPath += _T("deepmetria_test_chart.png");
    ::DeleteFileW(outPath);

    EXPECT_NO_THROW(CChartRenderer::RenderToFile(outPath, 800, 600, config));

    DWORD attr = ::GetFileAttributesW(outPath);
    EXPECT_NE(attr, INVALID_FILE_ATTRIBUTES)
        << "RenderToFile 호출 후 파일이 생성되지 않음: " << (LPCWSTR)outPath;

    ::DeleteFileW(outPath);
    CChartRenderer::ShutdownGdiplus();
}
#endif
