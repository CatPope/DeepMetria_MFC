// Render/PieChartStrategy.cpp
#include "stdafx.h"
#include "PieChartStrategy.h"
#include "ChartRenderHelpers.h"
#include "Visualization.h"
#include <algorithm>

using namespace Gdiplus;

namespace deepmetria {

static deepmetria::Series ExtractSeriesForPie(
    const std::vector<std::vector<std::wstring>>& cells)
{
    deepmetria::Series s;
    if (cells.size() < 2 || cells[0].empty()) return s;
    int lastCol = static_cast<int>(cells[0].size()) - 1;
    s.label = cells[0][lastCol];
    for (size_t r = 1; r < cells.size(); ++r)
    {
        if (lastCol >= static_cast<int>(cells[r].size())) continue;
        const auto& cell = cells[r][lastCol];
        wchar_t* end = nullptr;
        double val = wcstod(cell.c_str(), &end);
        if (end == cell.c_str()) continue;
        s.values.push_back(val);
    }
    return s;
}

void PieChartStrategy::Draw(Graphics& g, const Visualization& v,
                            const DataSource* /*dsForSummary*/)
{
    deepmetria::Series fallback;
    const deepmetria::Series* psrc = nullptr;
    if (!v.series.empty() && !v.series[0].values.empty())
        psrc = &v.series[0];
    else if (!v.tableCells.empty())
    {
        fallback = ExtractSeriesForPie(v.tableCells);
        if (!fallback.values.empty()) psrc = &fallback;
    }
    if (!psrc) return;
    const auto& s = *psrc;
    double total = 0;
    for (double x : s.values) total += x;
    if (total <= 0) return;

    REAL cx = static_cast<REAL>(v.x + v.width / 2);
    REAL cy = static_cast<REAL>(v.y + v.height / 2 + 8);
    REAL r  = static_cast<REAL>(std::min(v.width, v.height) / 2 - 30);

    // v.fillColor 를 기준으로 한 hue-shift 팔레트 생성 (사용자 색 반영)
    DWORD base = v.fillColor;
    DWORD palette[7] = {
        base,
        v.accentColor,
        0xF59E0B, 0xEF4444, 0x6366F1, 0x14B8A6, 0xF97316
    };

    REAL start = -90.0f;
    Pen labelPen(FromArgb24(0x6B7280), 1.0f);

    for (size_t i = 0; i < s.values.size(); ++i)
    {
        REAL sweep = static_cast<REAL>(360.0 * s.values[i] / total);
        SolidBrush sl(FromArgb24(palette[i % 7]));
        g.FillPie(&sl, cx - r, cy - r, r * 2, r * 2, start, sweep);

        // 조각 중심 각도로 레이블 위치 계산
        double midDeg = static_cast<double>(start) + sweep * 0.5;
        double rad = midDeg * 3.14159265358979 / 180.0;
        REAL labelR = r + 18;
        REAL lx = cx + static_cast<REAL>(labelR * std::cos(rad));
        REAL ly = cy + static_cast<REAL>(labelR * std::sin(rad));

        // 카테고리 + 퍼센트
        double pct = 100.0 * s.values[i] / total;
        std::wstring labelText;
        if (i < v.categories.size())
            labelText = v.categories[i] + L"  ";
        wchar_t pctStr[16];
        swprintf_s(pctStr, L"%.1f%%", pct);
        labelText += pctStr;

        DrawLabel(g, labelText,
                  lx - 40, ly - 8, 80, 16,
                  FromArgb24(0x111827), 9, false, StringAlignmentCenter);

        start += sweep;
    }
}

} // namespace deepmetria
