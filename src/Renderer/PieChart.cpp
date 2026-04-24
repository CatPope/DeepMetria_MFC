#include "stdafx.h"

// PieChart.cpp — 파이/도넛 차트 렌더러
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조

#include "PieChart.h"
#include "ChartRenderer.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <numeric>

using namespace Gdiplus;

// ============================================================
// JSON 파싱
// 형식: { "labels": ["A","B",...], "values": [10, 20, ...] }
// ============================================================

std::vector<double> CPieChart::ParseValues(const CString& dataJson,
                                            std::vector<CString>& outLabels)
{
    std::vector<double> result;
    outLabels.clear();

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

    // values 파싱
    size_t valPos    = json.find(L"\"values\"");
    size_t vArrStart = json.find(L'[', valPos);
    size_t vArrEnd   = json.find(L']', vArrStart);
    if (vArrStart != std::wstring::npos && vArrEnd != std::wstring::npos)
    {
        std::wstring valStr = json.substr(vArrStart + 1, vArrEnd - vArrStart - 1);
        std::wistringstream vss(valStr);
        std::wstring vtoken;
        while (std::getline(vss, vtoken, L','))
        {
            try { result.push_back(std::stod(vtoken)); }
            catch (...) { result.push_back(0.0); }
        }
    }

    return result;
}

// ============================================================
// 작은 조각 합산
// ============================================================

void CPieChart::MergeSmallSlices(std::vector<double>& values,
                                  std::vector<CString>& labels,
                                  double threshold)
{
    double total = std::accumulate(values.begin(), values.end(), 0.0);
    if (total <= 0.0)
        return;

    std::vector<double> newValues;
    std::vector<CString> newLabels;
    double otherSum = 0.0;

    for (size_t i = 0; i < values.size(); ++i)
    {
        if (values[i] / total < threshold)
            otherSum += values[i];
        else
        {
            newValues.push_back(values[i]);
            newLabels.push_back((i < labels.size()) ? labels[i] : _T(""));
        }
    }

    if (otherSum > 0.0)
    {
        newValues.push_back(otherSum);
        newLabels.push_back(_T("기타"));
    }

    values = newValues;
    labels = newLabels;
}

// ============================================================
// 조각 퍼센트 레이블
// ============================================================

void CPieChart::DrawSliceLabel(Graphics& g, REAL cx, REAL cy,
                                REAL radius, REAL startAngle, REAL sweepAngle,
                                double percent, const Font& font)
{
    // 조각 중앙 각도
    const REAL PI = 3.14159265f;
    REAL midAngle = (startAngle + sweepAngle / 2.0f) * PI / 180.0f;

    // 레이블 위치 (반지름 60% 지점)
    REAL lx = cx + radius * 0.6f * std::cos(midAngle);
    REAL ly = cy + radius * 0.6f * std::sin(midAngle);

    wchar_t buf[16];
    swprintf_s(buf, 16, L"%.1f%%", percent * 100.0);

    SolidBrush brush(Color::White);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    RectF labelRect(lx - 25.0f, ly - 10.0f, 50.0f, 20.0f);
    g.DrawString(buf, -1, &font, labelRect, &sf, &brush);
}

// ============================================================
// 그라디언트로 조각 채우기 (약한 3D 효과)
// ============================================================

void CPieChart::FillSliceGradient(Graphics& g, const RectF& bounds,
                                   REAL startAngle, REAL sweepAngle,
                                   const Color& baseColor)
{
    // 밝은 색 (상단 하이라이트)
    Color lightColor(
        (BYTE)min(255, (int)baseColor.GetR() + 60),
        (BYTE)min(255, (int)baseColor.GetG() + 60),
        (BYTE)min(255, (int)baseColor.GetB() + 60));

    LinearGradientBrush brush(
        PointF(bounds.X, bounds.Y),
        PointF(bounds.X + bounds.Width, bounds.Y + bounds.Height),
        lightColor, baseColor);

    GraphicsPath path;
    path.AddPie(bounds.X, bounds.Y, bounds.Width, bounds.Height,
                startAngle, sweepAngle);
    g.FillPath(&brush, &path);
}

// ============================================================
// 메인 Draw
// ============================================================

void CPieChart::Draw(Graphics& g, const CRect& plotArea, const ChartConfig& config,
                     BOOL bDonut)
{
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    std::vector<CString> labels;
    std::vector<double> values = ParseValues(config.dataJson, labels);

    if (values.empty())
        return;

    // 5% 미만 조각 합산
    MergeSmallSlices(values, labels, 0.05);

    double total = std::accumulate(values.begin(), values.end(), 0.0);
    if (total <= 0.0)
        return;

    std::vector<Color> palette = CChartRenderer::GetColorPalette();

    FontFamily ff(L"맑은 고딕");
    Font labelFont(&ff, 10, FontStyleRegular, UnitPoint);

    // 파이 영역 계산 (범례 공간 제외, 정사각형으로)
    REAL plotW = (REAL)plotArea.Width();
    REAL plotH = (REAL)plotArea.Height();
    REAL diameter = min(plotW * 0.75f, plotH);  // 범례 공간 확보
    REAL radius   = diameter / 2.0f;

    // 파이 중심
    REAL cx = (REAL)plotArea.left + diameter / 2.0f + (plotW - diameter * 1.05f) / 2.0f;
    REAL cy = (REAL)plotArea.top  + plotH / 2.0f;

    RectF pieRect(cx - radius, cy - radius, diameter, diameter);

    // --------------------------------------------------------
    // 조각 그리기
    // --------------------------------------------------------
    REAL startAngle = -90.0f; // 12시 방향에서 시작

    for (size_t i = 0; i < values.size(); ++i)
    {
        REAL sweepAngle = (REAL)(values[i] / total * 360.0);
        Color c = palette[i % (int)palette.size()];

        // 그라디언트 채우기
        FillSliceGradient(g, pieRect, startAngle, sweepAngle, c);

        // 경계선
        Pen borderPen(Color(255, 255, 255), 2.0f);
        g.DrawPie(&borderPen, pieRect.X, pieRect.Y, pieRect.Width, pieRect.Height,
                  startAngle, sweepAngle);

        // 퍼센트 레이블 (조각이 충분히 크면)
        double pct = values[i] / total;
        if (pct >= 0.04)
            DrawSliceLabel(g, cx, cy, radius, startAngle, sweepAngle, pct, labelFont);

        startAngle += sweepAngle;
    }

    // --------------------------------------------------------
    // 도넛 차트: 중앙 원 제거
    // --------------------------------------------------------
    if (bDonut)
    {
        REAL holeR = radius * 0.45f;
        SolidBrush holeBrush(Color::White);
        g.FillEllipse(&holeBrush, cx - holeR, cy - holeR, holeR * 2, holeR * 2);
    }

    // --------------------------------------------------------
    // 범례
    // --------------------------------------------------------
    std::vector<Color> legendColors;
    for (size_t i = 0; i < values.size(); ++i)
        legendColors.push_back(palette[i % (int)palette.size()]);

    CRect fullRect(plotArea.left - 70, plotArea.top - 50,
                   plotArea.right + 150, plotArea.bottom + 60);
    CChartRenderer::DrawLegend(g, fullRect, labels, legendColors);
}
