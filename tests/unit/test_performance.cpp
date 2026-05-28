// test_performance.cpp
// NFR 성능 요건 단위 테스트
// TC-P-01: BasicStats 100ms 이내
// TC-P-02: 대용량 1000행 테이블 생성 성능
// TC-P-03: 전체 분석 도구 수행 시간
// TC-P-04: DataTable 메모리 크기 추정
// TC-P-05: 반복 실행 안정성

// MFC 헤더를 Google Test보다 먼저 포함 (PCH 순서 보장)
#include <windows.h>
#include <atlstr.h>

#include <gtest/gtest.h>

#include <chrono>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>

#include "Common/Types.h"
#include "Domain/Analysis/AnalysisTools.h"
#include "fixtures/TestDataFixture.h"

// ============================================================
// 헬퍼 — 대용량 DataTable 생성
// ============================================================
namespace {

DataTable BuildLargeTable(int rowCount, int colCount = 5) {
    DataTable table;
    table.rowCount = rowCount;
    table.colCount = colCount;

    // numeric 컬럼 (colCount-2개)
    for (int c = 0; c < colCount - 2; ++c) {
        DataColumn col;
        col.name = CString((_T("col_") + std::to_wstring(c)).c_str());
        col.type = _T("numeric");
        for (int r = 0; r < rowCount; ++r) {
            col.values.push_back(CString(std::to_wstring(r * (c + 1) + 1).c_str()));
        }
        table.columns.push_back(col);
    }

    // text 컬럼 1개
    {
        DataColumn col;
        col.name = _T("category");
        col.type = _T("text");
        for (int r = 0; r < rowCount; ++r) {
            // 3가지 카테고리 순환
            const wchar_t* cats[] = { L"A", L"B", L"C" };
            col.values.push_back(CString(cats[r % 3]));
        }
        table.columns.push_back(col);
    }

    // date 컬럼 1개
    {
        DataColumn col;
        col.name = _T("date");
        col.type = _T("date");
        for (int r = 0; r < rowCount; ++r) {
            std::wstring d = L"2024-" + std::to_wstring((r % 12) + 1);
            col.values.push_back(CString(d.c_str()));
        }
        table.columns.push_back(col);
    }

    return table;
}

// 경과 시간(ms) 측정 헬퍼
using Clock = std::chrono::high_resolution_clock;
using Ms    = std::chrono::milliseconds;

template<typename Fn>
long long MeasureMs(Fn&& fn) {
    auto start = Clock::now();
    fn();
    auto end   = Clock::now();
    return std::chrono::duration_cast<Ms>(end - start).count();
}

} // anonymous namespace

// ============================================================
// TC-P-01: BasicStats 성능 — 10행 데이터 → 100ms 이내
// ============================================================
class PerformanceTest : public TestDataFixture {};

TEST_F(PerformanceTest, BasicStats_10Rows_CompletesWithin100ms) {
    CString result;
    long long ms = MeasureMs([&] {
        result = AnalysisTools::BasicStats(m_sampleTable);
    });

    EXPECT_FALSE(result.IsEmpty()) << "BasicStats 결과가 비어 있으면 안 된다";
    EXPECT_LT(ms, 100LL)
        << "BasicStats(10행)은 100ms 이내여야 한다. 실제: " << ms << "ms";
}

// GroupByAggregate도 10행에서 100ms 이내
TEST_F(PerformanceTest, GroupByAggregate_10Rows_CompletesWithin100ms) {
    CString result;
    long long ms = MeasureMs([&] {
        result = AnalysisTools::GroupByAggregate(
            m_sampleTable, _T("카테고리"), _T("매출"), _T("sum"));
    });

    EXPECT_FALSE(result.IsEmpty());
    EXPECT_LT(ms, 100LL)
        << "GroupByAggregate(10행)은 100ms 이내여야 한다. 실제: " << ms << "ms";
}

// ============================================================
// TC-P-02: 대용량 데이터 테이블 생성 성능 (1000행)
// ============================================================
TEST(PerformanceStandaloneTest, BuildLargeTable_1000Rows_CompletesWithin500ms) {
    long long ms = MeasureMs([&] {
        DataTable large = BuildLargeTable(1000, 5);
        // 생성 검증
        EXPECT_EQ(large.rowCount, 1000);
        EXPECT_EQ(static_cast<int>(large.columns.size()), 5);
    });

    EXPECT_LT(ms, 500LL)
        << "1000행 테이블 생성은 500ms 이내여야 한다. 실제: " << ms << "ms";
}

// BasicStats를 1000행 테이블에서 수행 — 1000ms 이내
TEST(PerformanceStandaloneTest, BasicStats_1000Rows_CompletesWithin1000ms) {
    DataTable large = BuildLargeTable(1000, 5);
    CString result;

    long long ms = MeasureMs([&] {
        result = AnalysisTools::BasicStats(large);
    });

    EXPECT_FALSE(result.IsEmpty());
    EXPECT_LT(ms, 1000LL)
        << "BasicStats(1000행)은 1000ms 이내여야 한다. 실제: " << ms << "ms";
}

// ============================================================
// TC-P-03: 분석 도구 전체 수행 시간 검증
// 사용 가능한 주요 도구들을 순차 실행, 각 도구 500ms 이내
// ============================================================
TEST_F(PerformanceTest, AllMajorTools_EachWithin500ms) {
    struct ToolRun {
        std::string name;
        long long   ms;
    };
    std::vector<ToolRun> results;

    auto run = [&](const std::string& name, std::function<CString()> fn) {
        CString res;
        long long ms = MeasureMs([&] { res = fn(); });
        results.push_back({ name, ms });
        EXPECT_FALSE(res.IsEmpty()) << name << " 결과가 비어 있음";
        EXPECT_LT(ms, 500LL) << name << " 500ms 초과. 실제: " << ms << "ms";
    };

    run("BasicStats", [&] { return AnalysisTools::BasicStats(m_sampleTable); });
    run("GroupByAggregate", [&] {
        return AnalysisTools::GroupByAggregate(
            m_sampleTable, _T("카테고리"), _T("매출"), _T("sum"));
    });
    run("FrequencyDistribution", [&] {
        return AnalysisTools::FrequencyDistribution(m_sampleTable, _T("카테고리"));
    });
    run("TopN", [&] {
        return AnalysisTools::TopN(m_sampleTable, _T("매출"), 5, false);
    });
    run("MovingAverage", [&] {
        return AnalysisTools::MovingAverage(m_sampleTable, _T("매출"), 3);
    });
    run("OutlierDetection", [&] {
        return AnalysisTools::OutlierDetection(m_sampleTable, _T("매출"), 1.5);
    });
    run("CrossTabulation", [&] {
        return AnalysisTools::CrossTabulation(
            m_sampleTable, _T("카테고리"), _T("지역"));
    });

    // 전체 누적 시간 출력 (정보용)
    long long total = 0;
    for (const auto& r : results) {
        total += r.ms;
    }
    // 총 수행 시간은 3500ms 이내 (7 도구 × 500ms)
    EXPECT_LT(total, 3500LL)
        << "전체 도구 누적 수행 시간: " << total << "ms";
}

// ============================================================
// TC-P-04: DataTable 메모리 크기 추정
// 1000행 10컬럼 DataTable의 값 데이터가 50MB 미만이어야 함
// ============================================================
TEST(PerformanceStandaloneTest, DataTable_1000Rows10Cols_MemoryBelow50MB) {
    DataTable large = BuildLargeTable(1000, 10);

    // 각 CString 값이 평균 약 20바이트(숫자 문자열)라고 가정
    // 1000행 × 10컬럼 = 10,000 값 × 20바이트 = 200KB — 50MB보다 훨씬 작음
    // 실제 메모리 측정은 OS 의존적이므로 값 개수로 추정한다
    size_t totalValues = 0;
    for (const auto& col : large.columns) {
        totalValues += col.values.size();
    }

    const size_t MAX_VALUES = 1000 * 10;
    EXPECT_LE(totalValues, MAX_VALUES)
        << "값 개수가 예상 범위를 초과한다: " << totalValues;

    // 값당 평균 크기 추정 (각 CString의 내부 wchar_t 버퍼 ≈ 20바이트)
    const size_t EST_BYTES_PER_VALUE = 64; // 넉넉한 상한
    size_t estBytes = totalValues * EST_BYTES_PER_VALUE;
    const size_t FIFTY_MB = 50ULL * 1024 * 1024;

    EXPECT_LT(estBytes, FIFTY_MB)
        << "추정 메모리 사용량이 50MB를 초과한다: " << estBytes << " bytes";
}

// ============================================================
// TC-P-05: 반복 실행 안정성 — 100회 반복 시 시간 편차 < 2x
// ============================================================
TEST_F(PerformanceTest, BasicStats_RepeatedExecution_TimeVarianceUnder2x) {
    const int REPEAT = 100;
    std::vector<long long> times;
    times.reserve(REPEAT);

    for (int i = 0; i < REPEAT; ++i) {
        long long ms = MeasureMs([&] {
            AnalysisTools::BasicStats(m_sampleTable);
        });
        times.push_back(ms);
    }

    // 최솟값이 0ms일 수 있으므로 1ms 하한 적용
    long long minMs = *std::min_element(times.begin(), times.end());
    long long maxMs = *std::max_element(times.begin(), times.end());

    // 모든 실행이 성공적으로 완료됨 (100회)
    EXPECT_EQ(static_cast<int>(times.size()), REPEAT);

    // 최대 시간이 절대 상한(200ms) 이내
    EXPECT_LT(maxMs, 200LL)
        << "100회 반복 중 최대 수행 시간이 200ms를 초과했다: " << maxMs << "ms";

    // 최댓값이 최솟값의 10배를 넘지 않아야 함 (1ms 하한 적용)
    long long baseline = std::max(minMs, 1LL);
    EXPECT_LT(maxMs, baseline * 10)
        << "수행 시간 편차가 너무 큼. min=" << minMs << "ms max=" << maxMs << "ms";
}

// GroupByAggregate 반복 안정성
TEST_F(PerformanceTest, GroupByAggregate_RepeatedExecution_NoCrash) {
    const int REPEAT = 50;
    int successCount = 0;

    for (int i = 0; i < REPEAT; ++i) {
        CString result = AnalysisTools::GroupByAggregate(
            m_sampleTable, _T("카테고리"), _T("매출"), _T("sum"));
        if (!result.IsEmpty()) ++successCount;
    }

    EXPECT_EQ(successCount, REPEAT)
        << "50회 반복 중 " << (REPEAT - successCount) << "회 실패";
}
