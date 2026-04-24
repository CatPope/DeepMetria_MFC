#pragma once

// PieChart.h — 파이/도넛 차트 렌더러
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조

#include "../stdafx.h"
#include "../Common/Types.h"
#include <gdiplus.h>
#include <vector>

using namespace Gdiplus;

// ============================================================
// CPieChart — 파이/도넛 차트 (정적 렌더러)
// ============================================================
class CPieChart
{
public:
    // 파이 차트 그리기
    // g        : GDI+ Graphics 객체
    // plotArea : 플롯 영역
    // config   : 차트 설정 (ChartConfig.dataJson에 데이터 포함)
    // bDonut   : TRUE이면 도넛 차트 (중앙 구멍)
    static void Draw(Graphics& g, const CRect& plotArea, const ChartConfig& config,
                     BOOL bDonut = FALSE);

private:
    // JSON에서 단일 시리즈 파싱
    static std::vector<double> ParseValues(const CString& dataJson,
                                           std::vector<CString>& outLabels);

    // 5% 미만 조각을 "기타"로 합산
    // 반환: 병합된 values, labels
    static void MergeSmallSlices(std::vector<double>& values,
                                 std::vector<CString>& labels,
                                 double threshold = 0.05);

    // 조각에 퍼센트 레이블 그리기
    static void DrawSliceLabel(Graphics& g, REAL cx, REAL cy,
                               REAL radius, REAL startAngle, REAL sweepAngle,
                               double percent, const Font& font);

    // 그라디언트 브러시로 조각 채우기 (약한 3D 효과)
    static void FillSliceGradient(Graphics& g, const RectF& bounds,
                                  REAL startAngle, REAL sweepAngle,
                                  const Color& baseColor);
};
