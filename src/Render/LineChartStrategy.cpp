// Render/LineChartStrategy.cpp
#include "stdafx.h"
#include "LineChartStrategy.h"
#include "ChartRenderHelpers.h"
#include "Visualization.h"
#include <algorithm>
#include <vector>

using namespace Gdiplus;

namespace deepmetria {

// tableCells의 마지막 컬럼을 수치 시리즈로 추출 (헤더 행 제외)
static deepmetria::Series ExtractSeriesFromTable(
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
        double v = wcstod(cell.c_str(), &end);
        if (end == cell.c_str()) continue;  // 파싱 실패
        s.values.push_back(v);
    }
    return s;
}

void LineChartStrategy::Draw(Graphics& g, const Visualization& v,
                             const DataSource* /*dsForSummary*/)
{
    // series가 없으면 tableCells에서 즉석 추출 (type 변환 대비)
    deepmetria::Series fallback;
    const deepmetria::Series* psrc = nullptr;
    if (!v.series.empty() && v.series[0].values.size() >= 2)
        psrc = &v.series[0];
    else if (!v.tableCells.empty())
    {
        fallback = ExtractSeriesFromTable(v.tableCells);
        if (fallback.values.size() >= 2) psrc = &fallback;
    }
    if (!psrc) return;
    const auto& s = *psrc;
    REAL ox = static_cast<REAL>(v.x + 40);
    REAL oy = static_cast<REAL>(v.y + v.height - 30);
    REAL plotW = static_cast<REAL>(v.width - 60);
    REAL plotH = static_cast<REAL>(v.height - 70);

    double maxV = *std::max_element(s.values.begin(), s.values.end());
    double minV = *std::min_element(s.values.begin(), s.values.end());
    if (maxV - minV < 1e-9) { maxV = minV + 1.0; }

    // 그리드선 (옵션)
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

    // Y축 눈금 (max/mid/min)
    DrawValue(g, maxV,        ox - 38, oy - plotH - 6, 36, 14, FromArgb24(0x6B7280));
    DrawValue(g, (maxV+minV)/2, ox - 38, oy - plotH/2 - 6, 36, 14, FromArgb24(0x6B7280));
    DrawValue(g, minV,        ox - 38, oy - 6,         36, 14, FromArgb24(0x6B7280));

    int n = static_cast<int>(s.values.size());
    Pen line(FromArgb24(v.fillColor), 2.5f);
    SolidBrush pt(FromArgb24(v.fillColor));

    std::vector<PointF> pts;
    pts.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        REAL px = ox + plotW * static_cast<REAL>(i) / static_cast<REAL>(n - 1);
        REAL py = oy - static_cast<REAL>(plotH * ((s.values[i] - minV) / (maxV - minV)));
        pts.push_back({ px, py });
    }
    g.DrawLines(&line, pts.data(), n);

    // 데이터 점 + 값 레이블 + x축 카테고리 레이블
    for (int i = 0; i < n; ++i)
    {
        g.FillEllipse(&pt, pts[i].X - 3.5f, pts[i].Y - 3.5f, 7.0f, 7.0f);
        // 값 (점 위)
        DrawValue(g, s.values[i],
                  pts[i].X - 22, pts[i].Y - 20, 44, 14,
                  FromArgb24(0x111827));
        // 카테고리 (x축 아래)
        if (i < static_cast<int>(v.categories.size()))
            DrawLabel(g, v.categories[i],
                      pts[i].X - 30, oy + 4, 60, 14,
                      FromArgb24(0x4B5563), 9, false, StringAlignmentCenter);
    }
}

} // namespace deepmetria
