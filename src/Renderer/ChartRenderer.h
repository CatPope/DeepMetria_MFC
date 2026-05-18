#pragma once

// ChartRenderer.h — GDI+ 차트 렌더링 팩토리/디스패처
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조

#include "../Common/Types.h"
#include <gdiplus.h>
#include <vector>

// ============================================================
// CChartRenderer — 차트 렌더링 팩토리 (정적 메서드 집합)
// ============================================================
class CChartRenderer
{
public:
    // 차트 렌더링 (CDC에 직접 그리기)
    static void Render(CDC* pDC, const CRect& rect, const ChartConfig& config);

    // 파일로 내보내기 (PNG/BMP, GDI+ Image::Save)
    static void RenderToFile(const CString& filePath, int width, int height, const ChartConfig& config);

    // --------------------------------------------------------
    // 공용 유틸리티 (차트 클래스들이 공유)
    // --------------------------------------------------------

    // 차트 제목 그리기 (상단 중앙, 14pt)
    static void DrawTitle(Gdiplus::Graphics& g, const CRect& rect, const CString& title);

    // 범례 그리기 (우측, 10pt)
    static void DrawLegend(Gdiplus::Graphics& g, const CRect& rect,
                           const std::vector<CString>& names,
                           const std::vector<Gdiplus::Color>& colors);

    // 격자선 그리기 (플롯 영역 내부)
    static void DrawGridLines(Gdiplus::Graphics& g, const CRect& plotArea,
                              int xCount, int yCount);

    // X축/Y축 레이블 그리기
    static void DrawAxisLabels(Gdiplus::Graphics& g, const CRect& plotArea,
                               const CString& xLabel, const CString& yLabel);

    // 12색 기본 팔레트 반환
    static std::vector<Gdiplus::Color> GetColorPalette();

    // 여백 제외한 플롯 영역 계산
    // 여백: 상=50(제목), 하=60(X축), 좌=70(Y축), 우=150(범례)
    static CRect CalcPlotArea(const CRect& totalRect, BOOL bLegend = TRUE);

    // GDI+ 초기화 (앱 시작 시 1회)
    static BOOL InitGdiplus();

    // GDI+ 종료 (앱 종료 시 1회)
    static void ShutdownGdiplus();

private:
    static ULONG_PTR s_gdiplusToken;  // GDI+ 토큰
};
