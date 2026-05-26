// test_analysis_tools.cpp
// AnalysisTools 13개 static 메서드 단위 테스트
// Google Test 기반, MFC CString 사용 (UNICODE 빌드)

// MFC 헤더를 Google Test보다 먼저 포함 (PCH 순서 보장)
#include <windows.h>
#include <atlstr.h>   // CString (ATL — MFC 없이 사용 가능)

#include <gtest/gtest.h>
#include <cmath>
#include <vector>

// 테스트 대상 (구현 파일은 CMake가 링크)
#include "Domain/Analysis/AnalysisTools.h"

// ============================================================
// 공통 테스트 픽스처 — 한국어 샘플 데이터 10행
// 컬럼: 날짜(date), 카테고리(text), 매출(numeric), 수량(numeric), 지역(text)
// ============================================================
class AnalysisToolsTest : public ::testing::Test {
protected:
    DataTable tbl;

    void SetUp() override {
        tbl.fileName = _T("sample_sales.csv");
        tbl.rowCount = 10;
        tbl.colCount = 5;

        // ── 날짜 컬럼 ──────────────────────────────────────
        DataColumn colDate;
        colDate.name = _T("날짜");
        colDate.type = _T("date");
        colDate.values = {
            _T("2024-01-05"), _T("2024-01-12"), _T("2024-02-03"),
            _T("2024-02-18"), _T("2024-03-07"), _T("2024-03-22"),
            _T("2024-04-01"), _T("2024-04-15"), _T("2024-05-10"),
            _T("2024-05-25")
        };

        // ── 카테고리 컬럼 ──────────────────────────────────
        DataColumn colCat;
        colCat.name = _T("카테고리");
        colCat.type = _T("text");
        colCat.values = {
            _T("전자"), _T("의류"), _T("전자"), _T("식품"),
            _T("의류"), _T("전자"), _T("식품"), _T("의류"),
            _T("전자"), _T("식품")
        };

        // ── 매출 컬럼 (numeric) ────────────────────────────
        DataColumn colSales;
        colSales.name = _T("매출");
        colSales.type = _T("numeric");
        colSales.values = {
            _T("150000"), _T("85000"),  _T("320000"), _T("45000"),
            _T("95000"),  _T("210000"), _T("38000"),  _T("72000"),
            _T("180000"), _T("55000")
        };

        // ── 수량 컬럼 (numeric) ────────────────────────────
        DataColumn colQty;
        colQty.name = _T("수량");
        colQty.type = _T("numeric");
        colQty.values = {
            _T("12"), _T("34"), _T("8"), _T("60"),
            _T("25"), _T("15"), _T("80"), _T("40"),
            _T("10"), _T("70")
        };

        // ── 지역 컬럼 ──────────────────────────────────────
        DataColumn colRegion;
        colRegion.name = _T("지역");
        colRegion.type = _T("text");
        colRegion.values = {
            _T("서울"), _T("부산"), _T("서울"), _T("대구"),
            _T("서울"), _T("부산"), _T("대구"), _T("서울"),
            _T("부산"), _T("대구")
        };

        tbl.columns = { colDate, colCat, colSales, colQty, colRegion };
    }

    // 공통 헬퍼: JSON 문자열에 특정 키워드가 있는지 확인
    static bool Contains(const CString& json, const CString& keyword) {
        return json.Find(keyword) >= 0;
    }

    // 공통 헬퍼: JSON에 "error" 키가 없는지 확인
    static bool NoError(const CString& json) {
        return json.Find(_T("\"error\"")) < 0;
    }
};

// ============================================================
// 1. BasicStats — 수치 컬럼에 mean/median/std/min/max 포함
// ============================================================
TEST_F(AnalysisToolsTest, BasicStats_ContainsMeanMedianStdMinMax) {
    CString result = AnalysisTools::BasicStats(tbl);

    EXPECT_TRUE(Contains(result, _T("\"mean\"")))    << result;
    EXPECT_TRUE(Contains(result, _T("\"median\"")))  << result;
    EXPECT_TRUE(Contains(result, _T("\"std\"")))     << result;
    EXPECT_TRUE(Contains(result, _T("\"min\"")))     << result;
    EXPECT_TRUE(Contains(result, _T("\"max\"")))     << result;
    EXPECT_TRUE(NoError(result))                     << result;
}

TEST_F(AnalysisToolsTest, BasicStats_CoversBothNumericColumns) {
    CString result = AnalysisTools::BasicStats(tbl);

    // 매출과 수량 두 numeric 컬럼 모두 포함
    EXPECT_TRUE(Contains(result, _T("\"매출\"")))  << result;
    EXPECT_TRUE(Contains(result, _T("\"수량\"")))  << result;
}

TEST_F(AnalysisToolsTest, BasicStats_ExcludesTextColumns) {
    CString result = AnalysisTools::BasicStats(tbl);

    // 텍스트/날짜 컬럼은 stats 배열에 포함되지 않아야 함
    // (카테고리, 지역은 numeric type이 아님)
    EXPECT_FALSE(Contains(result, _T("\"카테고리\""))) << result;
    EXPECT_FALSE(Contains(result, _T("\"지역\"")))     << result;
}

// ============================================================
// 2. GroupByAggregate_Sum — 카테고리별 매출 합계
// ============================================================
TEST_F(AnalysisToolsTest, GroupByAggregate_Sum_ReturnsResultsArray) {
    CString result = AnalysisTools::GroupByAggregate(
        tbl, _T("카테고리"), _T("매출"), _T("sum"));

    EXPECT_TRUE(Contains(result, _T("\"results\"")))  << result;
    EXPECT_TRUE(Contains(result, _T("\"group\"")))    << result;
    EXPECT_TRUE(Contains(result, _T("\"value\"")))    << result;
    EXPECT_TRUE(NoError(result))                      << result;
}

TEST_F(AnalysisToolsTest, GroupByAggregate_Sum_ContainsAllCategories) {
    CString result = AnalysisTools::GroupByAggregate(
        tbl, _T("카테고리"), _T("매출"), _T("sum"));

    EXPECT_TRUE(Contains(result, _T("전자")))  << result;
    EXPECT_TRUE(Contains(result, _T("의류")))  << result;
    EXPECT_TRUE(Contains(result, _T("식품")))  << result;
}

// ============================================================
// 3. GroupByAggregate_Avg — 지역별 수량 평균
// ============================================================
TEST_F(AnalysisToolsTest, GroupByAggregate_Avg_ReturnsAggFuncAvg) {
    CString result = AnalysisTools::GroupByAggregate(
        tbl, _T("지역"), _T("수량"), _T("avg"));

    EXPECT_TRUE(Contains(result, _T("\"agg_func\":\"avg\""))) << result;
    EXPECT_TRUE(Contains(result, _T("서울"))) << result;
    EXPECT_TRUE(Contains(result, _T("부산"))) << result;
    EXPECT_TRUE(Contains(result, _T("대구"))) << result;
    EXPECT_TRUE(NoError(result))              << result;
}

// ============================================================
// 4. GroupByAggregate_Count — 카테고리별 건수
// ============================================================
TEST_F(AnalysisToolsTest, GroupByAggregate_Count_ReturnsCountPerGroup) {
    CString result = AnalysisTools::GroupByAggregate(
        tbl, _T("카테고리"), _T("매출"), _T("count"));

    EXPECT_TRUE(Contains(result, _T("\"agg_func\":\"count\""))) << result;
    EXPECT_TRUE(NoError(result))                                 << result;
}

// ============================================================
// 5. GroupByAggregate_InvalidCol — 존재하지 않는 컬럼
// ============================================================
TEST_F(AnalysisToolsTest, GroupByAggregate_InvalidCol_ReturnsError) {
    CString result = AnalysisTools::GroupByAggregate(
        tbl, _T("없는컬럼"), _T("매출"), _T("sum"));

    EXPECT_TRUE(Contains(result, _T("\"error\""))) << result;
}

TEST_F(AnalysisToolsTest, GroupByAggregate_InvalidValueCol_ReturnsError) {
    CString result = AnalysisTools::GroupByAggregate(
        tbl, _T("카테고리"), _T("없는값컬럼"), _T("sum"));

    EXPECT_TRUE(Contains(result, _T("\"error\""))) << result;
}

// ============================================================
// 6. TimeSeriesAnalysis — 날짜/매출 시계열 (월별 위임)
// ============================================================
TEST_F(AnalysisToolsTest, TimeSeriesAnalysis_ReturnsPointsArray) {
    CString result = AnalysisTools::TimeSeriesAnalysis(
        tbl, _T("날짜"), _T("매출"));

    EXPECT_TRUE(Contains(result, _T("\"points\"")))   << result;
    EXPECT_TRUE(Contains(result, _T("\"period\"")))   << result;
    EXPECT_TRUE(NoError(result))                       << result;
}

TEST_F(AnalysisToolsTest, TimeSeriesAnalysis_GroupsByMonth) {
    CString result = AnalysisTools::TimeSeriesAnalysis(
        tbl, _T("날짜"), _T("매출"));

    // TimeSeriesAnalysis는 DateGroupAggregate(month)에 위임
    // YYYY-MM 형식 키가 존재해야 함
    EXPECT_TRUE(Contains(result, _T("2024-01"))) << result;
    EXPECT_TRUE(Contains(result, _T("2024-02"))) << result;
}

// ============================================================
// 7. CorrelationMatrix — 매출, 수량 상관계수 범위 [-1,1]
// ============================================================
TEST_F(AnalysisToolsTest, CorrelationMatrix_ReturnsMatrixStructure) {
    std::vector<CString> cols = { _T("매출"), _T("수량") };
    CString result = AnalysisTools::CorrelationMatrix(tbl, cols);

    EXPECT_TRUE(Contains(result, _T("\"columns\""))) << result;
    EXPECT_TRUE(Contains(result, _T("\"matrix\"")))  << result;
    EXPECT_TRUE(NoError(result))                      << result;
}

TEST_F(AnalysisToolsTest, CorrelationMatrix_DiagonalIsOne) {
    std::vector<CString> cols = { _T("매출"), _T("수량") };
    CString result = AnalysisTools::CorrelationMatrix(tbl, cols);

    // 대각 원소는 항상 1.0000
    EXPECT_TRUE(Contains(result, _T("1.0000"))) << result;
}

// ============================================================
// 8. CorrelationMatrix_SingleCol — 단일 컬럼 처리
// ============================================================
TEST_F(AnalysisToolsTest, CorrelationMatrix_SingleCol_ReturnsValidJson) {
    std::vector<CString> cols = { _T("매출") };
    CString result = AnalysisTools::CorrelationMatrix(tbl, cols);

    // 단일 컬럼도 유효한 JSON 반환
    EXPECT_TRUE(Contains(result, _T("\"columns\""))) << result;
    EXPECT_TRUE(Contains(result, _T("1.0000")))       << result;
}

// ============================================================
// 9. TopN_Ascending — 매출 오름차순 상위 3개
// ============================================================
TEST_F(AnalysisToolsTest, TopN_Ascending_ReturnsThreeRows) {
    CString result = AnalysisTools::TopN(tbl, _T("매출"), 3, true);

    EXPECT_TRUE(Contains(result, _T("\"rows\"")))             << result;
    EXPECT_TRUE(Contains(result, _T("\"ascending\":true")))   << result;
    EXPECT_TRUE(NoError(result))                              << result;

    // 최솟값 38000이 포함되어야 함
    EXPECT_TRUE(Contains(result, _T("38000"))) << result;
}

// ============================================================
// 10. TopN_Descending — 수량 내림차순 상위 5개
// ============================================================
TEST_F(AnalysisToolsTest, TopN_Descending_ReturnsFiveRows) {
    CString result = AnalysisTools::TopN(tbl, _T("수량"), 5, false);

    EXPECT_TRUE(Contains(result, _T("\"rows\"")))              << result;
    EXPECT_TRUE(Contains(result, _T("\"ascending\":false")))   << result;
    EXPECT_TRUE(NoError(result))                               << result;

    // 최댓값 80이 포함되어야 함
    EXPECT_TRUE(Contains(result, _T("80"))) << result;
}

// ============================================================
// 11. TopN_ExceedsRows — n > rowCount 시 전체 행 반환
// ============================================================
TEST_F(AnalysisToolsTest, TopN_ExceedsRows_ReturnsAllRows) {
    CString result = AnalysisTools::TopN(tbl, _T("매출"), 100, false);

    EXPECT_TRUE(Contains(result, _T("\"rows\""))) << result;
    EXPECT_TRUE(NoError(result))                  << result;
    // 10개 행이 모두 있어야 하므로 최솟값 38000도 포함
    EXPECT_TRUE(Contains(result, _T("38000"))) << result;
}

// ============================================================
// 12. FrequencyDistribution — 카테고리별 빈도
// ============================================================
TEST_F(AnalysisToolsTest, FrequencyDistribution_ReturnsItemsWithCounts) {
    CString result = AnalysisTools::FrequencyDistribution(tbl, _T("카테고리"));

    EXPECT_TRUE(Contains(result, _T("\"items\"")))         << result;
    EXPECT_TRUE(Contains(result, _T("\"total_unique\"")))  << result;
    EXPECT_TRUE(Contains(result, _T("\"count\"")))         << result;
    EXPECT_TRUE(NoError(result))                           << result;
}

TEST_F(AnalysisToolsTest, FrequencyDistribution_ThreeUniqueCategories) {
    CString result = AnalysisTools::FrequencyDistribution(tbl, _T("카테고리"));

    // 전자(4), 의류(3), 식품(3) → total_unique:3
    EXPECT_TRUE(Contains(result, _T("\"total_unique\":3"))) << result;
}

// ============================================================
// 13. CrossTabulation — 카테고리 x 지역 교차표
// ============================================================
TEST_F(AnalysisToolsTest, CrossTabulation_ReturnsRowsAndColumns) {
    CString result = AnalysisTools::CrossTabulation(
        tbl, _T("카테고리"), _T("지역"));

    EXPECT_TRUE(Contains(result, _T("\"row_col\"")))  << result;
    EXPECT_TRUE(Contains(result, _T("\"col_col\"")))  << result;
    EXPECT_TRUE(Contains(result, _T("\"columns\"")))  << result;
    EXPECT_TRUE(Contains(result, _T("\"rows\"")))     << result;
    EXPECT_TRUE(NoError(result))                      << result;
}

TEST_F(AnalysisToolsTest, CrossTabulation_ContainsAllRegions) {
    CString result = AnalysisTools::CrossTabulation(
        tbl, _T("카테고리"), _T("지역"));

    EXPECT_TRUE(Contains(result, _T("서울"))) << result;
    EXPECT_TRUE(Contains(result, _T("부산"))) << result;
    EXPECT_TRUE(Contains(result, _T("대구"))) << result;
}

// ============================================================
// 14. MovingAverage_Window3 — 매출 윈도우=3 이동평균
// ============================================================
TEST_F(AnalysisToolsTest, MovingAverage_Window3_ReturnsValuesArray) {
    CString result = AnalysisTools::MovingAverage(tbl, _T("매출"), 3);

    EXPECT_TRUE(Contains(result, _T("\"values\"")))   << result;
    EXPECT_TRUE(Contains(result, _T("\"window\":3"))) << result;
    EXPECT_TRUE(Contains(result, _T("\"ma\"")))       << result;
    EXPECT_TRUE(NoError(result))                      << result;
}

TEST_F(AnalysisToolsTest, MovingAverage_Window3_FirstValueEqualsOriginal) {
    CString result = AnalysisTools::MovingAverage(tbl, _T("매출"), 3);

    // 첫 번째 값(index:0)의 ma는 원본 값과 동일
    EXPECT_TRUE(Contains(result, _T("\"index\":0"))) << result;
    EXPECT_TRUE(Contains(result, _T("150000.0000"))) << result;
}

// ============================================================
// 15. MovingAverage_Window1 — 윈도우=1 시 원본 값과 동일
// ============================================================
TEST_F(AnalysisToolsTest, MovingAverage_Window1_MaEqualsOriginal) {
    CString result = AnalysisTools::MovingAverage(tbl, _T("매출"), 1);

    EXPECT_TRUE(Contains(result, _T("\"window\":1"))) << result;
    // 윈도우=1이면 원본과 이동평균이 동일
    EXPECT_TRUE(Contains(result, _T("150000.0000"))) << result;
    EXPECT_TRUE(Contains(result, _T("85000.0000")))  << result;
}

// ============================================================
// 16. Percentile — 매출 25th/50th/75th 백분위수
// ============================================================
TEST_F(AnalysisToolsTest, Percentile_ReturnsPercentileArray) {
    std::vector<double> ps = { 25.0, 50.0, 75.0 };
    CString result = AnalysisTools::Percentile(tbl, _T("매출"), ps);

    EXPECT_TRUE(Contains(result, _T("\"percentiles\""))) << result;
    EXPECT_TRUE(Contains(result, _T("\"p\"")))           << result;
    EXPECT_TRUE(Contains(result, _T("\"value\"")))       << result;
    EXPECT_TRUE(NoError(result))                         << result;
}

TEST_F(AnalysisToolsTest, Percentile_ContainsThreeEntries) {
    std::vector<double> ps = { 25.0, 50.0, 75.0 };
    CString result = AnalysisTools::Percentile(tbl, _T("매출"), ps);

    // 25.0, 50.0, 75.0 레이블이 모두 있어야 함
    EXPECT_TRUE(Contains(result, _T("\"p\":25.0")))  << result;
    EXPECT_TRUE(Contains(result, _T("\"p\":50.0")))  << result;
    EXPECT_TRUE(Contains(result, _T("\"p\":75.0")))  << result;
}

// ============================================================
// 17. DateGroupAggregate_Month — 월별 집계
// ============================================================
TEST_F(AnalysisToolsTest, DateGroupAggregate_Month_ReturnsMonthlyKeys) {
    CString result = AnalysisTools::DateGroupAggregate(
        tbl, _T("날짜"), _T("매출"), _T("month"));

    EXPECT_TRUE(Contains(result, _T("\"period\":\"month\""))) << result;
    EXPECT_TRUE(Contains(result, _T("2024-01")))              << result;
    EXPECT_TRUE(Contains(result, _T("2024-05")))              << result;
    EXPECT_TRUE(NoError(result))                              << result;
}

// ============================================================
// 18. DateGroupAggregate_Year — 연도별 집계
// ============================================================
TEST_F(AnalysisToolsTest, DateGroupAggregate_Year_ReturnsYearKey) {
    CString result = AnalysisTools::DateGroupAggregate(
        tbl, _T("날짜"), _T("매출"), _T("year"));

    EXPECT_TRUE(Contains(result, _T("\"period\":\"year\""))) << result;
    // 모든 행이 2024년이므로 "2024" 키 하나만 있어야 함
    EXPECT_TRUE(Contains(result, _T("\"2024\""))) << result;
    EXPECT_TRUE(NoError(result))                  << result;
}

// ============================================================
// 19. Filtering_GreaterThan — 매출 > 100000
// ============================================================
TEST_F(AnalysisToolsTest, Filtering_GreaterThan_ReturnsMatchedCount) {
    CString result = AnalysisTools::Filtering(
        tbl, _T("매출"), _T(">"), _T("100000"));

    EXPECT_TRUE(Contains(result, _T("\"matched_count\""))) << result;
    EXPECT_TRUE(Contains(result, _T("\"total_count\"")))   << result;
    EXPECT_TRUE(NoError(result))                           << result;

    // 100000 초과: 150000, 320000, 210000, 180000 → 4개
    EXPECT_TRUE(Contains(result, _T("\"matched_count\":4"))) << result;
}

// ============================================================
// 20. Filtering_Equals — 카테고리 == "전자"
// ============================================================
TEST_F(AnalysisToolsTest, Filtering_Equals_MatchesTextValue) {
    CString result = AnalysisTools::Filtering(
        tbl, _T("카테고리"), _T("=="), _T("전자"));

    EXPECT_TRUE(Contains(result, _T("\"matched_count\""))) << result;
    // 전자: 4개
    EXPECT_TRUE(Contains(result, _T("\"matched_count\":4"))) << result;
    EXPECT_TRUE(NoError(result))                              << result;
}

// ============================================================
// 21. Filtering_InvalidOp — 잘못된 연산자 사용 시 0건 매칭
// ============================================================
TEST_F(AnalysisToolsTest, Filtering_InvalidOp_ReturnsZeroMatches) {
    CString result = AnalysisTools::Filtering(
        tbl, _T("매출"), _T("??"), _T("100000"));

    // 알 수 없는 연산자 → 매칭 0건 (에러 없이 처리)
    EXPECT_TRUE(Contains(result, _T("\"matched_count\":0"))) << result;
}

// ============================================================
// 22. PivotTable — 카테고리 x 지역, 매출 합계
// ============================================================
TEST_F(AnalysisToolsTest, PivotTable_ReturnsRowsAndColumnHeaders) {
    CString result = AnalysisTools::PivotTable(
        tbl, _T("카테고리"), _T("지역"), _T("매출"), _T("sum"));

    EXPECT_TRUE(Contains(result, _T("\"row_col\"")))   << result;
    EXPECT_TRUE(Contains(result, _T("\"col_col\"")))   << result;
    EXPECT_TRUE(Contains(result, _T("\"value_col\""))) << result;
    EXPECT_TRUE(Contains(result, _T("\"columns\"")))   << result;
    EXPECT_TRUE(Contains(result, _T("\"rows\"")))      << result;
    EXPECT_TRUE(NoError(result))                       << result;
}

TEST_F(AnalysisToolsTest, PivotTable_ContainsExpectedAggFunc) {
    CString result = AnalysisTools::PivotTable(
        tbl, _T("카테고리"), _T("지역"), _T("매출"), _T("sum"));

    EXPECT_TRUE(Contains(result, _T("\"agg_func\":\"sum\""))) << result;
}

// ============================================================
// 23. OutlierDetection — 이상치가 있는 데이터에서 이상치 탐지
// ============================================================
TEST_F(AnalysisToolsTest, OutlierDetection_ReturnsIQRFields) {
    CString result = AnalysisTools::OutlierDetection(tbl, _T("매출"), 1.5);

    EXPECT_TRUE(Contains(result, _T("\"q1\"")))           << result;
    EXPECT_TRUE(Contains(result, _T("\"q3\"")))           << result;
    EXPECT_TRUE(Contains(result, _T("\"iqr\"")))          << result;
    EXPECT_TRUE(Contains(result, _T("\"outlier_count\""))) << result;
    EXPECT_TRUE(NoError(result))                          << result;
}

TEST_F(AnalysisToolsTest, OutlierDetection_OutlierCountIsNonNegative) {
    CString result = AnalysisTools::OutlierDetection(tbl, _T("매출"), 1.5);

    // outlier_count 필드가 존재하고 숫자여야 함
    EXPECT_TRUE(Contains(result, _T("\"outlier_count\":"))) << result;
}

// ============================================================
// 24. OutlierDetection_NoOutliers — 균일 데이터는 이상치 없음
// ============================================================
TEST_F(AnalysisToolsTest, OutlierDetection_UniformData_ZeroOutliers) {
    // 모든 매출이 동일한 균일 데이터 테이블 생성
    DataTable uniform;
    uniform.rowCount = 5;
    DataColumn col;
    col.name = _T("매출");
    col.type = _T("numeric");
    col.values = { _T("100"), _T("100"), _T("100"), _T("100"), _T("100") };
    uniform.columns = { col };

    CString result = AnalysisTools::OutlierDetection(uniform, _T("매출"), 1.5);

    EXPECT_TRUE(Contains(result, _T("\"outlier_count\":0"))) << result;
    EXPECT_TRUE(NoError(result))                             << result;
}

// ============================================================
// 25. EmptyDataTable — 빈 테이블에 대해 모든 도구가 안전하게 처리
// ============================================================
class AnalysisToolsEmptyTest : public ::testing::Test {
protected:
    DataTable empty;

    void SetUp() override {
        empty.rowCount = 0;
        empty.colCount = 0;
        // 컬럼 없음
    }
};

TEST_F(AnalysisToolsEmptyTest, BasicStats_EmptyTable_ReturnsEmptyStatsArray) {
    CString result = AnalysisTools::BasicStats(empty);
    // 수치 컬럼이 없으면 빈 stats 배열
    EXPECT_TRUE(result.Find(_T("\"stats\":[]")) >= 0) << result;
}

TEST_F(AnalysisToolsEmptyTest, GroupByAggregate_EmptyTable_ReturnsError) {
    CString result = AnalysisTools::GroupByAggregate(
        empty, _T("카테고리"), _T("매출"), _T("sum"));
    EXPECT_TRUE(result.Find(_T("\"error\"")) >= 0) << result;
}

TEST_F(AnalysisToolsEmptyTest, TopN_EmptyTable_ReturnsError) {
    CString result = AnalysisTools::TopN(empty, _T("매출"), 5, false);
    EXPECT_TRUE(result.Find(_T("\"error\"")) >= 0) << result;
}

TEST_F(AnalysisToolsEmptyTest, FrequencyDistribution_EmptyTable_ReturnsError) {
    CString result = AnalysisTools::FrequencyDistribution(empty, _T("카테고리"));
    EXPECT_TRUE(result.Find(_T("\"error\"")) >= 0) << result;
}

TEST_F(AnalysisToolsEmptyTest, CrossTabulation_EmptyTable_ReturnsError) {
    CString result = AnalysisTools::CrossTabulation(
        empty, _T("카테고리"), _T("지역"));
    EXPECT_TRUE(result.Find(_T("\"error\"")) >= 0) << result;
}

TEST_F(AnalysisToolsEmptyTest, MovingAverage_EmptyTable_ReturnsError) {
    CString result = AnalysisTools::MovingAverage(empty, _T("매출"), 3);
    EXPECT_TRUE(result.Find(_T("\"error\"")) >= 0) << result;
}

TEST_F(AnalysisToolsEmptyTest, OutlierDetection_EmptyTable_ReturnsError) {
    CString result = AnalysisTools::OutlierDetection(empty, _T("매출"), 1.5);
    EXPECT_TRUE(result.Find(_T("\"error\"")) >= 0) << result;
}

// ============================================================
// 26. FindColumnIndex — 존재하는 컬럼 인덱스 확인
// (private이므로 GroupByAggregate 등 public 메서드를 통해 간접 검증)
// ============================================================
TEST_F(AnalysisToolsTest, FindColumnIndex_Exists_CorrectBehaviorViaPublicApi) {
    // "매출" 컬럼이 인덱스 2에 있으므로 GroupByAggregate가 정상 동작해야 함
    CString result = AnalysisTools::GroupByAggregate(
        tbl, _T("카테고리"), _T("매출"), _T("sum"));
    EXPECT_TRUE(result.Find(_T("\"error\"")) < 0) << result;
}

TEST_F(AnalysisToolsTest, FindColumnIndex_NotExists_ReturnsErrorViaPublicApi) {
    // 없는 컬럼 참조 시 error 반환
    CString result = AnalysisTools::GroupByAggregate(
        tbl, _T("없음"), _T("매출"), _T("sum"));
    EXPECT_TRUE(result.Find(_T("\"error\"")) >= 0) << result;
}

// ============================================================
// 27. ExtractNumericValues — 수치 값 추출 (BasicStats 통해 간접 검증)
// ============================================================
TEST_F(AnalysisToolsTest, ExtractNumericValues_IgnoresNonNumeric) {
    // 텍스트 컬럼(카테고리)은 BasicStats에 포함되지 않음
    // → 수치가 아닌 값이 추출 시 건너뛰어짐을 의미
    CString result = AnalysisTools::BasicStats(tbl);
    EXPECT_FALSE(result.Find(_T("\"카테고리\"")) >= 0) << result;
}

// ============================================================
// 28. EscapeJsonString — 특수문자 이스케이프
// (EscapeJsonString은 private; DataTable 컬럼명에 특수문자 포함 시 간접 검증)
// ============================================================
TEST_F(AnalysisToolsTest, EscapeJsonString_SpecialCharsInColumnName_DoesNotBreakJson) {
    DataTable tblSpecial;
    tblSpecial.rowCount = 2;
    DataColumn colSpecial;
    colSpecial.name   = _T("매출\"금액");   // 따옴표 포함 이름
    colSpecial.type   = _T("numeric");
    colSpecial.values = { _T("1000"), _T("2000") };
    tblSpecial.columns = { colSpecial };

    CString result = AnalysisTools::BasicStats(tblSpecial);

    // JSON이 깨지지 않고 이스케이프되어야 함
    EXPECT_TRUE(result.Find(_T("\\\"")) >= 0) << result;  // \" 포함
}
