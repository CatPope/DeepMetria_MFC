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

    // 4. 분석 결과 → 차트 데이터 형식으로 변환
    outConfig.dataJson  = ConvertToChartData(toolName, analysisResult);

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

// ============================================================
// BuildChartJson — 표준 차트 JSON 포맷 생성 헬퍼
// ============================================================
/*static*/ CString ChartSelector::BuildChartJson(const CString& labels,
                                                  const CString& values,
                                                  const CString& datasetName)
{
    CString result;
    result.Format(_T("{\"labels\":[%s],\"datasets\":[{\"name\":\"%s\",\"values\":[%s]}]}"),
                  (LPCTSTR)labels, (LPCTSTR)datasetName, (LPCTSTR)values);
    return result;
}

// ============================================================
// ConvertToChartData — 분석 결과 → 차트 데이터 JSON 변환
// ============================================================
CString ChartSelector::ConvertToChartData(const CString& toolName, const CString& analysisResult)
{
    // 이미 차트 형식이면 그대로 반환
    if (analysisResult.Find(_T("\"labels\"")) >= 0 &&
        analysisResult.Find(_T("\"datasets\"")) >= 0)
        return analysisResult;

    // JSON에서 문자열 값 추출 헬퍼
    auto extractStr = [&](const CString& json, const CString& key, int from) -> CString {
        CString search = _T("\"") + key + _T("\":\"");
        int p = json.Find(search, from);
        if (p < 0) return _T("");
        p += search.GetLength();
        int q = json.Find(_T("\""), p);
        return (q > p) ? json.Mid(p, q - p) : _T("");
    };

    // JSON에서 숫자 값 추출 헬퍼
    auto extractNum = [&](const CString& json, const CString& key, int from) -> double {
        CString search = _T("\"") + key + _T("\":");
        int p = json.Find(search, from);
        if (p < 0) return 0.0;
        p += search.GetLength();
        return _tcstod(json.Mid(p), nullptr);
    };

    CString labels;
    CString values;

    // --- BasicStats: {"stats":[{"column":"col","mean":X,...},...]} ---
    if (toolName.CompareNoCase(_T("BasicStats")) == 0)
    {
        int pos = 0;
        bool first = true;
        while ((pos = analysisResult.Find(_T("\"column\":\""), pos)) >= 0)
        {
            CString col = extractStr(analysisResult, _T("column"), pos);
            double mean = extractNum(analysisResult, _T("mean"), pos);

            if (!first) { labels += _T(","); values += _T(","); }
            first = false;
            labels += _T("\"") + col + _T("\"");

            CString v;
            v.Format(_T("%.2f"), mean);
            values += v;
            pos += 10;
        }

        return BuildChartJson(labels, values, _T("평균"));
    }

    // --- GroupByAggregate / TopN: {"results":[{"group":"X","value":Y},...]} ---
    if (toolName.CompareNoCase(_T("GroupByAggregate")) == 0 ||
        toolName.CompareNoCase(_T("TopN")) == 0 ||
        toolName.CompareNoCase(_T("Filtering")) == 0)
    {
        // "group" 또는 "label" 또는 첫번째 문자열 키 탐색
        int pos = 0;
        bool first = true;
        while ((pos = analysisResult.Find(_T("{"), pos + 1)) >= 0)
        {
            // results 배열 내부의 각 객체
            CString grp = extractStr(analysisResult, _T("group"), pos);
            if (grp.IsEmpty()) grp = extractStr(analysisResult, _T("label"), pos);
            if (grp.IsEmpty()) grp = extractStr(analysisResult, _T("key"), pos);

            double val = extractNum(analysisResult, _T("value"), pos);
            if (val == 0.0) val = extractNum(analysisResult, _T("agg_value"), pos);
            if (val == 0.0) val = extractNum(analysisResult, _T("count"), pos);

            if (grp.IsEmpty()) continue;

            if (!first) { labels += _T(","); values += _T(","); }
            first = false;
            labels += _T("\"") + grp + _T("\"");

            CString v;
            v.Format(_T("%.2f"), val);
            values += v;
        }

        if (!labels.IsEmpty())
        {
            CString dsName = extractStr(analysisResult, _T("agg_func"), 0);
            if (dsName.IsEmpty()) dsName = _T("값");
            return BuildChartJson(labels, values, dsName);
        }
    }

    // --- FrequencyDistribution: {"distribution":[{"value":"X","count":N},...]} ---
    if (toolName.CompareNoCase(_T("FrequencyDistribution")) == 0)
    {
        int pos = 0;
        bool first = true;
        while ((pos = analysisResult.Find(_T("{"), pos + 1)) >= 0)
        {
            CString val = extractStr(analysisResult, _T("value"), pos);
            double cnt = extractNum(analysisResult, _T("count"), pos);

            if (val.IsEmpty()) continue;

            if (!first) { labels += _T(","); values += _T(","); }
            first = false;
            labels += _T("\"") + val + _T("\"");

            CString v;
            v.Format(_T("%.0f"), cnt);
            values += v;
        }

        if (!labels.IsEmpty())
            return BuildChartJson(labels, values, _T("빈도"));
    }

    // --- Percentile: {"percentiles":[{"p":25,"value":X},...]} ---
    if (toolName.CompareNoCase(_T("Percentile")) == 0)
    {
        int pos = 0;
        bool first = true;
        while ((pos = analysisResult.Find(_T("{"), pos + 1)) >= 0)
        {
            double p = extractNum(analysisResult, _T("p"), pos);
            double val = extractNum(analysisResult, _T("value"), pos);

            if (p == 0.0 && val == 0.0) continue;

            if (!first) { labels += _T(","); values += _T(","); }
            first = false;

            CString pLabel;
            pLabel.Format(_T("\"P%.0f\""), p);
            labels += pLabel;

            CString v;
            v.Format(_T("%.2f"), val);
            values += v;
        }

        if (!labels.IsEmpty())
            return BuildChartJson(labels, values, _T("백분위"));
    }

    // 변환 불가 시 원본 반환
    return analysisResult;
}
