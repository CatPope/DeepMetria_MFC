// Domain/AnalysisTool.h - LLM이 호출 가능한 미리 정의된 분석 도구
#pragma once
#include "DataSource.h"
#include "Visualization.h"
#include <string>
#include <vector>
#include <optional>

namespace deepmetria {

class AnalysisTool
{
public:
    // 통계: 행 수/결측치/수치 통계
    static void ComputeBasicStatistics(DataSource& ds);

    // 컬럼 타입 추정 (수치/날짜/텍스트)
    static void InferSchemaTypes(DataSource& ds);

    // 그룹별 합계: groupColumn 기준 valueColumn 합계
    static std::optional<Visualization>
    GroupBySum(const DataSource& ds,
               const std::wstring& groupColumn,
               const std::wstring& valueColumn);

    // 시계열 추이: 날짜 컬럼 + 수치 컬럼 → 라인 차트
    static std::optional<Visualization>
    TrendOverTime(const DataSource& ds,
                  const std::wstring& dateColumn,
                  const std::wstring& valueColumn);

    // 분포(상위 N): 텍스트 컬럼 값 빈도 → 막대
    static std::optional<Visualization>
    TopNDistribution(const DataSource& ds,
                     const std::wstring& column,
                     int topN);

    // 표 형태로 원본 N행 출력 (기본 대시보드 구성용)
    static Visualization TableSample(const DataSource& ds, int rowLimit);

    // 데이터 요약 패널 (스키마/도메인/통계) — VizType::Summary
    static Visualization SummaryPanel(const DataSource& ds);

    // ─── Tier 1 신규 도구 ───
    // 조건 행 필터 — column [op] value 로 잘라 표로 미리보기
    // op: "==", "!=", ">", "<", ">=", "<=", "contains"
    static std::optional<Visualization>
    Filter(const DataSource& ds,
           const std::wstring& column,
           const std::wstring& op,
           const std::wstring& value,
           int rowLimit = 30);

    // 그룹별 집계 — func: sum/mean/median/min/max/count
    static std::optional<Visualization>
    Aggregate(const DataSource& ds,
              const std::wstring& groupColumn,
              const std::wstring& valueColumn,
              const std::wstring& func);

    // 그룹별 개수 (Aggregate 의 count 버전, 별도 노출용)
    static std::optional<Visualization>
    GroupByCount(const DataSource& ds, const std::wstring& groupColumn);

    // 그룹별 평균
    static std::optional<Visualization>
    GroupByMean(const DataSource& ds,
                const std::wstring& groupColumn,
                const std::wstring& valueColumn);

    // 단일 수치 컬럼의 평균/중앙/표준편차/min/max 요약 카드
    static std::optional<Visualization>
    SummaryStats(const DataSource& ds, const std::wstring& valueColumn);

    // 수치 컬럼 구간 히스토그램
    static std::optional<Visualization>
    Histogram(const DataSource& ds,
              const std::wstring& valueColumn,
              int binCount = 10);

    // 두 수치 컬럼 산점도
    static std::optional<Visualization>
    ScatterPlot(const DataSource& ds,
                const std::wstring& xColumn,
                const std::wstring& yColumn);

    // 단일 메트릭 KPI 카드 — func: sum/mean/median/min/max/count
    static std::optional<Visualization>
    KpiCard(const DataSource& ds,
            const std::wstring& valueColumn,
            const std::wstring& func);

    // Wide 형식 데이터의 다중 컬럼 비교 — 한 행의 여러 수치 컬럼을 막대로.
    // 예: compare_columns("연도","2010","중앙정부,지방자치단체,적자성 채무,금융성 채무")
    //     → 4개 막대 (각 컬럼명이 카테고리, 그 행의 값이 수치)
    // filterValue 가 비어있으면 전체 행의 합계 비교.
    static std::optional<Visualization>
    CompareColumns(const DataSource& ds,
                   const std::wstring& filterColumn,
                   const std::wstring& filterValue,
                   const std::wstring& valueColumnsCsv);

    // 특정 N개 행 + 단일 수치 컬럼 비교.
    // 예: compare_rows("연도","2010,2015,2020","중앙정부")
    //     → 3개 막대 (categories=2010,2015,2020, values=각 행의 중앙정부 값)
    static std::optional<Visualization>
    CompareRows(const DataSource& ds,
                const std::wstring& keyColumn,
                const std::wstring& keyValuesCsv,
                const std::wstring& valueColumn);

    // 일부 행 × 일부 컬럼 (multi-series).
    // 예: select_rows_columns("연도","2010,2015,2020","중앙정부,지방자치단체")
    //     → categories=2010/2015/2020, series=[중앙정부, 지방자치단체] 두 줄/막대 그룹.
    static std::optional<Visualization>
    SelectRowsColumns(const DataSource& ds,
                      const std::wstring& filterColumn,
                      const std::wstring& filterValuesCsv,
                      const std::wstring& valueColumnsCsv);

    // 도구 분류 — viz 생성형 vs 데이터 편집형 vs 단순 채팅
    enum class ToolKind
    {
        Visualization,  // 호출 시 viz 카드 생성
        DataEdit,       // 데이터셋 가공 (projection/pivot/join/merge 등) — viz 생성 안 함
        Chat            // 단순 대화 응답 (도구 호출 없음과 동일)
    };

    // 도구 카탈로그 — LLM에 노출 가능한 함수 메타데이터
    struct ToolDescriptor
    {
        std::wstring name;
        std::wstring description;
        std::wstring jsonParameters;  // JSON Schema 문자열
        ToolKind     kind = ToolKind::Visualization;
    };
    static const std::vector<ToolDescriptor>& Catalog();

    // 이름으로 도구 분류 조회 — AnalysisFlow 디스패치 가드용
    static ToolKind KindOf(const std::wstring& toolName);
};

} // namespace deepmetria
