#pragma once

// LineChart.h — 선 차트 렌더러
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조

#include "../stdafx.h"
#include "../Common/Types.h"
#include <gdiplus.h>
#include <vector>

using namespace Gdiplus;

// ============================================================
// CLineChart — 선 차트 (정적 렌더러)
// ============================================================
class CLineChart
{
public:
    // 선 차트 그리기
    // g        : GDI+ Graphics 객체
    // plotArea : 플롯 영역
    // config   : 차트 설정 (ChartConfig.dataJson에 데이터 포함)
    static void Draw(Graphics& g, const CRect& plotArea, const ChartConfig& config);

private:
    // JSON에서 datasets 파싱
    static std::vector<std::vector<double>> ParseDatasets(const CString& dataJson,
                                                           std::vector<CString>& outLabels,
                                                           std::vector<CString>& outDatasetNames);

    // Y축 최대/최소값 계산
    static void CalcYRange(const std::vector<std::vector<double>>& datasets,
                           double& outMin, double& outMax);

    // 데이터셋별 대시 패턴 (다중 데이터셋 구분)
    static void ApplyDashStyle(Pen& pen, int datasetIdx);

    // 데이터 포인트에 원형 마커 그리기 (반경 4px)
    static void DrawMarker(Graphics& g, REAL cx, REAL cy, const Color& color);
};
