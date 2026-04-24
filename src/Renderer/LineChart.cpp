#include "stdafx.h"

// LineChart.cpp — 선 차트 렌더러
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조

#include "LineChart.h"
#include "ChartRenderer.h"
#include <cmath>
#include <algorithm>
#include <sstream>

using namespace Gdiplus;

// ============================================================
// JSON 파싱
// 형식: { "labels": ["A","B",...], "datasets": [ { "name":"...", "values":[1,2,...] }, ... ] }
// ============================================================

std::vector<std::vector<double>> CLineChart::ParseDatasets(const CString& dataJson,
                                                            std::vector<CString>& outLabels,
                                                            std::vector<CString>& outDatasetNames)
{
    std::vector<std::vector<double>> result;
    outLabels.clear();
    outDatasetNames.clear();

    std::wstring json = dataJson.GetString();

    // labels 파싱
    size_t labelsPos = json.find(L"\"labels\"");
    if (labelsPos != std::wstring::npos)
    {
        size_t arrStart = json.find(L'[', labelsPos);
        size_t arrEnd   = json.find(L']', arrStart);
        if (arrStart != std::wstring::npos && arrEnd != std::wstring::npos)
        {
            std::wstring labelsStr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
            std::wistringstream ss(labelsStr);
            std::wstring token;
            while (std::getline(ss, token, L','))
            {
                size_t q1 = token.find(L'"');
                size_t q2 = token.rfind(L'"');
                if (q1 != std::wstring::npos && q2 != q1)
                    outLabels.push_back(CString(token.substr(q1 + 1, q2 - q1 - 1).c_str()));
                else
                    outLabels.push_back(CString(token.c_str()));
            }
        }
    }

    // datasets 파싱
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
        outDatasetNames.push_back(dsName);

        // values
        std::vector<double> values;
        size_t valPos    = obj.find(L"\"values\"");
        size_t vArrStart = obj.find(L'[', valPos);
        size_t vArrEnd   = obj.find(L']', vArrStart);
        if (vArrStart != std::wstring::npos && vArrEnd != std::wstring::npos)
        {
            std::wstring valStr = obj.substr(vArrStart + 1, vArrEnd - vArrStart - 1);
            std::wistringstream vss(valStr);
            std::wstring vtoken;
            while (std::getline(vss, vtoken, L','))
            {
                try { values.push_back(std::stod(vtoken)); }
                catch (...) { values.push_back(0.0); }
            }
        }
        result.push_back(values);
        pos = objEnd;
    }

    return result;
}

// ============================================================
// Y 범위 계산
// ============================================================

void CLineChart::CalcYRange(const std::vector<std::vector<double>>& datasets,
                             double& outMin, double& outMax)
{
    outMin = 0.0;
    outMax = 1.0;
    bool first = true;
    for (auto& ds : datasets)
    {
        for (double v : ds)
        {
            if (first) { outMin = outMax = v; first = false; }
            else { outMin = min(outMin, v); outMax = max(outMax, v); }
        }
    }
    // 약간의 여백
    double range = outMax - outMin;
    if (range == 0.0) range = 1.0;
    outMin = outMin - range * 0.05;
    outMax = outMax + range * 0.1;
}

// ============================================================
// 대시 패턴 (다중 데이터셋 구분)
// ============================================================

void CLineChart::ApplyDashStyle(Pen& pen, int datasetIdx)
{
    switch (datasetIdx % 4)
    {
    case 0: pen.SetDashStyle(DashStyleSolid);       break;
    case 1: pen.SetDashStyle(DashStyleDash);        break;
    case 2: pen.SetDashStyle(DashStyleDot);         break;
    case 3: pen.SetDashStyle(DashStyleDashDot);     break;
    }
}

// ============================================================
// 원형 마커 (반경 4px)
// ============================================================

void CLineChart::DrawMarker(Graphics& g, REAL cx, REAL cy, const Color& color)
{
    const REAL r = 4.0f;
    SolidBrush fill(color);
    Pen        border(Color::White, 1.5f);

    g.FillEllipse(&fill, cx - r, cy - r, r * 2, r * 2);
    g.DrawEllipse(&border, cx - r, cy - r, r * 2, r * 2);
}

// ============================================================
// 메인 Draw
// ============================================================

void CLineChart::Draw(Graphics& g, const CRect& plotArea, const ChartConfig& config)
{
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    std::vector<CString> labels;
    std::vector<CString> datasetNames;
    std::vector<std::vector<double>> datasets = ParseDatasets(config.dataJson, labels, datasetNames);

    if (datasets.empty())
        return;

    int nDatasets   = (int)datasets.size();
    int nPoints     = 0;
    for (auto& ds : datasets)
        nPoints = max(nPoints, (int)ds.size());

    if (nPoints < 1)
        return;

    double yMin, yMax;
    CalcYRange(datasets, yMin, yMax);
    double yRange = yMax - yMin;
    if (yRange == 0.0) yRange = 1.0;

    std::vector<Color> palette = CChartRenderer::GetColorPalette();

    FontFamily ff(L"맑은 고딕");
    Font labelFont(&ff, 9,  FontStyleRegular, UnitPoint);

    REAL plotL = (REAL)plotArea.left;
    REAL plotT = (REAL)plotArea.top;
    REAL plotW = (REAL)plotArea.Width();
    REAL plotH = (REAL)plotArea.Height();
    REAL plotB = (REAL)plotArea.bottom;

    // --------------------------------------------------------
    // 격자선 + 축
    // --------------------------------------------------------
    const int yTickCount = 6;
    Pen axisPen(Color(100, 100, 100), 1.5f);
    Pen gridPen(Color(220, 220, 220), 1.0f);
    SolidBrush axisTextBrush(Color(80, 80, 80));

    for (int i = 0; i <= yTickCount; ++i)
    {
        double val  = yMin + (yMax - yMin) * i / yTickCount;
        REAL   yPos = plotB - (REAL)((val - yMin) / yRange * plotH);

        g.DrawLine(&gridPen, plotL, yPos, plotL + plotW, yPos);

        wchar_t buf[32];
        swprintf_s(buf, 32, L"%.1f", val);
        StringFormat sf;
        sf.SetAlignment(StringAlignmentFar);
        RectF labelRect(plotL - 60.0f, yPos - 8.0f, 55.0f, 16.0f);
        g.DrawString(buf, -1, &labelFont, labelRect, &sf, &axisTextBrush);
    }

    g.DrawLine(&axisPen, plotL, plotT, plotL, plotB);
    g.DrawLine(&axisPen, plotL, plotB, plotL + plotW, plotB);

    // X축 카테고리 레이블
    REAL xStep = (nPoints > 1) ? plotW / (nPoints - 1) : 0.0f;
    for (int i = 0; i < nPoints; ++i)
    {
        REAL xPos = plotL + i * xStep;
        CString lbl = (i < (int)labels.size()) ? labels[i] : _T("");

        StringFormat sfX;
        sfX.SetAlignment(StringAlignmentCenter);
        RectF xLabelRect(xPos - 30.0f, plotB + 6.0f, 60.0f, 20.0f);
        g.DrawString(lbl, -1, &labelFont, xLabelRect, &sfX, &axisTextBrush);
    }

    // --------------------------------------------------------
    // 선 + 마커 그리기
    // --------------------------------------------------------
    for (int ds = 0; ds < nDatasets; ++ds)
    {
        const auto& values = datasets[ds];
        if (values.empty())
            continue;

        Color c = palette[ds % (int)palette.size()];
        Pen linePen(c, 2.0f);
        ApplyDashStyle(linePen, ds);

        int cnt = (int)values.size();
        std::vector<PointF> pts(cnt);
        for (int i = 0; i < cnt; ++i)
        {
            REAL xPos = plotL + (nPoints > 1 ? i * plotW / (nPoints - 1) : plotW / 2.0f);
            REAL yPos = plotB - (REAL)((values[i] - yMin) / yRange * plotH);
            pts[i] = PointF(xPos, yPos);
        }

        // 선 그리기
        if (cnt > 1)
            g.DrawLines(&linePen, pts.data(), cnt);

        // 마커 그리기
        for (auto& pt : pts)
            DrawMarker(g, pt.X, pt.Y, c);
    }

    // --------------------------------------------------------
    // 범례
    // --------------------------------------------------------
    std::vector<Color> legendColors;
    for (int i = 0; i < nDatasets; ++i)
        legendColors.push_back(palette[i % (int)palette.size()]);

    CRect fullRect(plotArea.left - 70, plotArea.top - 50,
                   plotArea.right + 150, plotArea.bottom + 60);
    CChartRenderer::DrawLegend(g, fullRect, datasetNames, legendColors);
}
