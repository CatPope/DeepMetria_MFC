// Domain/AnalysisTool.cpp
#include "stdafx.h"
#include "AnalysisTool.h"
#include <algorithm>
#include <cwctype>
#include <unordered_map>
#include <sstream>
#include <cmath>

namespace deepmetria {

namespace {

bool TryParseNumber(const std::wstring& s, double& out)
{
    if (s.empty()) return false;
    wchar_t* end = nullptr;
    double v = std::wcstod(s.c_str(), &end);
    if (end == s.c_str()) return false;
    out = v;
    return true;
}

bool LooksLikeDate(const std::wstring& s)
{
    if (s.size() < 6) return false;
    int digits = 0, seps = 0;
    for (wchar_t c : s)
    {
        if (std::iswdigit(c)) ++digits;
        else if (c == L'-' || c == L'/' || c == L'.') ++seps;
        else if (c != L' ' && c != L':') return false;
    }
    return digits >= 4 && seps >= 1;
}

int FindColumnIndex(const DataSource& ds, const std::wstring& name)
{
    for (size_t i = 0; i < ds.Columns().size(); ++i)
        if (ds.Columns()[i].name == name) return static_cast<int>(i);
    return -1;
}

} // namespace

void AnalysisTool::ComputeBasicStatistics(DataSource& ds)
{
    auto& cols = ds.MutableColumns();
    const auto& rows = ds.Rows();

    for (size_t ci = 0; ci < cols.size(); ++ci)
    {
        auto& c = cols[ci];
        c.rowCount = static_cast<long long>(rows.size());
        c.missing  = 0;
        c.sum      = 0.0;
        c.mean     = 0.0;
        c.minVal   = 0.0;
        c.maxVal   = 0.0;

        long long n = 0;
        double mn = 0.0, mx = 0.0;
        bool   any = false;
        for (const auto& row : rows)
        {
            if (ci >= row.size() || row[ci].empty())
            {
                ++c.missing;
                continue;
            }
            double v = 0.0;
            if (TryParseNumber(row[ci], v))
            {
                c.sum += v;
                if (!any) { mn = mx = v; any = true; }
                else { mn = std::min(mn, v); mx = std::max(mx, v); }
                ++n;
            }
        }
        if (n > 0)
        {
            c.mean   = c.sum / static_cast<double>(n);
            c.minVal = mn;
            c.maxVal = mx;
        }
    }
}

void AnalysisTool::InferSchemaTypes(DataSource& ds)
{
    auto& cols = ds.MutableColumns();
    const auto& rows = ds.Rows();

    for (size_t ci = 0; ci < cols.size(); ++ci)
    {
        auto& c = cols[ci];
        size_t numCount = 0, dateCount = 0, textCount = 0;
        size_t sampled = 0;
        for (const auto& row : rows)
        {
            if (ci >= row.size() || row[ci].empty()) continue;
            double v = 0.0;
            if (TryParseNumber(row[ci], v)) ++numCount;
            else if (LooksLikeDate(row[ci])) ++dateCount;
            else ++textCount;
            if (++sampled >= 50) break;
        }
        if (numCount >= sampled * 0.7 && sampled > 0) c.type = ColumnType::Number;
        else if (dateCount >= sampled * 0.5 && sampled > 0) c.type = ColumnType::Date;
        else if (textCount > 0) c.type = ColumnType::Text;
        else c.type = ColumnType::Unknown;
    }

    // 도메인 요약 휴리스틱
    int numCols = 0, dateCols = 0;
    for (const auto& c : cols)
    {
        if (c.type == ColumnType::Number) ++numCols;
        else if (c.type == ColumnType::Date) ++dateCols;
    }
    std::wostringstream s;
    s << L"행 " << rows.size() << L", 컬럼 " << cols.size()
      << L" (수치 " << numCols << L", 날짜 " << dateCols << L")";
    ds.SetDomainSummary(s.str());
}

std::optional<Visualization>
AnalysisTool::GroupBySum(const DataSource& ds,
                         const std::wstring& groupColumn,
                         const std::wstring& valueColumn)
{
    int gi = FindColumnIndex(ds, groupColumn);
    int vi = FindColumnIndex(ds, valueColumn);
    if (gi < 0 || vi < 0) return std::nullopt;

    std::unordered_map<std::wstring, double> agg;
    for (const auto& row : ds.Rows())
    {
        if (gi >= static_cast<int>(row.size()) || vi >= static_cast<int>(row.size())) continue;
        double v = 0.0;
        if (!TryParseNumber(row[vi], v)) continue;
        agg[row[gi]] += v;
    }
    if (agg.empty()) return std::nullopt;

    Visualization viz;
    viz.type = VizType::Bar;
    viz.title = L"합계: " + valueColumn + L" by " + groupColumn;
    viz.width = 560; viz.height = 360;
    Series s;
    s.label = valueColumn;
    for (auto& [k, v] : agg) { viz.categories.push_back(k); s.values.push_back(v); }
    viz.series.push_back(std::move(s));
    return viz;
}

std::optional<Visualization>
AnalysisTool::TrendOverTime(const DataSource& ds,
                            const std::wstring& dateColumn,
                            const std::wstring& valueColumn)
{
    int di = FindColumnIndex(ds, dateColumn);
    int vi = FindColumnIndex(ds, valueColumn);
    if (di < 0 || vi < 0) return std::nullopt;

    Visualization viz;
    viz.type = VizType::Line;
    viz.title = L"추이: " + valueColumn + L" by " + dateColumn;
    viz.width = 640; viz.height = 360;
    Series s;
    s.label = valueColumn;

    for (const auto& row : ds.Rows())
    {
        if (di >= static_cast<int>(row.size()) || vi >= static_cast<int>(row.size())) continue;
        double v = 0.0;
        if (!TryParseNumber(row[vi], v)) continue;
        viz.categories.push_back(row[di]);
        s.values.push_back(v);
    }
    if (s.values.empty()) return std::nullopt;
    viz.series.push_back(std::move(s));
    return viz;
}

std::optional<Visualization>
AnalysisTool::TopNDistribution(const DataSource& ds,
                               const std::wstring& column,
                               int topN)
{
    int ci = FindColumnIndex(ds, column);
    if (ci < 0) return std::nullopt;

    std::unordered_map<std::wstring, long long> freq;
    for (const auto& row : ds.Rows())
    {
        if (ci >= static_cast<int>(row.size())) continue;
        if (row[ci].empty()) continue;
        ++freq[row[ci]];
    }
    if (freq.empty()) return std::nullopt;

    std::vector<std::pair<std::wstring, long long>> v(freq.begin(), freq.end());
    std::sort(v.begin(), v.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    if (static_cast<int>(v.size()) > topN) v.resize(topN);

    Visualization viz;
    viz.type = VizType::Bar;
    viz.title = column + L" 상위 " + std::to_wstring(topN);
    viz.width = 560; viz.height = 360;
    Series s;
    s.label = L"빈도";
    for (auto& [k, n] : v) { viz.categories.push_back(k); s.values.push_back(static_cast<double>(n)); }
    viz.series.push_back(std::move(s));
    return viz;
}

Visualization AnalysisTool::TableSample(const DataSource& ds, int rowLimit)
{
    Visualization viz;
    viz.type = VizType::Table;
    viz.title = L"데이터 미리보기";
    viz.width = 720; viz.height = 320;

    std::vector<std::wstring> headers;
    for (const auto& c : ds.Columns()) headers.push_back(c.name);
    viz.tableCells.push_back(headers);

    int n = 0;
    for (const auto& row : ds.Rows())
    {
        viz.tableCells.push_back(row);
        if (++n >= rowLimit) break;
    }
    return viz;
}

Visualization AnalysisTool::SummaryPanel(const DataSource& ds)
{
    Visualization viz;
    viz.type = VizType::Summary;
    viz.title = L"데이터 요약";
    viz.width = 360; viz.height = 320;
    viz.caption = ds.DomainSummary();

    // 표 셀에 요약을 채움 [컬럼명, 타입, 결측, 평균]
    viz.tableCells.push_back({ L"컬럼", L"타입", L"결측", L"평균" });
    for (const auto& c : ds.Columns())
    {
        std::wstring t = (c.type == ColumnType::Number) ? L"수치"
                      : (c.type == ColumnType::Date)   ? L"날짜"
                      : (c.type == ColumnType::Text)   ? L"텍스트" : L"?";
        wchar_t buf[64];
        swprintf_s(buf, L"%.2f", c.mean);
        viz.tableCells.push_back({ c.name, t, std::to_wstring(c.missing), buf });
    }
    return viz;
}

// ────────────────────────────────────────────────────────────────────
// Tier 1 신규 도구 구현
// ────────────────────────────────────────────────────────────────────

namespace {

// 함수 식별자 표준화 (소문자)
std::wstring NormFunc(std::wstring s)
{
    for (auto& c : s) c = static_cast<wchar_t>(std::towlower(c));
    return s;
}

// 그룹화된 값들에 함수 적용 (sum/mean/median/min/max/count)
double ApplyAgg(std::vector<double> vals, const std::wstring& func)
{
    if (vals.empty()) return 0.0;
    std::wstring f = NormFunc(func);
    if (f == L"count")  return static_cast<double>(vals.size());
    if (f == L"sum")
    {
        double s = 0;
        for (double v : vals) s += v;
        return s;
    }
    if (f == L"mean" || f == L"avg" || f == L"average")
    {
        double s = 0;
        for (double v : vals) s += v;
        return s / vals.size();
    }
    if (f == L"median")
    {
        std::sort(vals.begin(), vals.end());
        size_t n = vals.size();
        return (n % 2 == 1) ? vals[n / 2]
                            : 0.5 * (vals[n / 2 - 1] + vals[n / 2]);
    }
    if (f == L"min") return *std::min_element(vals.begin(), vals.end());
    if (f == L"max") return *std::max_element(vals.begin(), vals.end());
    // 알 수 없는 함수는 sum 폴백
    double s = 0;
    for (double v : vals) s += v;
    return s;
}

bool CompareCell(const std::wstring& cell, const std::wstring& op,
                 const std::wstring& value)
{
    if (op == L"==") return cell == value;
    if (op == L"!=") return cell != value;
    if (op == L"contains") return cell.find(value) != std::wstring::npos;

    double cv = 0.0, vv = 0.0;
    if (!TryParseNumber(cell, cv)) return false;
    if (!TryParseNumber(value, vv)) return false;
    if (op == L">")  return cv > vv;
    if (op == L"<")  return cv < vv;
    if (op == L">=") return cv >= vv;
    if (op == L"<=") return cv <= vv;
    return false;
}

} // namespace

std::optional<Visualization>
AnalysisTool::Filter(const DataSource& ds,
                     const std::wstring& column,
                     const std::wstring& op,
                     const std::wstring& value,
                     int rowLimit)
{
    int ci = FindColumnIndex(ds, column);
    if (ci < 0) return std::nullopt;

    Visualization viz;
    viz.type   = VizType::Table;
    viz.title  = L"필터: " + column + L" " + op + L" " + value;
    viz.width  = 720; viz.height = 360;

    // 헤더
    std::vector<std::wstring> headers;
    for (const auto& c : ds.Columns()) headers.push_back(c.name);
    viz.tableCells.push_back(headers);

    int kept = 0;
    for (const auto& row : ds.Rows())
    {
        if (ci >= (int)row.size()) continue;
        if (!CompareCell(row[ci], op, value)) continue;
        viz.tableCells.push_back(row);
        if (++kept >= rowLimit) break;
    }
    return viz;
}

std::optional<Visualization>
AnalysisTool::Aggregate(const DataSource& ds,
                        const std::wstring& groupColumn,
                        const std::wstring& valueColumn,
                        const std::wstring& func)
{
    int gi = FindColumnIndex(ds, groupColumn);
    int vi = (func == L"count" || NormFunc(func) == L"count")
             ? gi   // count는 값 컬럼 없어도 됨
             : FindColumnIndex(ds, valueColumn);
    if (gi < 0 || (vi < 0 && NormFunc(func) != L"count")) return std::nullopt;

    std::unordered_map<std::wstring, std::vector<double>> agg;
    for (const auto& row : ds.Rows())
    {
        if (gi >= (int)row.size()) continue;
        if (NormFunc(func) == L"count")
        {
            agg[row[gi]].push_back(1.0);
        }
        else
        {
            if (vi >= (int)row.size()) continue;
            double v = 0.0;
            if (!TryParseNumber(row[vi], v)) continue;
            agg[row[gi]].push_back(v);
        }
    }
    if (agg.empty()) return std::nullopt;

    Visualization viz;
    viz.type   = VizType::Bar;
    viz.title  = NormFunc(func) + L"(" + valueColumn + L") by " + groupColumn;
    viz.width  = 560; viz.height = 360;
    Series s;
    s.label = NormFunc(func) + L"(" + valueColumn + L")";
    for (auto& [k, vals] : agg)
    {
        viz.categories.push_back(k);
        s.values.push_back(ApplyAgg(vals, func));
    }
    viz.series.push_back(std::move(s));
    return viz;
}

std::optional<Visualization>
AnalysisTool::GroupByCount(const DataSource& ds, const std::wstring& groupColumn)
{
    return Aggregate(ds, groupColumn, L"", L"count");
}

std::optional<Visualization>
AnalysisTool::GroupByMean(const DataSource& ds,
                          const std::wstring& groupColumn,
                          const std::wstring& valueColumn)
{
    return Aggregate(ds, groupColumn, valueColumn, L"mean");
}

std::optional<Visualization>
AnalysisTool::SummaryStats(const DataSource& ds, const std::wstring& valueColumn)
{
    int vi = FindColumnIndex(ds, valueColumn);
    if (vi < 0) return std::nullopt;

    std::vector<double> vals;
    for (const auto& row : ds.Rows())
    {
        if (vi >= (int)row.size()) continue;
        double v = 0.0;
        if (TryParseNumber(row[vi], v)) vals.push_back(v);
    }
    if (vals.empty()) return std::nullopt;

    double sum  = 0;
    for (double v : vals) sum += v;
    double mean = sum / vals.size();

    double sq = 0;
    for (double v : vals) sq += (v - mean) * (v - mean);
    double stdv = std::sqrt(sq / vals.size());

    std::vector<double> sorted = vals;
    std::sort(sorted.begin(), sorted.end());
    double median = (sorted.size() % 2 == 1)
                  ? sorted[sorted.size() / 2]
                  : 0.5 * (sorted[sorted.size() / 2 - 1] + sorted[sorted.size() / 2]);
    double mn = sorted.front(), mx = sorted.back();

    Visualization viz;
    viz.type  = VizType::Table;
    viz.title = valueColumn + L" 요약 통계";
    viz.width = 360; viz.height = 220;
    viz.tableCells.push_back({ L"통계량", L"값" });
    auto add = [&](const wchar_t* k, double v) {
        wchar_t buf[64]; swprintf_s(buf, L"%.4f", v);
        viz.tableCells.push_back({ k, buf });
    };
    add(L"평균",   mean);
    add(L"중앙값", median);
    add(L"표준편차", stdv);
    add(L"최소값", mn);
    add(L"최대값", mx);
    add(L"건수",   static_cast<double>(vals.size()));
    return viz;
}

std::optional<Visualization>
AnalysisTool::Histogram(const DataSource& ds,
                        const std::wstring& valueColumn,
                        int binCount)
{
    int vi = FindColumnIndex(ds, valueColumn);
    if (vi < 0) return std::nullopt;
    if (binCount < 2) binCount = 10;

    std::vector<double> vals;
    for (const auto& row : ds.Rows())
    {
        if (vi >= (int)row.size()) continue;
        double v = 0.0;
        if (TryParseNumber(row[vi], v)) vals.push_back(v);
    }
    if (vals.empty()) return std::nullopt;

    double mn = *std::min_element(vals.begin(), vals.end());
    double mx = *std::max_element(vals.begin(), vals.end());
    if (mx - mn < 1e-9) mx = mn + 1.0;
    double binW = (mx - mn) / binCount;

    std::vector<long long> bins(binCount, 0);
    for (double v : vals)
    {
        int idx = static_cast<int>((v - mn) / binW);
        if (idx >= binCount) idx = binCount - 1;
        if (idx < 0) idx = 0;
        ++bins[idx];
    }

    Visualization viz;
    viz.type  = VizType::Bar;
    viz.title = valueColumn + L" 히스토그램 (" + std::to_wstring(binCount) + L"구간)";
    viz.width = 640; viz.height = 360;
    Series s; s.label = L"빈도";
    for (int i = 0; i < binCount; ++i)
    {
        double lo = mn + binW * i;
        wchar_t buf[32]; swprintf_s(buf, L"%.1f", lo);
        viz.categories.push_back(buf);
        s.values.push_back(static_cast<double>(bins[i]));
    }
    viz.series.push_back(std::move(s));
    return viz;
}

std::optional<Visualization>
AnalysisTool::ScatterPlot(const DataSource& ds,
                          const std::wstring& xColumn,
                          const std::wstring& yColumn)
{
    int xi = FindColumnIndex(ds, xColumn);
    int yi = FindColumnIndex(ds, yColumn);
    if (xi < 0 || yi < 0) return std::nullopt;

    Series xs, ys;
    xs.label = xColumn;
    ys.label = yColumn;
    for (const auto& row : ds.Rows())
    {
        if (xi >= (int)row.size() || yi >= (int)row.size()) continue;
        double x = 0.0, y = 0.0;
        if (!TryParseNumber(row[xi], x)) continue;
        if (!TryParseNumber(row[yi], y)) continue;
        xs.values.push_back(x);
        ys.values.push_back(y);
    }
    if (xs.values.empty()) return std::nullopt;

    Visualization viz;
    viz.type  = VizType::Scatter;
    viz.title = L"산점도: " + xColumn + L" vs " + yColumn;
    viz.width = 560; viz.height = 360;
    viz.series.push_back(std::move(xs));  // [0] = X
    viz.series.push_back(std::move(ys));  // [1] = Y
    return viz;
}

std::optional<Visualization>
AnalysisTool::KpiCard(const DataSource& ds,
                      const std::wstring& valueColumn,
                      const std::wstring& func)
{
    int vi = FindColumnIndex(ds, valueColumn);
    bool isCount = NormFunc(func) == L"count";
    if (!isCount && vi < 0) return std::nullopt;

    std::vector<double> vals;
    for (const auto& row : ds.Rows())
    {
        if (isCount)
        {
            vals.push_back(1.0);
            continue;
        }
        if (vi >= (int)row.size()) continue;
        double v = 0.0;
        if (TryParseNumber(row[vi], v)) vals.push_back(v);
    }
    if (vals.empty()) return std::nullopt;

    double metric = ApplyAgg(vals, func);

    Visualization viz;
    viz.type  = VizType::Summary;
    viz.title = NormFunc(func) + L"(" + (isCount ? L"*" : valueColumn) + L")";
    viz.width = 280; viz.height = 140;
    wchar_t buf[64];
    if (std::abs(metric) >= 1e8)      swprintf_s(buf, L"%.2f억", metric / 1e8);
    else if (std::abs(metric) >= 1e4) swprintf_s(buf, L"%.0f", metric);
    else                              swprintf_s(buf, L"%.2f", metric);
    viz.tableCells.push_back({ buf });
    return viz;
}

// ────────────────────────────────────────────────────────────────────
// 공통 CSV 분리 헬퍼 (공백 trim)
// ────────────────────────────────────────────────────────────────────
namespace {
std::vector<std::wstring> SplitCsv(const std::wstring& s)
{
    std::vector<std::wstring> out;
    std::wstring buf;
    auto flush = [&](){
        while (!buf.empty() && (buf.front() == L' ' || buf.front() == L'\t')) buf.erase(buf.begin());
        while (!buf.empty() && (buf.back()  == L' ' || buf.back()  == L'\t')) buf.pop_back();
        if (!buf.empty()) out.push_back(buf);
        buf.clear();
    };
    for (wchar_t c : s)
    {
        if (c == L',') flush();
        else buf.push_back(c);
    }
    flush();
    return out;
}
} // namespace

// ────────────────────────────────────────────────────────────────────
// Wide 형식: 단일 행의 여러 수치 컬럼 비교
// ────────────────────────────────────────────────────────────────────
std::optional<Visualization>
AnalysisTool::CompareColumns(const DataSource& ds,
                             const std::wstring& filterColumn,
                             const std::wstring& filterValue,
                             const std::wstring& valueColumnsCsv)
{
    if (valueColumnsCsv.empty()) return std::nullopt;

    auto cols = SplitCsv(valueColumnsCsv);
    if (cols.empty()) return std::nullopt;

    // 컬럼 인덱스 매핑
    std::vector<int> colIdx;
    for (const auto& name : cols)
    {
        int i = FindColumnIndex(ds, name);
        if (i < 0) return std::nullopt;  // 누락된 컬럼이 하나라도 있으면 실패
        colIdx.push_back(i);
    }

    // 값 산출: filterValue 가 있으면 그 행만, 없으면 전체 합계
    std::vector<double> values(cols.size(), 0.0);
    if (!filterColumn.empty() && !filterValue.empty())
    {
        int fi = FindColumnIndex(ds, filterColumn);
        if (fi < 0) return std::nullopt;
        bool found = false;
        for (const auto& row : ds.Rows())
        {
            if (fi >= (int)row.size()) continue;
            if (row[fi] != filterValue) continue;
            found = true;
            for (size_t k = 0; k < colIdx.size(); ++k)
            {
                int ci = colIdx[k];
                if (ci < (int)row.size())
                {
                    double v = 0.0;
                    if (TryParseNumber(row[ci], v)) values[k] = v;
                }
            }
            break;  // 첫 일치 행만 사용 (필요 시 정책 변경)
        }
        if (!found) return std::nullopt;
    }
    else
    {
        // filter 없으면 전체 행 합계
        for (const auto& row : ds.Rows())
        {
            for (size_t k = 0; k < colIdx.size(); ++k)
            {
                int ci = colIdx[k];
                if (ci < (int)row.size())
                {
                    double v = 0.0;
                    if (TryParseNumber(row[ci], v)) values[k] += v;
                }
            }
        }
    }

    Visualization viz;
    viz.type   = VizType::Bar;
    viz.title  = filterValue.empty()
                 ? L"컬럼 비교"
                 : (filterColumn + L"=" + filterValue);
    viz.width  = 640; viz.height = 360;
    Series s; s.label = L"값";
    for (size_t k = 0; k < cols.size(); ++k)
    {
        viz.categories.push_back(cols[k]);
        s.values.push_back(values[k]);
    }
    viz.series.push_back(std::move(s));
    return viz;
}

// ────────────────────────────────────────────────────────────────────
// 특정 N개 행 비교 — 단일 수치 컬럼
// ────────────────────────────────────────────────────────────────────
std::optional<Visualization>
AnalysisTool::CompareRows(const DataSource& ds,
                          const std::wstring& keyColumn,
                          const std::wstring& keyValuesCsv,
                          const std::wstring& valueColumn)
{
    if (keyColumn.empty() || keyValuesCsv.empty() || valueColumn.empty())
        return std::nullopt;

    int ki = FindColumnIndex(ds, keyColumn);
    int vi = FindColumnIndex(ds, valueColumn);
    if (ki < 0 || vi < 0) return std::nullopt;

    auto keys = SplitCsv(keyValuesCsv);
    if (keys.empty()) return std::nullopt;

    // 각 key 마다 첫 일치 행의 valueColumn 값 산출 (없으면 0)
    std::vector<double> values(keys.size(), 0.0);
    std::vector<bool>   found (keys.size(), false);
    for (const auto& row : ds.Rows())
    {
        if (ki >= (int)row.size() || vi >= (int)row.size()) continue;
        for (size_t k = 0; k < keys.size(); ++k)
        {
            if (found[k]) continue;
            if (row[ki] == keys[k])
            {
                double v = 0.0;
                if (TryParseNumber(row[vi], v)) values[k] = v;
                found[k] = true;
            }
        }
    }
    bool anyFound = false;
    for (bool b : found) if (b) { anyFound = true; break; }
    if (!anyFound) return std::nullopt;

    Visualization viz;
    viz.type   = VizType::Bar;
    viz.title  = valueColumn + L" 비교";
    viz.width  = 640; viz.height = 360;
    Series s; s.label = valueColumn;
    for (size_t k = 0; k < keys.size(); ++k)
    {
        viz.categories.push_back(keys[k]);
        s.values.push_back(values[k]);
    }
    viz.series.push_back(std::move(s));
    return viz;
}

// ────────────────────────────────────────────────────────────────────
// 일부 행 × 일부 컬럼 — multi-series
// ────────────────────────────────────────────────────────────────────
std::optional<Visualization>
AnalysisTool::SelectRowsColumns(const DataSource& ds,
                                const std::wstring& filterColumn,
                                const std::wstring& filterValuesCsv,
                                const std::wstring& valueColumnsCsv)
{
    if (filterColumn.empty() || filterValuesCsv.empty() || valueColumnsCsv.empty())
        return std::nullopt;

    int fi = FindColumnIndex(ds, filterColumn);
    if (fi < 0) return std::nullopt;

    auto rows = SplitCsv(filterValuesCsv);
    auto cols = SplitCsv(valueColumnsCsv);
    if (rows.empty() || cols.empty()) return std::nullopt;

    // 컬럼 인덱스 매핑 — 누락이 하나라도 있으면 실패
    std::vector<int> colIdx;
    for (const auto& name : cols)
    {
        int i = FindColumnIndex(ds, name);
        if (i < 0) return std::nullopt;
        colIdx.push_back(i);
    }

    // matrix[rowIdx][colIdx] = value
    std::vector<std::vector<double>> matrix(rows.size(),
                                            std::vector<double>(cols.size(), 0.0));
    std::vector<bool> rowFound(rows.size(), false);
    for (const auto& row : ds.Rows())
    {
        if (fi >= (int)row.size()) continue;
        for (size_t r = 0; r < rows.size(); ++r)
        {
            if (rowFound[r]) continue;
            if (row[fi] != rows[r]) continue;
            for (size_t c = 0; c < colIdx.size(); ++c)
            {
                int ci = colIdx[c];
                if (ci < (int)row.size())
                {
                    double v = 0.0;
                    if (TryParseNumber(row[ci], v)) matrix[r][c] = v;
                }
            }
            rowFound[r] = true;
            break;
        }
    }

    Visualization viz;
    viz.type   = VizType::Bar;
    viz.title  = filterColumn + L" × 컬럼 비교";
    viz.width  = 720; viz.height = 380;

    // categories = filter values (예: 2010/2015/2020)
    for (const auto& r : rows) viz.categories.push_back(r);

    // series = 각 컬럼 (예: 중앙정부, 지방자치단체) → multi-series 막대
    for (size_t c = 0; c < cols.size(); ++c)
    {
        Series s;
        s.label = cols[c];
        for (size_t r = 0; r < rows.size(); ++r)
            s.values.push_back(matrix[r][c]);
        viz.series.push_back(std::move(s));
    }
    return viz;
}

// ────────────────────────────────────────────────────────────────────
// LLM 호출용 도구 카탈로그 — 위 모든 도구 + 기존 도구
// ────────────────────────────────────────────────────────────────────
const std::vector<AnalysisTool::ToolDescriptor>& AnalysisTool::Catalog()
{
    using K = ToolKind;
    static const std::vector<ToolDescriptor> kCatalog = {
        // 기존
        { L"group_by_sum",
          L"지정한 그룹 컬럼 기준으로 값 컬럼을 합계하여 막대 차트로 시각화",
          L"{\"group\":\"string\",\"value\":\"string\"}", K::Visualization },
        { L"trend_over_time",
          L"날짜 컬럼 기준으로 값 컬럼의 라인 추이를 시각화",
          L"{\"date\":\"string\",\"value\":\"string\"}", K::Visualization },
        { L"top_n_distribution",
          L"텍스트 컬럼 상위 N개 빈도를 막대 차트로 시각화",
          L"{\"column\":\"string\",\"n\":\"integer\"}", K::Visualization },
        { L"table_sample",
          L"원본 데이터 N행을 표로 출력",
          L"{\"rows\":\"integer\"}", K::Visualization },
        { L"summary_panel",
          L"데이터셋의 스키마/도메인/통계 요약 카드",
          L"{}", K::Visualization },

        // Tier 1 신규
        { L"filter",
          L"조건에 맞는 행만 추출 (op: ==,!=,>,<,>=,<=,contains)",
          L"{\"column\":\"string\",\"op\":\"string\",\"value\":\"string\",\"rowLimit\":\"integer\"}", K::Visualization },
        { L"aggregate",
          L"그룹 컬럼 기준으로 값 컬럼을 임의 함수로 집계 (func: sum/mean/median/min/max/count)",
          L"{\"group\":\"string\",\"value\":\"string\",\"func\":\"string\"}", K::Visualization },
        { L"group_by_count",
          L"그룹 컬럼 기준 행 수를 막대 차트로 시각화",
          L"{\"group\":\"string\"}", K::Visualization },
        { L"group_by_mean",
          L"그룹 컬럼 기준 값 컬럼의 평균을 막대 차트로 시각화",
          L"{\"group\":\"string\",\"value\":\"string\"}", K::Visualization },
        { L"summary_stats",
          L"단일 수치 컬럼의 평균/중앙/표준편차/min/max 요약 표",
          L"{\"value\":\"string\"}", K::Visualization },
        { L"histogram",
          L"수치 컬럼의 구간 분포 히스토그램 (binCount 기본 10)",
          L"{\"value\":\"string\",\"bins\":\"integer\"}", K::Visualization },
        { L"scatter_plot",
          L"두 수치 컬럼의 산점도",
          L"{\"x\":\"string\",\"y\":\"string\"}", K::Visualization },
        { L"kpi_card",
          L"단일 메트릭을 큰 글씨 카드로 표시 (func: sum/mean/median/min/max/count)",
          L"{\"value\":\"string\",\"func\":\"string\"}", K::Visualization },

        // Wide 형식 데이터 (한 행 = 한 시점, 여러 수치 컬럼 = 카테고리들)
        { L"compare_columns",
          L"한 행의 여러 수치 컬럼을 막대 차트로 비교 — wide 형식 데이터에 적합 "
          L"(예: 한 해의 부처별 예산을 컬럼별로 비교). "
          L"filterValue 가 비면 모든 행의 컬럼별 합계를 비교.",
          L"{\"filterColumn\":\"string\",\"filterValue\":\"string\","
          L"\"valueColumnsCsv\":\"string (쉼표 구분 컬럼명들)\"}",
          K::Visualization },

        // 특정 N개 행 + 단일 수치 비교 (예: 특정 연도들의 한 컬럼 비교)
        { L"compare_rows",
          L"key 컬럼에서 지정한 N개 값에 해당하는 행들의 단일 수치 컬럼을 막대로 비교 "
          L"(예: 2010/2015/2020 의 중앙정부 채무 비교).",
          L"{\"keyColumn\":\"string\",\"keyValuesCsv\":\"string (쉼표 구분)\","
          L"\"valueColumn\":\"string\"}",
          K::Visualization },

        // 일부 행 × 일부 컬럼 (multi-series)
        { L"select_rows_columns",
          L"filter 컬럼에서 지정한 N개 행 + valueColumns 의 여러 수치 컬럼을 "
          L"multi-series 막대(또는 라인)로 시각화 — "
          L"예: 2010/2015/2020 × [중앙정부, 지방자치단체] 그룹 막대.",
          L"{\"filterColumn\":\"string\",\"filterValuesCsv\":\"string (쉼표 구분)\","
          L"\"valueColumnsCsv\":\"string (쉼표 구분)\"}",
          K::Visualization }
    };
    return kCatalog;
}

AnalysisTool::ToolKind AnalysisTool::KindOf(const std::wstring& toolName)
{
    if (toolName.empty()) return ToolKind::Chat;
    for (const auto& t : Catalog())
        if (t.name == toolName) return t.kind;
    // 알 수 없는 도구 — 호출자가 별도 에러 처리
    return ToolKind::Visualization;
}

} // namespace deepmetria
