// Render/TableChartStrategy.cpp
#include "stdafx.h"
#include "TableChartStrategy.h"
#include "ChartRenderHelpers.h"
#include "Visualization.h"

using namespace Gdiplus;

namespace deepmetria {

void TableChartStrategy::Draw(Graphics& g, const Visualization& v,
                              const DataSource* /*dsForSummary*/)
{
    // tableCells가 비어있으면 categories + series로 즉석 구성 (type 변환 대비)
    std::vector<std::vector<std::wstring>> fallback;
    const std::vector<std::vector<std::wstring>>* cells = &v.tableCells;
    if (cells->empty() && !v.series.empty() && !v.categories.empty())
    {
        const auto& s = v.series[0];
        fallback.push_back({ L"항목", s.label.empty() ? L"값" : s.label });
        size_t n = std::min(v.categories.size(), s.values.size());
        for (size_t i = 0; i < n; ++i)
        {
            wchar_t buf[32];
            swprintf_s(buf, L"%.2f", s.values[i]);
            fallback.push_back({ v.categories[i], buf });
        }
        cells = &fallback;
    }
    if (cells->empty()) return;

    const int padding = 8;
    const int headerH = 24;
    const int rowH    = 20;
    int cols = static_cast<int>((*cells)[0].size());
    if (cols == 0) return;
    int colW = (v.width - padding * 2) / cols;

    REAL x0 = static_cast<REAL>(v.x + padding);
    REAL y0 = static_cast<REAL>(v.y + 32);

    // 헤더 배경 — 사용자가 선택한 viz.fillColor 적용
    SolidBrush hbg(FromArgb24(v.fillColor));
    g.FillRectangle(&hbg, x0, y0, static_cast<REAL>(colW * cols), static_cast<REAL>(headerH));

    // 헤더 텍스트는 배경 색에 따라 흰색으로 (대비 확보)
    for (int c = 0; c < cols; ++c)
        DrawLabel(g, (*cells)[0][c],
                 x0 + c * colW + 6, y0,
                 static_cast<REAL>(colW - 12), static_cast<REAL>(headerH),
                 FromArgb24(0xFFFFFF), 11, true);

    REAL y = y0 + headerH;
    Pen sep(FromArgb24(0xE5E7EB), 1.0f);
    for (size_t r = 1; r < cells->size(); ++r)
    {
        if (y + rowH > v.y + v.height - 8) break;
        for (int c = 0; c < cols && c < static_cast<int>((*cells)[r].size()); ++c)
            DrawLabel(g, (*cells)[r][c],
                     x0 + c * colW + 6, y,
                     static_cast<REAL>(colW - 12), static_cast<REAL>(rowH),
                     FromArgb24(0x374151), 10);
        y += rowH;
        g.DrawLine(&sep, x0, y, x0 + static_cast<REAL>(colW * cols), y);
    }
}

} // namespace deepmetria
