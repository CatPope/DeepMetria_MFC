#include "stdafx.h"

// BarChart.cpp — 수직 막대 차트 렌더러
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조

#include "BarChart.h"
#include "ChartRenderer.h"
#include <cmath>
#include <algorithm>
#include <sstream>

using namespace Gdiplus;

// ============================================================
// JSON 파싱 헬퍼
// 형식: { "labels": ["A","B",...], "datasets": [ { "name":"...", "values":[1,2,...] }, ... ] }
// ============================================================

std::vector<std::vector<double>> CBarChart::ParseDatasets(const CString& dataJson,
                                                           std::vector<CString>& outLabels,
                                                           std::vector<CString>& outDatasetNames)
{
    std::vector<std::vector<double>> result;
    outLabels.clear();
    outDatasetNames.clear();

    // 간단한 수동 파싱 (외부 JSON 라이브러리 미사용)
    std::wstring json = dataJson.GetString();

    // labels 배열 파싱
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
                // 따옴표 제거
                size_t q1 = token.find(L'"');
                size_t q2 = token.rfind(L'"');
                if (q1 != std::wstring::npos && q2 != q1)
                    outLabels.push_back(CString(token.substr(q1 + 1, q2 - q1 - 1).c_str()));
                else
                    outLabels.push_back(CString(token.c_str()));
            }
        }
    }

    // datasets 배열 파싱
    size_t datasetsPos = json.find(L"\"datasets\"");
    if (datasetsPos == std::wstring::npos)
        return result;

    size_t arrStart = json.find(L'[', datasetsPos);
    if (arrStart == std::wstring::npos)
        return result;

    // 각 dataset 객체 { ... } 추출
    size_t pos = arrStart + 1;
    while (pos < json.size())
    {
        size_t objStart = json.find(L'{', pos);
        if (objStart == std::wstring::npos)
            break;

        // 중괄호 매칭
        int depth = 1;
        size_t objEnd = objStart + 1;
        while (objEnd < json.size() && depth > 0)
        {
            if (json[objEnd] == L'{') ++depth;
            else if (json[objEnd] == L'}') --depth;
            ++objEnd;
        }

        std::wstring obj = json.substr(objStart + 1, objEnd - objStart - 2);

        // name 파싱
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

        // values 배열 파싱
        std::vector<double> values;
        size_t valPos   = obj.find(L"\"values\"");
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
// Y축 스케일 유틸
// ============================================================

double CBarChart::CalcNiceMax(double rawMax)
{
    if (rawMax <= 0.0) return 10.0;
    double magnitude = std::pow(10.0, std::floor(std::log10(rawMax)));
    double normalized = rawMax / magnitude;
    double nice = (normalized <= 1.0) ? 1.0 :
                  (normalized <= 2.0) ? 2.0 :
                  (normalized <= 5.0) ? 5.0 : 10.0;
    return nice * magnitude;
}

int CBarChart::CalcTickCount(double niceMax)
{
    // 5~8 사이 눈금 수
    if (niceMax <= 0) return 5;
    double step = niceMax / 5.0;
    double mag  = std::pow(10.0, std::floor(std::log10(step)));
    double norm = step / mag;
    return (norm <= 2.0) ? 5 : (norm <= 5.0) ? 6 : 8;
}

// ============================================================
// 값 레이블 그리기
// ============================================================

void CBarChart::DrawValueLabel(Gdiplus::Graphics& g, double value,
                               Gdiplus::REAL cx, Gdiplus::REAL top, const Gdiplus::Font& font)
{
    SolidBrush brush(Color(50, 50, 50));
    wchar_t buf[32];
    // 소수점 1자리 (정수면 정수로)
    if (value == std::floor(value))
        swprintf_s(buf, 32, L"%.0f", value);
    else
        swprintf_s(buf, 32, L"%.1f", value);

    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    RectF rect(cx - 30.0f, top - 18.0f, 60.0f, 16.0f);
    g.DrawString(buf, -1, &font, rect, &sf, &brush);
}

// ============================================================
// 메인 Draw
// ============================================================

void CBarChart::Draw(Graphics& g, const CRect& plotArea, const ChartConfig& config)
{
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    std::vector<CString> labels;
    std::vector<CString> datasetNames;
    std::vector<std::vector<double>> datasets = ParseDatasets(config.dataJson, labels, datasetNames);

    if (datasets.empty())
        return;

    int nDatasets  = (int)datasets.size();
    int nCategories = 0;
    for (auto& ds : datasets)
        nCategories = max(nCategories, (int)ds.size());

    if (nCategories == 0)
        return;

    // 최대값 계산
    double rawMax = 0.0;
    for (auto& ds : datasets)
        for (double v : ds)
            rawMax = max(rawMax, v);

    double yMax     = CalcNiceMax(rawMax);
    int    tickCount = CalcTickCount(yMax);

    std::vector<Color> palette = CChartRenderer::GetColorPalette();

    FontFamily ff(L"맑은 고딕");
    Gdiplus::Font labelFont(&ff, 9,  FontStyleRegular, UnitPoint);
    Gdiplus::Font valueFont(&ff, 8,  FontStyleRegular, UnitPoint);

    REAL plotW = (REAL)plotArea.Width();
    REAL plotH = (REAL)plotArea.Height();
    REAL plotL = (REAL)plotArea.left;
    REAL plotT = (REAL)plotArea.top;
    REAL plotB = (REAL)plotArea.bottom;

    // --------------------------------------------------------
    // 격자선 + Y축 눈금/레이블
    // --------------------------------------------------------
    Pen axisPen(Color(100, 100, 100), 1.5f);
    Pen gridPen(Color(220, 220, 220), 1.0f);
    SolidBrush axisTextBrush(Color(80, 80, 80));

    // Y축 눈금선 및 레이블
    for (int i = 0; i <= tickCount; ++i)
    {
        double val  = yMax * i / tickCount;
        REAL   yPos = plotB - (REAL)(val / yMax * plotH);

        // 격자선
        g.DrawLine(&gridPen, plotL, yPos, plotL + plotW, yPos);

        // 눈금 레이블
        wchar_t buf[32];
        if (val == std::floor(val))
            swprintf_s(buf, 32, L"%.0f", val);
        else
            swprintf_s(buf, 32, L"%.1f", val);

        StringFormat sf;
        sf.SetAlignment(StringAlignmentFar);
        RectF labelRect(plotL - 60.0f, yPos - 8.0f, 55.0f, 16.0f);
        g.DrawString(buf, -1, &labelFont, labelRect, &sf, &axisTextBrush);
    }

    // --------------------------------------------------------
    // 축 그리기
    // --------------------------------------------------------
    g.DrawLine(&axisPen, plotL, plotT, plotL, plotB);       // Y축
    g.DrawLine(&axisPen, plotL, plotB, plotL + plotW, plotB); // X축

    // --------------------------------------------------------
    // 막대 그리기
    // --------------------------------------------------------
    REAL groupW    = plotW / nCategories;          // 카테고리 하나의 너비
    REAL barPadding = groupW * 0.1f;               // 카테고리 양쪽 패딩
    REAL totalBarW  = groupW - barPadding * 2.0f;  // 카테고리 내 전체 막대 너비
    REAL singleBarW = (nDatasets > 0) ? totalBarW / nDatasets : totalBarW;

    for (int cat = 0; cat < nCategories; ++cat)
    {
        REAL groupX = plotL + cat * groupW + barPadding;

        for (int ds = 0; ds < nDatasets; ++ds)
        {
            if (cat >= (int)datasets[ds].size())
                continue;

            double val   = datasets[ds][cat];
            REAL   barH  = (yMax > 0) ? (REAL)(val / yMax * plotH) : 0.0f;
            REAL   barX  = groupX + ds * singleBarW;
            REAL   barY  = plotB - barH;

            Color c = palette[ds % (int)palette.size()];

            // 막대 채우기
            SolidBrush barBrush(c);
            g.FillRectangle(&barBrush, barX, barY, singleBarW - 2.0f, barH);

            // 막대 테두리
            Color borderC(
                (BYTE)max(0, (int)c.GetR() - 40),
                (BYTE)max(0, (int)c.GetG() - 40),
                (BYTE)max(0, (int)c.GetB() - 40));
            Pen barPen(borderC, 0.8f);
            g.DrawRectangle(&barPen, barX, barY, singleBarW - 2.0f, barH);

            // 값 레이블 (막대 위)
            if (barH > 12.0f)
                DrawValueLabel(g, val, barX + (singleBarW - 2.0f) / 2.0f, barY, valueFont);
        }

        // X축 카테고리 레이블
        CString catLabel = (cat < (int)labels.size()) ? labels[cat] : _T("");
        StringFormat sfX;
        sfX.SetAlignment(StringAlignmentCenter);
        sfX.SetTrimming(StringTrimmingEllipsisCharacter);
        RectF catRect(groupX, plotB + 6.0f, groupW - barPadding, 20.0f);
        g.DrawString(catLabel, -1, &labelFont, catRect, &sfX, &axisTextBrush);
    }

    // --------------------------------------------------------
    // 범례 그리기
    // --------------------------------------------------------
    std::vector<Color> legendColors;
    for (int i = 0; i < nDatasets; ++i)
        legendColors.push_back(palette[i % (int)palette.size()]);

    CRect fullRect(plotArea.left - 70, plotArea.top - 50,
                   plotArea.right + 150, plotArea.bottom + 60);
    CChartRenderer::DrawLegend(g, fullRect, datasetNames, legendColors);
}
