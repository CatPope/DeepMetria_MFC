#include "stdafx.h"

// ScatterChart.cpp — 산점도 렌더러
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조

#include "ScatterChart.h"
#include "ChartRenderer.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <numeric>

using namespace Gdiplus;

// ============================================================
// JSON 파싱
// 형식: { "datasets": [ { "name":"...", "points": [[x,y], ...] }, ... ] }
// ============================================================

std::vector<std::vector<CScatterChart::PointXY>> CScatterChart::ParseDatasets(
    const CString& dataJson,
    std::vector<CString>& outNames)
{
    std::vector<std::vector<PointXY>> result;
    outNames.clear();

    std::wstring json = dataJson.GetString();

    size_t datasetsPos = json.find(L"\"datasets\"");
    if (datasetsPos == std::wstring::npos)
        return result;

    size_t arrStart = json.find(L'[', datasetsPos);
    if (arrStart == std::wstring::npos)
        return result;

    size_t pos = arrStart + 1;
    while (pos < json.size())
    {
        size_t objStart = json.find(L'{', pos);
        if (objStart == std::wstring::npos)
            break;

        int depth = 1;
        size_t objEnd = objStart + 1;
        while (objEnd < json.size() && depth > 0)
        {
            if (json[objEnd] == L'{') ++depth;
            else if (json[objEnd] == L'}') --depth;
            ++objEnd;
        }

        std::wstring obj = json.substr(objStart + 1, objEnd - objStart - 2);

        // name
        CString dsName;
        size_t namePos = obj.find(L"\"name\"");
        if (namePos != std::wstring::npos)
        {
            size_t q1 = obj.find(L'"', namePos + 6);
            size_t q2 = obj.find(L'"', q1 + 1);
            if (q1 != std::wstring::npos && q2 != std::wstring::npos)
                dsName = CString(obj.substr(q1 + 1, q2 - q1 - 1).c_str());
        }
        outNames.push_back(dsName);

        // points 배열 파싱
        std::vector<PointXY> points;
        size_t ptsPos    = obj.find(L"\"points\"");
        size_t pArrStart = obj.find(L'[', ptsPos);
        if (pArrStart != std::wstring::npos)
        {
            // 점 배열: [[x,y],[x,y],...]
            size_t cur = pArrStart + 1;
            while (cur < obj.size())
            {
                size_t pStart = obj.find(L'[', cur);
                if (pStart == std::wstring::npos)
                    break;
                size_t pEnd = obj.find(L']', pStart);
                if (pEnd == std::wstring::npos)
                    break;

                std::wstring pair = obj.substr(pStart + 1, pEnd - pStart - 1);
                size_t comma = pair.find(L',');
                if (comma != std::wstring::npos)
                {
                    PointXY pt = { 0.0, 0.0 };
                    try { pt.x = std::stod(pair.substr(0, comma)); } catch (...) {}
                    try { pt.y = std::stod(pair.substr(comma + 1)); } catch (...) {}
                    points.push_back(pt);
                }
                cur = pEnd + 1;
            }
        }
        result.push_back(points);
        pos = objEnd;
    }

    return result;
}

// ============================================================
// X/Y 범위 계산
// ============================================================

void CScatterChart::CalcRange(const std::vector<std::vector<PointXY>>& datasets,
                               double& xMin, double& xMax, double& yMin, double& yMax)
{
    xMin = xMax = yMin = yMax = 0.0;
    bool first = true;
    for (auto& ds : datasets)
    {
        for (auto& pt : ds)
        {
            if (first)
            {
                xMin = xMax = pt.x;
                yMin = yMax = pt.y;
                first = false;
            }
            else
            {
                xMin = min(xMin, pt.x); xMax = max(xMax, pt.x);
                yMin = min(yMin, pt.y); yMax = max(yMax, pt.y);
            }
        }
    }

    // 여백
    double xRange = (xMax - xMin == 0.0) ? 1.0 : (xMax - xMin);
    double yRange = (yMax - yMin == 0.0) ? 1.0 : (yMax - yMin);
    xMin -= xRange * 0.05; xMax += xRange * 0.05;
    yMin -= yRange * 0.05; yMax += yRange * 0.1;
}

// ============================================================
// 마커 (반지름 5px, 반투명)
// ============================================================

void CScatterChart::DrawMarker(Graphics& g, REAL cx, REAL cy, const Color& color)
{
    const REAL r = 5.0f;
    // 반투명 채우기 (알파 160)
    Color fillColor(160, color.GetR(), color.GetG(), color.GetB());
    SolidBrush fill(fillColor);
    Pen border(color, 1.0f);

    g.FillEllipse(&fill, cx - r, cy - r, r * 2, r * 2);
    g.DrawEllipse(&border, cx - r, cy - r, r * 2, r * 2);
}

// ============================================================
// 선형 회귀 계수 계산 (y = a*x + b)
// ============================================================

void CScatterChart::LinearRegression(const std::vector<PointXY>& points,
                                      double& outA, double& outB)
{
    int n = (int)points.size();
    if (n < 2) { outA = 0.0; outB = 0.0; return; }

    double sumX = 0, sumY = 0, sumXX = 0, sumXY = 0;
    for (auto& pt : points)
    {
        sumX  += pt.x;
        sumY  += pt.y;
        sumXX += pt.x * pt.x;
        sumXY += pt.x * pt.y;
    }

    double denom = n * sumXX - sumX * sumX;
    if (denom == 0.0) { outA = 0.0; outB = sumY / n; return; }

    outA = (n * sumXY - sumX * sumY) / denom;
    outB = (sumY - outA * sumX) / n;
}

// ============================================================
// 회귀선 그리기
// ============================================================

void CScatterChart::DrawTrendLine(Graphics& g, const std::vector<PointXY>& points,
                                   const CRect& plotArea,
                                   double xMin, double xMax,
                                   double yMin, double yMax,
                                   const Color& color)
{
    if (points.size() < 2)
        return;

    double a, b;
    LinearRegression(points, a, b);

    double xRange = xMax - xMin;
    double yRange = yMax - yMin;
    if (xRange == 0.0 || yRange == 0.0)
        return;

    REAL plotL = (REAL)plotArea.left;
    REAL plotT = (REAL)plotArea.top;
    REAL plotW = (REAL)plotArea.Width();
    REAL plotH = (REAL)plotArea.Height();
    REAL plotB = (REAL)plotArea.bottom;

    auto toScreen = [&](double x, double y) -> PointF {
        REAL sx = plotL + (REAL)((x - xMin) / xRange * plotW);
        REAL sy = plotB - (REAL)((y - yMin) / yRange * plotH);
        return PointF(sx, sy);
    };

    PointF p1 = toScreen(xMin, a * xMin + b);
    PointF p2 = toScreen(xMax, a * xMax + b);

    // 반투명 대시선
    Color lineColor(180, color.GetR(), color.GetG(), color.GetB());
    Pen trendPen(lineColor, 1.5f);
    trendPen.SetDashStyle(DashStyleDash);
    g.DrawLine(&trendPen, p1, p2);
}

// ============================================================
// 메인 Draw
// ============================================================

void CScatterChart::Draw(Graphics& g, const CRect& plotArea, const ChartConfig& config,
                          BOOL bTrendLine)
{
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    std::vector<CString> names;
    std::vector<std::vector<PointXY>> datasets = ParseDatasets(config.dataJson, names);

    if (datasets.empty())
        return;

    double xMin, xMax, yMin, yMax;
    CalcRange(datasets, xMin, xMax, yMin, yMax);

    double xRange = xMax - xMin;
    double yRange = yMax - yMin;
    if (xRange == 0.0) xRange = 1.0;
    if (yRange == 0.0) yRange = 1.0;

    std::vector<Color> palette = CChartRenderer::GetColorPalette();

    FontFamily ff(L"맑은 고딕");
    Gdiplus::Font labelFont(&ff, 9, FontStyleRegular, UnitPoint);

    REAL plotL = (REAL)plotArea.left;
    REAL plotT = (REAL)plotArea.top;
    REAL plotW = (REAL)plotArea.Width();
    REAL plotH = (REAL)plotArea.Height();
    REAL plotB = (REAL)plotArea.bottom;

    auto toScreen = [&](double x, double y) -> PointF {
        REAL sx = plotL + (REAL)((x - xMin) / xRange * plotW);
        REAL sy = plotB - (REAL)((y - yMin) / yRange * plotH);
        return PointF(sx, sy);
    };

    // --------------------------------------------------------
    // 격자선 + 축
    // --------------------------------------------------------
    const int tickCount = 5;
    Pen axisPen(Color(100, 100, 100), 1.5f);
    Pen gridPen(Color(220, 220, 220), 1.0f);
    SolidBrush axisTextBrush(Color(80, 80, 80));

    for (int i = 0; i <= tickCount; ++i)
    {
        // Y 격자
        double yVal = yMin + yRange * i / tickCount;
        REAL   yPos = plotB - (REAL)((yVal - yMin) / yRange * plotH);
        g.DrawLine(&gridPen, plotL, yPos, plotL + plotW, yPos);

        wchar_t buf[32];
        swprintf_s(buf, 32, L"%.1f", yVal);
        StringFormat sfY;
        sfY.SetAlignment(StringAlignmentFar);
        RectF yLabelRect(plotL - 60.0f, yPos - 8.0f, 55.0f, 16.0f);
        g.DrawString(buf, -1, &labelFont, yLabelRect, &sfY, &axisTextBrush);

        // X 격자
        double xVal = xMin + xRange * i / tickCount;
        REAL   xPos = plotL + (REAL)((xVal - xMin) / xRange * plotW);
        g.DrawLine(&gridPen, xPos, plotT, xPos, plotB);

        swprintf_s(buf, 32, L"%.1f", xVal);
        StringFormat sfX;
        sfX.SetAlignment(StringAlignmentCenter);
        RectF xLabelRect(xPos - 30.0f, plotB + 6.0f, 60.0f, 16.0f);
        g.DrawString(buf, -1, &labelFont, xLabelRect, &sfX, &axisTextBrush);
    }

    g.DrawLine(&axisPen, plotL, plotT, plotL, plotB);
    g.DrawLine(&axisPen, plotL, plotB, plotL + plotW, plotB);

    // --------------------------------------------------------
    // 데이터 포인트 + 회귀선
    // --------------------------------------------------------
    for (int ds = 0; ds < (int)datasets.size(); ++ds)
    {
        Color c = palette[ds % (int)palette.size()];

        // 회귀선 (포인트 아래 레이어로 먼저)
        if (bTrendLine)
            DrawTrendLine(g, datasets[ds], plotArea, xMin, xMax, yMin, yMax, c);

        // 마커
        for (auto& pt : datasets[ds])
        {
            PointF sp = toScreen(pt.x, pt.y);
            DrawMarker(g, sp.X, sp.Y, c);
        }
    }

    // --------------------------------------------------------
    // 범례
    // --------------------------------------------------------
    std::vector<Color> legendColors;
    for (int i = 0; i < (int)datasets.size(); ++i)
        legendColors.push_back(palette[i % (int)palette.size()]);

    CRect fullRect(plotArea.left - 70, plotArea.top - 50,
                   plotArea.right + 150, plotArea.bottom + 60);
    CChartRenderer::DrawLegend(g, fullRect, names, legendColors);
}
