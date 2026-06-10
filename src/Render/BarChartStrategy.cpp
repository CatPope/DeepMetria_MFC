// Render/BarChartStrategy.cpp
#include "stdafx.h"
#include "BarChartStrategy.h"
#include "ChartRenderHelpers.h"
#include "Visualization.h"
#include <algorithm>

using namespace Gdiplus;

namespace deepmetria {

// tableCells에서 카테고리 + 수치 시리즈 추출 (마지막 컬럼을 값으로)
static void ExtractBarFromTable(
    const std::vector<std::vector<std::wstring>>& cells,
    std::vector<std::wstring>& outCats, deepmetria::Series& outS)
{
    if (cells.size() < 2 || cells[0].empty()) return;
    int lastCol = static_cast<int>(cells[0].size()) - 1;
    outS.label = cells[0][lastCol];
    for (size_t r = 1; r < cells.size(); ++r)
    {
        if (lastCol >= static_cast<int>(cells[r].size())) continue;
        const auto& cell = cells[r][lastCol];
        wchar_t* end = nullptr;
        double val = wcstod(cell.c_str(), &end);
        if (end == cell.c_str()) continue;
        outS.values.push_back(val);
        outCats.push_back(cells[r][0]);
    }
}

void BarChartStrategy::Draw(Graphics& g, const Visualization& v,
                            const DataSource* /*dsForSummary*/)
{
    deepmetria::Series fallbackS;
    std::vector<std::wstring> fallbackCats;
    const deepmetria::Series* psrc = nullptr;
    const std::vector<std::wstring>* pcats = nullptr;

    if (!v.series.empty() && !v.series[0].values.empty())
    {
        psrc = &v.series[0];
        pcats = &v.categories;
    }
    else if (!v.tableCells.empty())
    {
        ExtractBarFromTable(v.tableCells, fallbackCats, fallbackS);
        if (!fallbackS.values.empty()) { psrc = &fallbackS; pcats = &fallbackCats; }
    }
    if (!psrc) return;
    const auto& s = *psrc;
    REAL ox = static_cast<REAL>(v.x + 40);
    REAL oy = static_cast<REAL>(v.y + v.height - 30);
    REAL plotW = static_cast<REAL>(v.width - 60);
    REAL plotH = static_cast<REAL>(v.height - 70);

    double maxV = *std::max_element(s.values.begin(), s.values.end());
    if (maxV <= 0) maxV = 1.0;

    int n = static_cast<int>(s.values.size());
    REAL barW = plotW / static_cast<REAL>(n * 1.5f);

    // 그리드선
    if (v.showGrid)
    {
        Pen grid(FromArgb24(0xE5E7EB), 1.0f);
        for (int i = 1; i <= 4; ++i)
        {
            REAL gy = oy - plotH * (i / 5.0f);
            g.DrawLine(&grid, ox, gy, ox + plotW, gy);
        }
    }

    Pen axis(FromArgb24(0x9CA3AF), 1.0f);
    g.DrawLine(&axis, ox, oy, ox + plotW, oy);
    g.DrawLine(&axis, ox, oy, ox, oy - plotH);

    SolidBrush bar(FromArgb24(v.fillColor));
    for (int i = 0; i < n; ++i)
    {
        REAL h = static_cast<REAL>(plotH * (s.values[i] / maxV));
        REAL bx = ox + static_cast<REAL>(i) * (barW * 1.5f) + barW * 0.25f;
        g.FillRectangle(&bar, bx, oy - h, barW, h);
        if (pcats && i < static_cast<int>(pcats->size()))
            DrawLabel(g, (*pcats)[i], bx - 8, oy + 4, barW + 16, 18,
                     FromArgb24(0x4B5563), 9, false, StringAlignmentCenter);
        DrawValue(g, s.values[i], bx - 6, oy - h - 18, barW + 12, 18,
                  FromArgb24(0x111827));
    }
}

} // namespace deepmetria
