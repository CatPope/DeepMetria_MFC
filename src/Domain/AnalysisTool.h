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

    // 도구 카탈로그 — LLM에 노출 가능한 함수 메타데이터
    struct ToolDescriptor
    {
        std::wstring name;
        std::wstring description;
        std::wstring jsonParameters;  // JSON Schema 문자열
    };
    static const std::vector<ToolDescriptor>& Catalog();
};

} // namespace deepmetria
