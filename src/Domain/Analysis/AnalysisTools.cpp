#include "stdafx.h"
#include "AnalysisTools.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

// ============================================================
// 내부 유틸리티
// ============================================================

int AnalysisTools::FindColumnIndex(const DataTable& data, const CString& colName)
{
    for (int i = 0; i < static_cast<int>(data.columns.size()); ++i) {
        if (data.columns[i].name.CompareNoCase(colName) == 0)
            return i;
    }
    return -1;
}

std::vector<double> AnalysisTools::ExtractNumericValues(const DataColumn& col)
{
    std::vector<double> result;
    result.reserve(col.values.size());
    for (const auto& v : col.values) {
        if (v.IsEmpty()) continue;
        TCHAR* end = nullptr;
        double d = _tcstod(v, &end);
        if (end && *end == _T('\0'))
            result.push_back(d);
    }
    return result;
}

double AnalysisTools::CalcMean(const std::vector<double>& values)
{
    if (values.empty()) return 0.0;
    double sum = 0.0;
    for (double v : values) sum += v;
    return sum / static_cast<double>(values.size());
}

double AnalysisTools::CalcMedian(std::vector<double> values)
{
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    size_t n = values.size();
    return (n % 2 == 0)
        ? (values[n / 2 - 1] + values[n / 2]) / 2.0
        : values[n / 2];
}

double AnalysisTools::CalcStdDev(const std::vector<double>& values, double mean)
{
    if (values.size() < 2) return 0.0;
    double sumSq = 0.0;
    for (double v : values) {
        double d = v - mean;
        sumSq += d * d;
    }
    return std::sqrt(sumSq / static_cast<double>(values.size() - 1));
}

CString AnalysisTools::EscapeJsonString(const CString& s)
{
    CString result;
    for (int i = 0; i < s.GetLength(); ++i) {
        TCHAR c = s[i];
        if      (c == _T('"'))  result += _T("\\\"");
        else if (c == _T('\\')) result += _T("\\\\");
        else if (c == _T('\n')) result += _T("\\n");
        else if (c == _T('\r')) result += _T("\\r");
        else if (c == _T('\t')) result += _T("\\t");
        else                    result += c;
    }
    return result;
}

// ============================================================
// BasicStats — 모든 수치 컬럼에 대한 기본 통계
// ============================================================
CString AnalysisTools::BasicStats(const DataTable& data)
{
    CString json = _T("{\"stats\":[");
    bool first = true;

    for (const auto& col : data.columns) {
        if (col.type != _T("numeric")) continue;

        std::vector<double> nums = ExtractNumericValues(col);
        int nullCount = static_cast<int>(col.values.size()) - static_cast<int>(nums.size());
        double mean   = CalcMean(nums);
        double median = CalcMedian(nums);
        double stddev = CalcStdDev(nums, mean);
        double minVal = nums.empty() ? 0.0 : *std::min_element(nums.begin(), nums.end());
        double maxVal = nums.empty() ? 0.0 : *std::max_element(nums.begin(), nums.end());

        if (!first) json += _T(",");
        first = false;

        CString entry;
        entry.Format(_T("{\"column\":\"%s\",\"count\":%d,\"null_count\":%d,"
                        "\"mean\":%.4f,\"median\":%.4f,\"std\":%.4f,"
                        "\"min\":%.4f,\"max\":%.4f}"),
                     (LPCTSTR)EscapeJsonString(col.name),
                     (int)nums.size(), nullCount,
                     mean, median, stddev, minVal, maxVal);
        json += entry;
    }

    json += _T("]}");
    return json;
}

// ============================================================
// FormatGroupResults — GroupByAggregate 집계 결과 JSON 직렬화 헬퍼
// ============================================================
/*static*/ CString AnalysisTools::FormatGroupResults(
    const std::map<CString, std::vector<double>>& groups,
    const CString& aggFunc)
{
    CString json;
    bool first = true;

    for (const auto& kv : groups) {
        const std::vector<double>& vals = kv.second;
        double aggVal = 0.0;

        if (aggFunc.CompareNoCase(_T("sum")) == 0) {
            for (double v : vals) aggVal += v;
        } else if (aggFunc.CompareNoCase(_T("avg")) == 0) {
            aggVal = CalcMean(vals);
        } else if (aggFunc.CompareNoCase(_T("count")) == 0) {
            aggVal = static_cast<double>(vals.size());
        } else if (aggFunc.CompareNoCase(_T("min")) == 0) {
            aggVal = *std::min_element(vals.begin(), vals.end());
        } else if (aggFunc.CompareNoCase(_T("max")) == 0) {
            aggVal = *std::max_element(vals.begin(), vals.end());
        } else {
            for (double v : vals) aggVal += v; // 기본 sum
        }

        if (!first) json += _T(",");
        first = false;
        CString entry;
        entry.Format(_T("{\"group\":\"%s\",\"value\":%.4f}"),
                     (LPCTSTR)EscapeJsonString(kv.first), aggVal);
        json += entry;
    }

    return json;
}

// ============================================================
// GroupByAggregate
// ============================================================
CString AnalysisTools::GroupByAggregate(const DataTable& data,
                                        const CString&   groupCol,
                                        const CString&   valueCol,
                                        const CString&   aggFunc)
{
    int gi = FindColumnIndex(data, groupCol);
    int vi = FindColumnIndex(data, valueCol);
    if (gi < 0 || vi < 0)
        return _T("{\"error\":\"column not found\"}");

    // 그룹별 값 수집
    std::map<CString, std::vector<double>> groups;
    const auto& gVals = data.columns[gi].values;
    const auto& vVals = data.columns[vi].values;
    int n = min((int)gVals.size(), (int)vVals.size());

    for (int i = 0; i < n; ++i) {
        TCHAR* end = nullptr;
        double d = _tcstod(vVals[i], &end);
        if (end && *end == _T('\0'))
            groups[gVals[i]].push_back(d);
    }

    CString json = _T("{\"group_col\":\"") + EscapeJsonString(groupCol)
                 + _T("\",\"value_col\":\"") + EscapeJsonString(valueCol)
                 + _T("\",\"agg_func\":\"") + EscapeJsonString(aggFunc)
                 + _T("\",\"results\":[");
    json += FormatGroupResults(groups, aggFunc);
    json += _T("]}");
    return json;
}

// ============================================================
// TimeSeriesAnalysis — 날짜/값 컬럼 시계열 집계
// ============================================================
CString AnalysisTools::TimeSeriesAnalysis(const DataTable& data,
                                          const CString&   dateCol,
                                          const CString&   valueCol)
{
    // DateGroupAggregate(month) 위임
    return DateGroupAggregate(data, dateCol, valueCol, _T("month"));
}

// ============================================================
// CalcCorrelation — 피어슨 상관계수 계산 헬퍼
// ============================================================
/*static*/ double AnalysisTools::CalcCorrelation(const std::vector<double>& x,
                                                  const std::vector<double>& y)
{
    double meanX = CalcMean(x);
    double meanY = CalcMean(y);
    size_t minLen = min(x.size(), y.size());
    double num = 0.0, dx = 0.0, dy = 0.0;
    for (size_t k = 0; k < minLen; ++k) {
        double a = x[k] - meanX;
        double b = y[k] - meanY;
        num += a * b;
        dx  += a * a;
        dy  += b * b;
    }
    return (dx > 0.0 && dy > 0.0) ? num / std::sqrt(dx * dy) : 0.0;
}

// ============================================================
// CorrelationMatrix — 피어슨 상관계수 행렬
// ============================================================
CString AnalysisTools::CorrelationMatrix(const DataTable&         data,
                                         const std::vector<CString>& numericCols)
{
    // 각 컬럼의 수치 값 수집
    std::vector<std::vector<double>> colData;
    std::vector<CString> validCols;

    for (const auto& colName : numericCols) {
        int idx = FindColumnIndex(data, colName);
        if (idx < 0) continue;
        auto vals = ExtractNumericValues(data.columns[idx]);
        if (!vals.empty()) {
            colData.push_back(std::move(vals));
            validCols.push_back(colName);
        }
    }

    size_t nc = validCols.size();
    CString json = _T("{\"columns\":[");
    for (size_t i = 0; i < nc; ++i) {
        if (i > 0) json += _T(",");
        json += _T("\"") + EscapeJsonString(validCols[i]) + _T("\"");
    }
    json += _T("],\"matrix\":[");

    for (size_t i = 0; i < nc; ++i) {
        if (i > 0) json += _T(",");
        json += _T("[");

        for (size_t j = 0; j < nc; ++j) {
            if (j > 0) json += _T(",");
            if (i == j) {
                json += _T("1.0000");
                continue;
            }
            double corr = CalcCorrelation(colData[i], colData[j]);
            CString val;
            val.Format(_T("%.4f"), corr);
            json += val;
        }
        json += _T("]");
    }

    json += _T("]}");
    return json;
}

// ============================================================
// TopN — 컬럼 기준 상위 N 행
// ============================================================
CString AnalysisTools::TopN(const DataTable& data,
                             const CString&   col,
                             int              n,
                             bool             ascending)
{
    int ci = FindColumnIndex(data, col);
    if (ci < 0) return _T("{\"error\":\"column not found\"}");

    int rowCount = data.rowCount;
    // 인덱스 벡터 생성 후 정렬
    std::vector<int> indices(rowCount);
    for (int i = 0; i < rowCount; ++i) indices[i] = i;

    const auto& sortVals = data.columns[ci].values;
    std::stable_sort(indices.begin(), indices.end(),
        [&](int a, int b) {
            TCHAR* ea = nullptr, *eb = nullptr;
            double da = (a < (int)sortVals.size()) ? _tcstod(sortVals[a], &ea) : 0.0;
            double db = (b < (int)sortVals.size()) ? _tcstod(sortVals[b], &eb) : 0.0;
            return ascending ? (da < db) : (da > db);
        });

    int take = min(n, rowCount);
    CString json = _T("{\"sort_col\":\"") + EscapeJsonString(col)
                 + _T("\",\"ascending\":") + (ascending ? _T("true") : _T("false"))
                 + _T(",\"rows\":[");

    for (int r = 0; r < take; ++r) {
        if (r > 0) json += _T(",");
        int idx = indices[r];
        json += _T("{");
        bool firstCol = true;
        for (const auto& c : data.columns) {
            if (!firstCol) json += _T(",");
            firstCol = false;
            CString val = (idx < (int)c.values.size()) ? c.values[idx] : _T("");
            json += _T("\"") + EscapeJsonString(c.name) + _T("\":\"")
                  + EscapeJsonString(val) + _T("\"");
        }
        json += _T("}");
    }

    json += _T("]}");
    return json;
}

// ============================================================
// FrequencyDistribution — 값별 빈도 집계
// ============================================================
CString AnalysisTools::FrequencyDistribution(const DataTable& data,
                                              const CString&   col)
{
    int ci = FindColumnIndex(data, col);
    if (ci < 0) return _T("{\"error\":\"column not found\"}");

    std::map<CString, int> freq;
    for (const auto& v : data.columns[ci].values) {
        if (!v.IsEmpty()) freq[v]++;
    }

    // 빈도 내림차순 정렬
    std::vector<std::pair<CString, int>> sorted(freq.begin(), freq.end());
    std::sort(sorted.begin(), sorted.end(),
        [](const std::pair<CString, int>& a, const std::pair<CString, int>& b) {
            return a.second > b.second;
        });

    CString json = _T("{\"column\":\"") + EscapeJsonString(col)
                 + _T("\",\"total_unique\":") ;
    CString tmp;
    tmp.Format(_T("%d"), (int)sorted.size());
    json += tmp + _T(",\"items\":[");

    for (size_t i = 0; i < sorted.size(); ++i) {
        if (i > 0) json += _T(",");
        CString entry;
        entry.Format(_T("{\"value\":\"%s\",\"count\":%d}"),
                     (LPCTSTR)EscapeJsonString(sorted[i].first), sorted[i].second);
        json += entry;
    }

    json += _T("]}");
    return json;
}

// ============================================================
// CrossTabulation — 교차 분석 테이블
// ============================================================
CString AnalysisTools::CrossTabulation(const DataTable& data,
                                        const CString&   rowCol,
                                        const CString&   colCol)
{
    int ri = FindColumnIndex(data, rowCol);
    int ci = FindColumnIndex(data, colCol);
    if (ri < 0 || ci < 0) return _T("{\"error\":\"column not found\"}");

    // (rowVal, colVal) → count
    std::map<CString, std::map<CString, int>> table;
    std::map<CString, int> colHeaders;
    int n = min((int)data.columns[ri].values.size(),
                (int)data.columns[ci].values.size());

    for (int i = 0; i < n; ++i) {
        const CString& rv = data.columns[ri].values[i];
        const CString& cv = data.columns[ci].values[i];
        table[rv][cv]++;
        colHeaders[cv]++;
    }

    CString json = _T("{\"row_col\":\"") + EscapeJsonString(rowCol)
                 + _T("\",\"col_col\":\"") + EscapeJsonString(colCol)
                 + _T("\",\"columns\":[");
    bool first = true;
    for (const auto& ch : colHeaders) {
        if (!first) json += _T(",");
        first = false;
        json += _T("\"") + EscapeJsonString(ch.first) + _T("\"");
    }
    json += _T("],\"rows\":[");

    first = true;
    for (const auto& row : table) {
        if (!first) json += _T(",");
        first = false;
        json += _T("{\"row\":\"") + EscapeJsonString(row.first) + _T("\",\"counts\":{");
        bool firstC = true;
        for (const auto& ch : colHeaders) {
            if (!firstC) json += _T(",");
            firstC = false;
            auto it = row.second.find(ch.first);
            int cnt = (it != row.second.end()) ? it->second : 0;
            CString val;
            val.Format(_T("\"%s\":%d"), (LPCTSTR)EscapeJsonString(ch.first), cnt);
            json += val;
        }
        json += _T("}}");
    }

    json += _T("]}");
    return json;
}

// ============================================================
// MovingAverage — 이동평균
// ============================================================
CString AnalysisTools::MovingAverage(const DataTable& data,
                                      const CString&   col,
                                      int              window)
{
    int ci = FindColumnIndex(data, col);
    if (ci < 0) return _T("{\"error\":\"column not found\"}");

    std::vector<double> vals = ExtractNumericValues(data.columns[ci]);
    int n = static_cast<int>(vals.size());
    if (window < 1) window = 1;

    CString json = _T("{\"column\":\"") + EscapeJsonString(col);
    CString tmp;
    tmp.Format(_T("\",\"window\":%d,\"values\":["), window);
    json += tmp;

    for (int i = 0; i < n; ++i) {
        if (i > 0) json += _T(",");
        int start = max(0, i - window + 1);
        double sum = 0.0;
        for (int k = start; k <= i; ++k) sum += vals[k];
        double ma = sum / static_cast<double>(i - start + 1);
        CString val;
        val.Format(_T("{\"index\":%d,\"original\":%.4f,\"ma\":%.4f}"), i, vals[i], ma);
        json += val;
    }

    json += _T("]}");
    return json;
}

// ============================================================
// Percentile
// ============================================================
CString AnalysisTools::Percentile(const DataTable&         data,
                                   const CString&           col,
                                   const std::vector<double>& percentiles)
{
    int ci = FindColumnIndex(data, col);
    if (ci < 0) return _T("{\"error\":\"column not found\"}");

    std::vector<double> vals = ExtractNumericValues(data.columns[ci]);
    std::sort(vals.begin(), vals.end());
    int n = static_cast<int>(vals.size());

    CString json = _T("{\"column\":\"") + EscapeJsonString(col)
                 + _T("\",\"percentiles\":[");
    for (size_t i = 0; i < percentiles.size(); ++i) {
        if (i > 0) json += _T(",");
        double p = percentiles[i];
        double pos = p / 100.0 * (n - 1);
        int lo = (int)pos;
        int hi = min(lo + 1, n - 1);
        double frac = pos - lo;
        double val = (n > 0) ? vals[lo] * (1.0 - frac) + vals[hi] * frac : 0.0;
        CString entry;
        entry.Format(_T("{\"p\":%.1f,\"value\":%.4f}"), p, val);
        json += entry;
    }

    json += _T("]}");
    return json;
}

// ============================================================
// DateGroupAggregate — 날짜 기준 기간 집계
// ============================================================
CString AnalysisTools::DateGroupAggregate(const DataTable& data,
                                           const CString&   dateCol,
                                           const CString&   valueCol,
                                           const CString&   period)
{
    int di = FindColumnIndex(data, dateCol);
    int vi = FindColumnIndex(data, valueCol);
    if (di < 0 || vi < 0) return _T("{\"error\":\"column not found\"}");

    // 기간 키 추출 (YYYY-MM 또는 YYYY)
    std::map<CString, double> sumMap;
    std::map<CString, int>    cntMap;
    int n = min((int)data.columns[di].values.size(),
                (int)data.columns[vi].values.size());

    for (int i = 0; i < n; ++i) {
        const CString& dateStr = data.columns[di].values[i];
        const CString& valStr  = data.columns[vi].values[i];
        if (dateStr.IsEmpty() || valStr.IsEmpty()) continue;

        TCHAR* end = nullptr;
        double d = _tcstod(valStr, &end);
        if (!end || *end != _T('\0')) continue;

        // 기간 키 계산 (year: 4자리, month: 7자리 YYYY-MM, week/day: 10자리)
        CString key;
        if (period.CompareNoCase(_T("year")) == 0 && dateStr.GetLength() >= 4)
            key = dateStr.Left(4);
        else if (period.CompareNoCase(_T("month")) == 0 && dateStr.GetLength() >= 7)
            key = dateStr.Left(7);
        else
            key = dateStr.Left(min(10, dateStr.GetLength())); // day/week fallback

        sumMap[key] += d;
        cntMap[key]++;
    }

    CString json = _T("{\"date_col\":\"") + EscapeJsonString(dateCol)
                 + _T("\",\"value_col\":\"") + EscapeJsonString(valueCol)
                 + _T("\",\"period\":\"") + EscapeJsonString(period)
                 + _T("\",\"points\":[");
    bool first = true;
    for (const auto& kv : sumMap) {
        if (!first) json += _T(",");
        first = false;
        CString entry;
        entry.Format(_T("{\"period\":\"%s\",\"sum\":%.4f,\"count\":%d,\"avg\":%.4f}"),
                     (LPCTSTR)EscapeJsonString(kv.first),
                     kv.second, cntMap[kv.first],
                     kv.second / cntMap[kv.first]);
        json += entry;
    }

    json += _T("]}");
    return json;
}

// ============================================================
// Filtering — 조건 기반 행 필터링 후 기본 통계
// ============================================================
CString AnalysisTools::Filtering(const DataTable& data,
                                  const CString&   col,
                                  const CString&   op,
                                  const CString&   value)
{
    int ci = FindColumnIndex(data, col);
    if (ci < 0) return _T("{\"error\":\"column not found\"}");

    double threshold = _tcstod(value, nullptr);
    std::vector<int> matchRows;

    for (int i = 0; i < (int)data.columns[ci].values.size(); ++i) {
        const CString& cv = data.columns[ci].values[i];
        TCHAR* end = nullptr;
        double d = _tcstod(cv, &end);
        bool isNum = (end && *end == _T('\0'));

        bool match = false;
        if (op == _T("==") || op == _T("=")) {
            match = isNum ? (d == threshold) : (cv.CompareNoCase(value) == 0);
        } else if (op == _T("!=")) {
            match = isNum ? (d != threshold) : (cv.CompareNoCase(value) != 0);
        } else if (op == _T(">"))  { match = isNum && d >  threshold; }
        else if (op == _T(">="))   { match = isNum && d >= threshold; }
        else if (op == _T("<"))    { match = isNum && d <  threshold; }
        else if (op == _T("<="))   { match = isNum && d <= threshold; }

        if (match) matchRows.push_back(i);
    }

    CString json;
    json.Format(_T("{\"column\":\"%s\",\"op\":\"%s\",\"value\":\"%s\","
                   "\"matched_count\":%d,\"total_count\":%d}"),
                (LPCTSTR)EscapeJsonString(col),
                (LPCTSTR)EscapeJsonString(op),
                (LPCTSTR)EscapeJsonString(value),
                (int)matchRows.size(), data.rowCount);
    return json;
}

// ============================================================
// FormatPivotResults — PivotTable 집계 결과 JSON 직렬화 헬퍼
// ============================================================
/*static*/ CString AnalysisTools::FormatPivotResults(
    const std::map<CString, std::map<CString, std::vector<double>>>& pivot,
    const CString& aggFunc)
{
    // colHeaders를 pivot에서 재수집
    std::map<CString, int> colHeaders;
    for (const auto& row : pivot)
        for (const auto& cell : row.second)
            colHeaders[cell.first]++;

    CString json;
    bool first = true;
    for (const auto& row : pivot) {
        if (!first) json += _T(",");
        first = false;
        json += _T("{\"row\":\"") + EscapeJsonString(row.first) + _T("\",\"values\":{");
        bool firstC = true;
        for (const auto& ch : colHeaders) {
            if (!firstC) json += _T(",");
            firstC = false;
            auto it = row.second.find(ch.first);
            double aggVal = 0.0;
            if (it != row.second.end()) {
                const auto& vals = it->second;
                if (aggFunc.CompareNoCase(_T("sum")) == 0)
                    for (double v : vals) aggVal += v;
                else if (aggFunc.CompareNoCase(_T("avg")) == 0)
                    aggVal = CalcMean(vals);
                else if (aggFunc.CompareNoCase(_T("count")) == 0)
                    aggVal = static_cast<double>(vals.size());
                else if (aggFunc.CompareNoCase(_T("min")) == 0)
                    aggVal = *std::min_element(vals.begin(), vals.end());
                else if (aggFunc.CompareNoCase(_T("max")) == 0)
                    aggVal = *std::max_element(vals.begin(), vals.end());
                else
                    for (double v : vals) aggVal += v;
            }
            CString val;
            val.Format(_T("\"%s\":%.4f"), (LPCTSTR)EscapeJsonString(ch.first), aggVal);
            json += val;
        }
        json += _T("}}");
    }
    return json;
}

// ============================================================
// PivotTable
// ============================================================
CString AnalysisTools::PivotTable(const DataTable& data,
                                   const CString&   rowCol,
                                   const CString&   colCol,
                                   const CString&   valueCol,
                                   const CString&   aggFunc)
{
    // CrossTabulation과 유사하되, 수치 집계
    int ri = FindColumnIndex(data, rowCol);
    int ci = FindColumnIndex(data, colCol);
    int vi = FindColumnIndex(data, valueCol);
    if (ri < 0 || ci < 0 || vi < 0) return _T("{\"error\":\"column not found\"}");

    std::map<CString, std::map<CString, std::vector<double>>> cells;
    std::map<CString, int> colHeaders;

    int n = (std::min)((int)data.columns[ri].values.size(),
             (std::min)((int)data.columns[ci].values.size(),
                        (int)data.columns[vi].values.size()));

    for (int i = 0; i < n; ++i) {
        const CString& rv = data.columns[ri].values[i];
        const CString& cv = data.columns[ci].values[i];
        const CString& vv = data.columns[vi].values[i];
        TCHAR* end = nullptr;
        double d = _tcstod(vv, &end);
        if (end && *end == _T('\0')) {
            cells[rv][cv].push_back(d);
            colHeaders[cv]++;
        }
    }

    CString json = _T("{\"row_col\":\"") + EscapeJsonString(rowCol)
                 + _T("\",\"col_col\":\"") + EscapeJsonString(colCol)
                 + _T("\",\"value_col\":\"") + EscapeJsonString(valueCol)
                 + _T("\",\"agg_func\":\"") + EscapeJsonString(aggFunc)
                 + _T("\",\"columns\":[");
    bool first = true;
    for (const auto& ch : colHeaders) {
        if (!first) json += _T(",");
        first = false;
        json += _T("\"") + EscapeJsonString(ch.first) + _T("\"");
    }
    json += _T("],\"rows\":[");
    json += FormatPivotResults(cells, aggFunc);
    json += _T("]}");
    return json;
}

// ============================================================
// OutlierDetection — IQR 기반 이상치 탐지
// ============================================================
CString AnalysisTools::OutlierDetection(const DataTable& data,
                                         const CString&   col,
                                         double           threshold)
{
    int ci = FindColumnIndex(data, col);
    if (ci < 0) return _T("{\"error\":\"column not found\"}");

    std::vector<double> vals = ExtractNumericValues(data.columns[ci]);
    if (vals.empty()) return _T("{\"error\":\"no numeric values\"}");

    std::vector<double> sorted = vals;
    std::sort(sorted.begin(), sorted.end());
    int n = static_cast<int>(sorted.size());

    // Q1, Q3 계산
    double q1 = sorted[n / 4];
    double q3 = sorted[3 * n / 4];
    double iqr = q3 - q1;
    double lower = q1 - threshold * iqr;
    double upper = q3 + threshold * iqr;

    std::vector<double> outliers;
    for (double v : vals) {
        if (v < lower || v > upper)
            outliers.push_back(v);
    }

    CString json;
    json.Format(_T("{\"column\":\"%s\",\"threshold\":%.2f,"
                   "\"q1\":%.4f,\"q3\":%.4f,\"iqr\":%.4f,"
                   "\"lower_bound\":%.4f,\"upper_bound\":%.4f,"
                   "\"outlier_count\":%d,\"total_count\":%d,\"outliers\":["),
                (LPCTSTR)EscapeJsonString(col),
                threshold, q1, q3, iqr, lower, upper,
                (int)outliers.size(), n);

    for (size_t i = 0; i < outliers.size(); ++i) {
        if (i > 0) json += _T(",");
        CString v;
        v.Format(_T("%.4f"), outliers[i]);
        json += v;
    }
    json += _T("]}");
    return json;
}
