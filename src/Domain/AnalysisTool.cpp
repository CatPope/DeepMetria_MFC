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

const std::vector<AnalysisTool::ToolDescriptor>& AnalysisTool::Catalog()
{
    static const std::vector<ToolDescriptor> kCatalog = {
        { L"group_by_sum",
          L"지정한 그룹 컬럼 기준으로 값 컬럼을 합계하여 막대 차트로 시각화",
          L"{\"group\":\"string\",\"value\":\"string\"}" },
        { L"trend_over_time",
          L"날짜 컬럼 기준으로 값 컬럼의 라인 추이를 시각화",
          L"{\"date\":\"string\",\"value\":\"string\"}" },
        { L"top_n_distribution",
          L"텍스트 컬럼 상위 N개 빈도를 막대 차트로 시각화",
          L"{\"column\":\"string\",\"n\":\"integer\"}" },
        { L"table_sample",
          L"원본 데이터 N행을 표로 출력",
          L"{\"rows\":\"integer\"}" }
    };
    return kCatalog;
}

} // namespace deepmetria
