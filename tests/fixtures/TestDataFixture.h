#pragma once
// TestDataFixture.h — 테스트용 샘플 데이터 픽스처
//
// sample_sales.csv 기반의 현실적인 한국 비즈니스 데이터를 제공한다.
// 컬럼 구성: 날짜(date), 카테고리(text), 매출(numeric), 수량(numeric), 지역(text)
// 행 수: 10행 (sample_sales.csv 와 동일)

#include <gtest/gtest.h>
#include "Common/Types.h"
#include <string>

// ============================================================
// TestDataFixture — Google Test Fixture 기반 클래스
// 각 테스트 케이스가 상속하여 샘플 데이터에 접근한다.
// ============================================================
class TestDataFixture : public ::testing::Test {
protected:
    // ── 생명주기 ──────────────────────────────────────────
    void SetUp() override {
        m_sampleTable    = BuildSampleDataTable();
        m_sampleSummary  = BuildSampleDataSummary();
        m_barChartConfig = BuildBarChartConfig();
        m_lineChartConfig = BuildLineChartConfig();
        m_pieChartConfig = BuildPieChartConfig();
        m_sampleViz      = BuildSampleVisualizationInfo();
    }

    void TearDown() override {
        // DataTable, DataSummary 등은 값 타입이므로 별도 해제 불필요
    }

    // ── 공개 샘플 데이터 ───────────────────────────────────
    DataTable         m_sampleTable;
    DataSummary       m_sampleSummary;
    ChartConfig       m_barChartConfig;
    ChartConfig       m_lineChartConfig;
    ChartConfig       m_pieChartConfig;
    VisualizationInfo m_sampleViz;

    // ── AppError 생성 헬퍼 ─────────────────────────────────
    static AppError MakeError(const CString& code,
                               const CString& message,
                               int severity = 2) {
        return AppError(code, message, severity);
    }

    static AppError MakeWarning(const CString& code, const CString& message) {
        return AppError(code, message, 1);
    }

    static AppError MakeInfo(const CString& code, const CString& message) {
        return AppError(code, message, 0);
    }

private:
    // ============================================================
    // DataTable 빌더
    // sample_sales.csv: 날짜, 카테고리, 매출, 수량, 지역 — 10행
    // ============================================================
    static DataTable BuildSampleDataTable() {
        DataTable table;
        table.fileName = _T("sample_sales.csv");
        table.rowCount = 10;
        table.colCount = 5;

        // ── 헤더 ──────────────────────────────────────────
        table.headers = { _T("날짜"), _T("카테고리"), _T("매출"), _T("수량"), _T("지역") };

        // ── 컬럼 기반 데이터 ──────────────────────────────
        // 날짜 컬럼 (date)
        DataColumn colDate;
        colDate.name = _T("날짜");
        colDate.type = _T("date");
        colDate.values = {
            _T("2024-01"), _T("2024-02"), _T("2024-03"), _T("2024-04"),
            _T("2024-05"), _T("2024-06"), _T("2024-07"), _T("2024-08"),
            _T("2024-09"), _T("2024-10")
        };

        // 카테고리 컬럼 (text)
        DataColumn colCategory;
        colCategory.name = _T("카테고리");
        colCategory.type = _T("text");
        colCategory.values = {
            _T("전자제품"), _T("전자제품"), _T("의류"), _T("의류"),
            _T("식품"),    _T("식품"),    _T("전자제품"), _T("의류"),
            _T("식품"),    _T("전자제품")
        };

        // 매출 컬럼 (numeric)
        DataColumn colSales;
        colSales.name = _T("매출");
        colSales.type = _T("numeric");
        colSales.values = {
            _T("1500000"), _T("1800000"), _T("900000"),  _T("1100000"),
            _T("500000"),  _T("600000"),  _T("2000000"), _T("1200000"),
            _T("700000"),  _T("2200000")
        };

        // 수량 컬럼 (numeric)
        DataColumn colQty;
        colQty.name = _T("수량");
        colQty.type = _T("numeric");
        colQty.values = {
            _T("120"), _T("150"), _T("200"), _T("180"),
            _T("300"), _T("350"), _T("170"), _T("220"),
            _T("400"), _T("190")
        };

        // 지역 컬럼 (text)
        DataColumn colRegion;
        colRegion.name = _T("지역");
        colRegion.type = _T("text");
        colRegion.values = {
            _T("서울"), _T("서울"), _T("부산"), _T("부산"),
            _T("대구"), _T("대구"), _T("서울"), _T("인천"),
            _T("광주"), _T("서울")
        };

        table.columns = { colDate, colCategory, colSales, colQty, colRegion };

        // ── 행 기반 데이터 ────────────────────────────────
        table.rows = {
            { _T("2024-01"), _T("전자제품"), _T("1500000"), _T("120"), _T("서울") },
            { _T("2024-02"), _T("전자제품"), _T("1800000"), _T("150"), _T("서울") },
            { _T("2024-03"), _T("의류"),     _T("900000"),  _T("200"), _T("부산") },
            { _T("2024-04"), _T("의류"),     _T("1100000"), _T("180"), _T("부산") },
            { _T("2024-05"), _T("식품"),     _T("500000"),  _T("300"), _T("대구") },
            { _T("2024-06"), _T("식품"),     _T("600000"),  _T("350"), _T("대구") },
            { _T("2024-07"), _T("전자제품"), _T("2000000"), _T("170"), _T("서울") },
            { _T("2024-08"), _T("의류"),     _T("1200000"), _T("220"), _T("인천") },
            { _T("2024-09"), _T("식품"),     _T("700000"),  _T("400"), _T("광주") },
            { _T("2024-10"), _T("전자제품"), _T("2200000"), _T("190"), _T("서울") },
        };

        return table;
    }

    // ============================================================
    // DataSummary 빌더
    // ============================================================
    static DataSummary BuildSampleDataSummary() {
        DataSummary summary;
        summary.datasourceId  = _T("1");
        summary.rowCount      = 10;
        summary.columnCount   = 5;
        summary.aiSummaryText = _T("2024년 1월~10월 판매 데이터. "
                                   "전자제품 매출이 가장 높으며 서울 지역 비중이 크다. "
                                   "월별 매출 증가 추세를 보인다.");

        // 날짜 스키마
        ColumnSchema schDate;
        schDate.name         = _T("날짜");
        schDate.type         = _T("date");
        schDate.nullCount    = 0;
        schDate.index        = 0;
        schDate.sampleValues = _T("2024-01,2024-02,2024-03");

        // 카테고리 스키마
        ColumnSchema schCat;
        schCat.name         = _T("카테고리");
        schCat.type         = _T("text");
        schCat.nullCount    = 0;
        schCat.index        = 1;
        schCat.sampleValues = _T("전자제품,의류,식품");

        // 매출 스키마
        ColumnSchema schSales;
        schSales.name         = _T("매출");
        schSales.type         = _T("numeric");
        schSales.nullCount    = 0;
        schSales.index        = 2;
        schSales.sampleValues = _T("1500000,1800000,900000");

        // 수량 스키마
        ColumnSchema schQty;
        schQty.name         = _T("수량");
        schQty.type         = _T("numeric");
        schQty.nullCount    = 0;
        schQty.index        = 3;
        schQty.sampleValues = _T("120,150,200");

        // 지역 스키마
        ColumnSchema schRegion;
        schRegion.name         = _T("지역");
        schRegion.type         = _T("text");
        schRegion.nullCount    = 0;
        schRegion.index        = 4;
        schRegion.sampleValues = _T("서울,부산,대구,인천,광주");

        summary.schema = { schDate, schCat, schSales, schQty, schRegion };

        return summary;
    }

    // ============================================================
    // ChartConfig 빌더 — Bar
    // ============================================================
    static ChartConfig BuildBarChartConfig() {
        ChartConfig cfg;
        cfg.chartType = _T("bar");
        cfg.title     = _T("카테고리별 월 매출");
        cfg.xLabel    = _T("카테고리");
        cfg.yLabel    = _T("매출 (원)");
        cfg.dataJson  = _T("{\"labels\":[\"전자제품\",\"의류\",\"식품\"],"
                           "\"values\":[7500000,3200000,1800000]}");
        return cfg;
    }

    // ============================================================
    // ChartConfig 빌더 — Line
    // ============================================================
    static ChartConfig BuildLineChartConfig() {
        ChartConfig cfg;
        cfg.chartType = _T("line");
        cfg.title     = _T("월별 매출 추이");
        cfg.xLabel    = _T("월");
        cfg.yLabel    = _T("매출 (원)");
        cfg.dataJson  = _T("{\"labels\":[\"2024-01\",\"2024-02\",\"2024-03\","
                           "\"2024-04\",\"2024-05\",\"2024-06\","
                           "\"2024-07\",\"2024-08\",\"2024-09\",\"2024-10\"],"
                           "\"values\":[1500000,1800000,900000,1100000,"
                           "500000,600000,2000000,1200000,700000,2200000]}");
        return cfg;
    }

    // ============================================================
    // ChartConfig 빌더 — Pie
    // ============================================================
    static ChartConfig BuildPieChartConfig() {
        ChartConfig cfg;
        cfg.chartType = _T("pie");
        cfg.title     = _T("지역별 매출 비율");
        cfg.xLabel    = _T("지역");
        cfg.yLabel    = _T("매출 비율");
        cfg.dataJson  = _T("{\"labels\":[\"서울\",\"부산\",\"대구\",\"인천\",\"광주\"],"
                           "\"values\":[7500000,2000000,1100000,1200000,700000]}");
        return cfg;
    }

    // ============================================================
    // VisualizationInfo 빌더
    // ============================================================
    static VisualizationInfo BuildSampleVisualizationInfo() {
        VisualizationInfo viz;
        viz.id          = _T("viz-001");
        viz.dashboardId = _T("dash-001");
        viz.vizType     = _T("bar_chart");
        viz.title       = _T("카테고리별 월 매출");
        viz.chartConfig = BuildBarChartConfig();

        // ChartStyle
        viz.style.primaryColor = _T("#4A90E2");
        viz.style.fontFamily   = _T("Malgun Gothic");
        viz.style.fontSize     = 12;
        viz.style.showLegend   = TRUE;
        viz.style.showGrid     = TRUE;

        // LayoutItem
        viz.position.id = _T("viz-001");
        viz.position.x  = 0;
        viz.position.y  = 0;
        viz.position.w  = 6;
        viz.position.h  = 4;

        return viz;
    }
};
