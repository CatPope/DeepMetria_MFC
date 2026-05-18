#pragma once

// ScatterChart.h — 산점도 렌더러
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조

#include "../Common/Types.h"
#include <gdiplus.h>
#include <vector>

// ============================================================
// CScatterChart — 산점도 (정적 렌더러)
// ============================================================
class CScatterChart
{
public:
    // 산점도 그리기
    // g           : GDI+ Graphics 객체
    // plotArea    : 플롯 영역
    // config      : 차트 설정 (ChartConfig.dataJson에 데이터 포함)
    // bTrendLine  : TRUE이면 선형 회귀선 그리기
    static void Draw(Gdiplus::Graphics& g, const CRect& plotArea, const ChartConfig& config,
                     BOOL bTrendLine = FALSE);

private:
    // XY 쌍 구조체
    struct PointXY {
        double x;
        double y;
    };

    // JSON에서 XY 쌍 파싱
    // 형식: { "datasets": [ { "name": "...", "points": [[x,y], ...] }, ... ] }
    static std::vector<std::vector<PointXY>> ParseDatasets(const CString& dataJson,
                                                            std::vector<CString>& outNames);

    // X/Y 범위 계산
    static void CalcRange(const std::vector<std::vector<PointXY>>& datasets,
                          double& xMin, double& xMax, double& yMin, double& yMax);

    // 원형 마커 그리기 (반경 5px, 반투명)
    static void DrawMarker(Gdiplus::Graphics& g, Gdiplus::REAL cx, Gdiplus::REAL cy, const Gdiplus::Color& color);

    // 선형 회귀선 그리기
    static void DrawTrendLine(Gdiplus::Graphics& g, const std::vector<PointXY>& points,
                              const CRect& plotArea,
                              double xMin, double xMax, double yMin, double yMax,
                              const Gdiplus::Color& color);

    // 선형 회귀 계수 계산 (y = a*x + b)
    static void LinearRegression(const std::vector<PointXY>& points,
                                 double& outA, double& outB);
};
