#pragma once

// 차트 유형 자동 선택 (규칙 기반, LLM 미사용)
// Architecture §5.5 / DetailedSpec §4 참조

#include "../../Common/Types.h"

// ============================================================
// ChartSelector — 분석 도구명 + 결과 기반 차트 유형 결정
// ============================================================
class ChartSelector {
public:
    // 분석 도구명, 분석 결과 JSON, 원본 데이터로 ChartConfig 결정
    static void SelectChart(const CString&    toolName,
                            const CString&    analysisResult,
                            const DataTable&  data,
                            ChartConfig&      outConfig);

private:
    // 도구명으로 기본 차트 유형 결정
    static CString MapToolToChartType(const CString& toolName);

    // FrequencyDistribution 결과: 항목 수에 따라 Pie/Bar 선택
    static CString SelectFreqChartType(const CString& analysisResult);

    // 결과 JSON에서 title 후보 생성
    static CString BuildDefaultTitle(const CString& toolName, const DataTable& data);
};
