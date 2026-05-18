#pragma once

// BarChart.h — 수직 막대 차트 렌더러
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조

#include "../Common/Types.h"
#include <gdiplus.h>
#include <vector>

// ============================================================
// CBarChart — 수직 막대 차트 (정적 렌더러)
// ============================================================
class CBarChart
{
public:
    // 막대 차트 그리기
    // g        : GDI+ Graphics 객체
    // plotArea : 플롯 영역 (CalcPlotArea로 계산된 값)
    // config   : 차트 설정 (ChartConfig.dataJson에 데이터 포함)
    static void Draw(Gdiplus::Graphics& g, const CRect& plotArea, const ChartConfig& config);

private:
    // JSON에서 datasets 파싱
    // 반환: datasets[datasetIdx][categoryIdx] = value
    static std::vector<std::vector<double>> ParseDatasets(const CString& dataJson,
                                                           std::vector<CString>& outLabels,
                                                           std::vector<CString>& outDatasetNames);

    // Y축 최대값 계산 (nice number로 올림)
    static double CalcNiceMax(double rawMax);

    // Y축 눈금 개수 (5~8)
    static int CalcTickCount(double niceMax);

    // 바 위에 값 레이블 그리기
    static void DrawValueLabel(Gdiplus::Graphics& g, double value,
                               Gdiplus::REAL cx, Gdiplus::REAL top, const Gdiplus::Font& font);
};
