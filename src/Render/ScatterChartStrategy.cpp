// Render/ScatterChartStrategy.cpp
#include "stdafx.h"
#include "ScatterChartStrategy.h"
#include "ChartRenderHelpers.h"
#include "Visualization.h"
#include <algorithm>

using namespace Gdiplus;

namespace deepmetria {

void ScatterChartStrategy::Draw(Graphics& g, const Visualization& v,
                                const DataSource* /*dsForSummary*/)
{
    // series[0] = X, series[1] = Y (AnalysisTool::ScatterPlot 가 채움)
    if (v.series.size() < 2) return;
    const auto& xs = v.series[0].values;
    const auto& ys = v.series[1].values;
    if (xs.empty() || xs.size() != ys.size()) return;

    REAL ox = static_cast<REAL>(v.x + 50);
    REAL oy = static_cast<REAL>(v.y + v.height - 30);
    REAL plotW = static_cast<REAL>(v.width - 70);
    REAL plotH = static_cast<REAL>(v.height - 70);

    double xmn = *std::min_element(xs.begin(), xs.end());
    double xmx = *std::max_element(xs.begin(), xs.end());
    double ymn = *std::min_element(ys.begin(), ys.end());
    double ymx = *std::max_element(ys.begin(), ys.end());
    if (xmx - xmn < 1e-9) xmx = xmn + 1.0;
    if (ymx - ymn < 1e-9) ymx = ymn + 1.0;

    // 그리드선
    if (v.showGrid)
    {
        Pen grid(FromArgb24(0xE5E7EB), 1.0f);
        for (int i = 1; i <= 4; ++i)
        {
            REAL gy = oy - plotH * (i / 5.0f);
            g.DrawLine(&grid, ox, gy, ox + plotW, gy);
            REAL gx = ox + plotW * (i / 5.0f);
            g.DrawLine(&grid, gx, oy, gx, oy - plotH);
        }
    }

    // 축
    Pen axis(FromArgb24(0x9CA3AF), 1.0f);
    g.DrawLine(&axis, ox, oy, ox + plotW, oy);
    g.DrawLine(&axis, ox, oy, ox, oy - plotH);

    // 축 레이블 (min/max)
    DrawValue(g, xmn, ox - 20, oy + 4,  40, 14, FromArgb24(0x6B7280));
    DrawValue(g, xmx, ox + plotW - 20, oy + 4, 40, 14, FromArgb24(0x6B7280));
    DrawValue(g, ymn, ox - 50, oy - 8, 44, 14, FromArgb24(0x6B7280));
    DrawValue(g, ymx, ox - 50, oy - plotH - 8, 44, 14, FromArgb24(0x6B7280));

    // X / Y 축 컬럼명
    DrawLabel(g, v.series[0].label,
              ox + plotW / 2 - 60, oy + 18, 120, 16,
              FromArgb24(0x374151), 9, false, StringAlignmentCenter);
    DrawLabel(g, v.series[1].label,
              v.x + 4, v.y + 32, 44, 16,
              FromArgb24(0x374151), 9, true);

    // 점 그리기
    SolidBrush pt(FromArgb24(v.fillColor));
    for (size_t i = 0; i < xs.size(); ++i)
    {
        REAL px = ox + plotW * static_cast<REAL>((xs[i] - xmn) / (xmx - xmn));
        REAL py = oy - plotH * static_cast<REAL>((ys[i] - ymn) / (ymx - ymn));
        g.FillEllipse(&pt, px - 3.5f, py - 3.5f, 7.0f, 7.0f);
    }
}

} // namespace deepmetria
