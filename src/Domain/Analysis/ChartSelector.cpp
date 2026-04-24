#include "stdafx.h"
#include "ChartSelector.h"

// ============================================================
// ChartSelector 구현 — 규칙 기반, LLM 미사용
// Architecture §5.5 / DetailedSpec §7.4 참조
// ============================================================

void ChartSelector::SelectChart(const CString&   toolName,
                                const CString&   analysisResult,
                                const DataTable& data,
                                ChartConfig&     outConfig)
{
    // 1. 도구명 기반 기본 차트 유형 결정
    CString chartType = MapToolToChartType(toolName);

    // 2. FrequencyDistribution은 항목 수로 Pie/Bar 재결정
    if (toolName.CompareNoCase(_T("FrequencyDistribution")) == 0)
        chartType = SelectFreqChartType(analysisResult);

    // 3. ChartConfig 채우기
    outConfig.chartType = chartType;
    outConfig.title     = BuildDefaultTitle(toolName, data);
    outConfig.dataJson  = analysisResult;

    // 축 레이블 기본값 설정
    if (chartType == _T("line") || chartType == _T("bar")) {
        outConfig.xLabel = _T("범주");
        outConfig.yLabel = _T("값");
    } else if (chartType == _T("scatter")) {
        outConfig.xLabel = _T("X");
        outConfig.yLabel = _T("Y");
    }
}

CString ChartSelector::MapToolToChartType(const CString& toolName)
{
    // 규칙 매핑 테이블 (Architecture §5.5)
    if (toolName.CompareNoCase(_T("GroupByAggregate")) == 0)    return _T("bar");
    if (toolName.CompareNoCase(_T("TimeSeriesAnalysis")) == 0)  return _T("line");
    if (toolName.CompareNoCase(_T("DateGroupAggregate")) == 0)  return _T("line");
    if (toolName.CompareNoCase(_T("CorrelationMatrix")) == 0)   return _T("scatter");
    if (toolName.CompareNoCase(_T("FrequencyDistribution")) == 0) return _T("bar"); // 재결정됨
    if (toolName.CompareNoCase(_T("CrossTabulation")) == 0)     return _T("bar");
    if (toolName.CompareNoCase(_T("MovingAverage")) == 0)       return _T("line");
    if (toolName.CompareNoCase(_T("PivotTable")) == 0)          return _T("bar");
    if (toolName.CompareNoCase(_T("TopN")) == 0)                return _T("bar");
    if (toolName.CompareNoCase(_T("Percentile")) == 0)          return _T("bar");
    if (toolName.CompareNoCase(_T("OutlierDetection")) == 0)    return _T("scatter");
    if (toolName.CompareNoCase(_T("Filtering")) == 0)           return _T("bar");
    if (toolName.CompareNoCase(_T("BasicStats")) == 0)          return _T("bar");

    return _T("bar"); // 기본값
}

CString ChartSelector::SelectFreqChartType(const CString& analysisResult)
{
    // JSON에서 total_unique 값 추출하여 5 이하이면 pie, 초과이면 bar
    int pos = analysisResult.Find(_T("\"total_unique\":"));
    if (pos < 0) return _T("bar");

    pos += (int)_tcslen(_T("\"total_unique\":"));
    CString numStr;
    while (pos < analysisResult.GetLength() &&
           _istdigit(analysisResult[pos])) {
        numStr += analysisResult[pos++];
    }
    int total = _ttoi(numStr);
    return (total <= 5) ? _T("pie") : _T("bar");
}

CString ChartSelector::BuildDefaultTitle(const CString& toolName, const DataTable& data)
{
    // 도구명 기반 기본 제목 생성 (파일명 참조)
    CString fileName = data.fileName;
    int lastSlash = max(fileName.ReverseFind(_T('\\')),
                        fileName.ReverseFind(_T('/')));
    if (lastSlash >= 0) fileName = fileName.Mid(lastSlash + 1);

    if (toolName.CompareNoCase(_T("GroupByAggregate")) == 0)    return _T("그룹별 집계");
    if (toolName.CompareNoCase(_T("TimeSeriesAnalysis")) == 0)  return _T("시계열 분석");
    if (toolName.CompareNoCase(_T("DateGroupAggregate")) == 0)  return _T("기간별 추이");
    if (toolName.CompareNoCase(_T("CorrelationMatrix")) == 0)   return _T("상관관계 분석");
    if (toolName.CompareNoCase(_T("FrequencyDistribution")) == 0) return _T("빈도 분포");
    if (toolName.CompareNoCase(_T("CrossTabulation")) == 0)     return _T("교차 분석");
    if (toolName.CompareNoCase(_T("MovingAverage")) == 0)       return _T("이동평균");
    if (toolName.CompareNoCase(_T("PivotTable")) == 0)          return _T("피벗 테이블");
    if (toolName.CompareNoCase(_T("TopN")) == 0)                return _T("상위 항목");
    if (toolName.CompareNoCase(_T("OutlierDetection")) == 0)    return _T("이상치 탐지");
    if (toolName.CompareNoCase(_T("BasicStats")) == 0)          return _T("기본 통계");

    return toolName; // 알 수 없는 도구명은 그대로 사용
}
