// Render/SummaryChartStrategy.cpp
#include "stdafx.h"
#include "SummaryChartStrategy.h"
#include "ChartRenderHelpers.h"
#include "Visualization.h"
#include "DataSource.h"

using namespace Gdiplus;

namespace deepmetria {

void SummaryChartStrategy::Draw(Graphics& g, const Visualization& v,
                                const DataSource* dsForSummary)
{
    if (!dsForSummary) return;
    const DataSource& ds = *dsForSummary;

    REAL x = static_cast<REAL>(v.x + 12);
    REAL y = static_cast<REAL>(v.y + 34);
    DrawLabel(g, L"파일:  " + ds.SourcePath(), x, y, static_cast<REAL>(v.width - 20), 20,
             FromArgb24(0x374151), 11);
    y += 20;
    std::wstring summaryLine = ds.DomainSummary().empty()
        ? std::wstring()
        : (std::wstring(L"요약: ") + ds.DomainSummary());
    DrawLabel(g, summaryLine, x, y, static_cast<REAL>(v.width - 20), 20,
             FromArgb24(0x111827), 11, true);
    y += 22;

    DrawLabel(g, L"컬럼",  x,        y, 140, 18, FromArgb24(0x6B7280), 10, true);
    DrawLabel(g, L"타입",  x + 140,  y, 60,  18, FromArgb24(0x6B7280), 10, true);
    DrawLabel(g, L"결측",  x + 200,  y, 50,  18, FromArgb24(0x6B7280), 10, true);
    DrawLabel(g, L"평균",  x + 250,  y, 80,  18, FromArgb24(0x6B7280), 10, true);
    y += 18;

    for (const auto& c : ds.Columns())
    {
        if (y + 18 > v.y + v.height - 8) break;
        std::wstring t = (c.type == ColumnType::Number) ? L"수치"
                      : (c.type == ColumnType::Date)   ? L"날짜"
                      : (c.type == ColumnType::Text)   ? L"텍스트" : L"?";
        DrawLabel(g, c.name, x,       y, 140, 18, FromArgb24(0x111827), 10);
        DrawLabel(g, t,      x + 140, y, 60,  18, FromArgb24(0x374151), 10);
        DrawLabel(g, std::to_wstring(c.missing),
                            x + 200, y, 50,  18, FromArgb24(0x374151), 10);
        wchar_t mean[32];
        swprintf_s(mean, L"%.2f", c.mean);
        DrawLabel(g, mean,   x + 250, y, 80,  18, FromArgb24(0x374151), 10);
        y += 18;
    }
}

} // namespace deepmetria
