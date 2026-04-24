#include "stdafx.h"

// ChartRenderer.cpp — GDI+ 차트 렌더링 팩토리/디스패처
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조

#include "ChartRenderer.h"
#include "BarChart.h"
#include "LineChart.h"
#include "PieChart.h"
#include "ScatterChart.h"
#include <gdiplus.h>

using namespace Gdiplus;

// 정적 멤버 초기화
ULONG_PTR CChartRenderer::s_gdiplusToken = 0;

// ============================================================
// GDI+ 초기화/종료
// ============================================================

BOOL CChartRenderer::InitGdiplus()
{
    GdiplusStartupInput input;
    Status status = GdiplusStartup(&s_gdiplusToken, &input, nullptr);
    return (status == Ok);
}

void CChartRenderer::ShutdownGdiplus()
{
    if (s_gdiplusToken != 0)
    {
        GdiplusShutdown(s_gdiplusToken);
        s_gdiplusToken = 0;
    }
}

// ============================================================
// 메인 렌더링 디스패처
// ============================================================

void CChartRenderer::Render(CDC* pDC, const CRect& rect, const ChartConfig& config)
{
    if (!pDC || rect.IsRectEmpty())
        return;

    // GDI+ Graphics 생성
    Graphics g(pDC->GetSafeHdc());
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    // 배경 흰색으로 채우기
    SolidBrush bgBrush(Color::White);
    g.FillRectangle(&bgBrush, rect.left, rect.top, rect.Width(), rect.Height());

    // 제목 그리기
    if (!config.title.IsEmpty())
        DrawTitle(g, rect, config.title);

    // 플롯 영역 계산
    CRect plotArea = CalcPlotArea(rect, TRUE);

    // 축 레이블 그리기
    DrawAxisLabels(g, rect, config.xLabel, config.yLabel);

    // chartType에 따라 차트 클래스 디스패치
    CString type = config.chartType;
    type.MakeLower();

    if (type == _T("bar"))
        CBarChart::Draw(g, plotArea, config);
    else if (type == _T("line"))
        CLineChart::Draw(g, plotArea, config);
    else if (type == _T("pie"))
        CPieChart::Draw(g, plotArea, config);
    else if (type == _T("scatter"))
        CScatterChart::Draw(g, plotArea, config);
    else
    {
        // 알 수 없는 차트 타입 — 오류 텍스트 표시
        FontFamily ff(L"맑은 고딕");
        Font font(&ff, 12, FontStyleRegular, UnitPoint);
        SolidBrush brush(Color(200, 0, 0));
        CString msg = _T("알 수 없는 차트 유형: ") + config.chartType;
        g.DrawString(msg, -1, &font,
                     PointF((REAL)rect.left + 10, (REAL)rect.top + 10),
                     &brush);
    }
}

// ============================================================
// 파일 내보내기 (PNG/BMP)
// ============================================================

void CChartRenderer::RenderToFile(const CString& filePath, int width, int height,
                                   const ChartConfig& config)
{
    if (filePath.IsEmpty() || width <= 0 || height <= 0)
        return;

    // 메모리 비트맵 생성
    Bitmap bitmap(width, height, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    CRect rect(0, 0, width, height);

    // 배경 흰색
    SolidBrush bgBrush(Color::White);
    g.FillRectangle(&bgBrush, 0, 0, width, height);

    // 제목
    if (!config.title.IsEmpty())
        DrawTitle(g, rect, config.title);

    // 플롯 영역
    CRect plotArea = CalcPlotArea(rect, TRUE);

    // 축 레이블
    DrawAxisLabels(g, rect, config.xLabel, config.yLabel);

    // 차트 디스패치
    CString type = config.chartType;
    type.MakeLower();

    if (type == _T("bar"))
        CBarChart::Draw(g, plotArea, config);
    else if (type == _T("line"))
        CLineChart::Draw(g, plotArea, config);
    else if (type == _T("pie"))
        CPieChart::Draw(g, plotArea, config);
    else if (type == _T("scatter"))
        CScatterChart::Draw(g, plotArea, config);

    // 파일 확장자로 포맷 결정
    CString ext = filePath.Right(4);
    ext.MakeLower();

    CLSID encoderClsid;
    if (ext == _T(".png"))
    {
        // PNG 인코더 CLSID
        UINT numEncoders = 0, size = 0;
        GetImageEncodersSize(&numEncoders, &size);
        std::vector<BYTE> buf(size);
        ImageCodecInfo* pCodecInfo = reinterpret_cast<ImageCodecInfo*>(buf.data());
        GetImageEncoders(numEncoders, size, pCodecInfo);
        for (UINT i = 0; i < numEncoders; ++i)
        {
            if (CString(pCodecInfo[i].MimeType) == _T("image/png"))
            {
                encoderClsid = pCodecInfo[i].Clsid;
                break;
            }
        }
    }
    else
    {
        // BMP 인코더 CLSID
        UINT numEncoders = 0, size = 0;
        GetImageEncodersSize(&numEncoders, &size);
        std::vector<BYTE> buf(size);
        ImageCodecInfo* pCodecInfo = reinterpret_cast<ImageCodecInfo*>(buf.data());
        GetImageEncoders(numEncoders, size, pCodecInfo);
        for (UINT i = 0; i < numEncoders; ++i)
        {
            if (CString(pCodecInfo[i].MimeType) == _T("image/bmp"))
            {
                encoderClsid = pCodecInfo[i].Clsid;
                break;
            }
        }
    }

    bitmap.Save(filePath, &encoderClsid, nullptr);
}

// ============================================================
// 공용 유틸리티
// ============================================================

void CChartRenderer::DrawTitle(Graphics& g, const CRect& rect, const CString& title)
{
    FontFamily ff(L"맑은 고딕");
    Font font(&ff, 14, FontStyleBold, UnitPoint);
    SolidBrush brush(Color(50, 50, 50));

    // 문자열 크기 측정 후 중앙 정렬
    RectF layoutRect((REAL)rect.left, (REAL)rect.top + 10,
                     (REAL)rect.Width(), 30.0f);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentNear);

    g.DrawString(title, -1, &font, layoutRect, &sf, &brush);
}

void CChartRenderer::DrawLegend(Graphics& g, const CRect& rect,
                                 const std::vector<CString>& names,
                                 const std::vector<Color>& colors)
{
    if (names.empty())
        return;

    FontFamily ff(L"맑은 고딕");
    Font font(&ff, 10, FontStyleRegular, UnitPoint);
    SolidBrush textBrush(Color(50, 50, 50));

    // 범례 시작 위치 (우측 여백)
    REAL x = (REAL)(rect.right - 140);
    REAL y = (REAL)(rect.top + 60);
    const REAL boxSize = 12.0f;
    const REAL lineH   = 20.0f;

    for (size_t i = 0; i < names.size(); ++i)
    {
        // 색상 사각형
        Color c = (i < colors.size()) ? colors[i] : Color(128, 128, 128);
        SolidBrush boxBrush(c);
        g.FillRectangle(&boxBrush, x, y + (REAL)i * lineH + 2.0f, boxSize, boxSize);

        // 테두리
        Pen borderPen(Color(80, 80, 80), 0.5f);
        g.DrawRectangle(&borderPen, x, y + (REAL)i * lineH + 2.0f, boxSize, boxSize);

        // 이름
        g.DrawString(names[i], -1, &font,
                     PointF(x + boxSize + 4.0f, y + (REAL)i * lineH),
                     &textBrush);
    }
}

void CChartRenderer::DrawGridLines(Graphics& g, const CRect& plotArea,
                                    int xCount, int yCount)
{
    Pen gridPen(Color(200, 200, 200), 1.0f);

    // 수평 격자선 (Y축 방향)
    if (yCount > 0)
    {
        REAL step = (REAL)plotArea.Height() / yCount;
        for (int i = 0; i <= yCount; ++i)
        {
            REAL y = (REAL)plotArea.top + i * step;
            g.DrawLine(&gridPen,
                       (REAL)plotArea.left, y,
                       (REAL)plotArea.right, y);
        }
    }

    // 수직 격자선 (X축 방향)
    if (xCount > 0)
    {
        REAL step = (REAL)plotArea.Width() / xCount;
        for (int i = 0; i <= xCount; ++i)
        {
            REAL x = (REAL)plotArea.left + i * step;
            g.DrawLine(&gridPen,
                       x, (REAL)plotArea.top,
                       x, (REAL)plotArea.bottom);
        }
    }
}

void CChartRenderer::DrawAxisLabels(Graphics& g, const CRect& rect,
                                     const CString& xLabel, const CString& yLabel)
{
    FontFamily ff(L"맑은 고딕");
    Font font(&ff, 9, FontStyleRegular, UnitPoint);
    SolidBrush brush(Color(80, 80, 80));

    // X축 레이블 — 하단 중앙
    if (!xLabel.IsEmpty())
    {
        StringFormat sfX;
        sfX.SetAlignment(StringAlignmentCenter);
        RectF xRect((REAL)rect.left, (REAL)(rect.bottom - 20),
                    (REAL)rect.Width(), 20.0f);
        g.DrawString(xLabel, -1, &font, xRect, &sfX, &brush);
    }

    // Y축 레이블 — 좌측 수직 (90도 회전)
    if (!yLabel.IsEmpty())
    {
        // 회전 변환 적용
        Matrix oldMatrix;
        g.GetTransform(&oldMatrix);

        REAL cx = (REAL)(rect.left + 12);
        REAL cy = (REAL)(rect.top + rect.Height() / 2);

        Matrix rotMatrix;
        rotMatrix.RotateAt(-90.0f, PointF(cx, cy));
        g.SetTransform(&rotMatrix);

        StringFormat sfY;
        sfY.SetAlignment(StringAlignmentCenter);
        RectF yRect(cx - 60.0f, cy - 10.0f, 120.0f, 20.0f);
        g.DrawString(yLabel, -1, &font, yRect, &sfY, &brush);

        g.SetTransform(&oldMatrix);
    }
}

std::vector<Color> CChartRenderer::GetColorPalette()
{
    return {
        Color(65,  105, 225),   // 로얄블루
        Color(220, 20,  60),    // 크림슨
        Color(50,  205, 50),    // 라임그린
        Color(255, 165, 0),     // 오렌지
        Color(138, 43,  226),   // 블루바이올렛
        Color(0,   206, 209),   // 다크터콰이즈
        Color(255, 105, 180),   // 핫핑크
        Color(34,  139, 34),    // 포레스트그린
        Color(255, 215, 0),     // 골드
        Color(70,  130, 180),   // 스틸블루
        Color(205, 92,  92),    // 인디언레드
        Color(147, 112, 219),   // 미디엄퍼플
    };
}

CRect CChartRenderer::CalcPlotArea(const CRect& totalRect, BOOL bLegend)
{
    // 여백: 상=50(제목), 하=60(X축), 좌=70(Y축), 우=150(범례) or 20(범례 없음)
    int marginTop    = 50;
    int marginBottom = 60;
    int marginLeft   = 70;
    int marginRight  = bLegend ? 150 : 20;

    CRect plot;
    plot.left   = totalRect.left   + marginLeft;
    plot.top    = totalRect.top    + marginTop;
    plot.right  = totalRect.right  - marginRight;
    plot.bottom = totalRect.bottom - marginBottom;

    // 최소 크기 보장
    if (plot.right  <= plot.left)   plot.right  = plot.left  + 10;
    if (plot.bottom <= plot.top)    plot.bottom = plot.top   + 10;

    return plot;
}
