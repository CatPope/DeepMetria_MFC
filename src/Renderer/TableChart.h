#pragma once

// TableChart.h — 테이블 시각화 렌더러
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조

#include "../Common/Types.h"
#include <gdiplus.h>
#include <vector>

class CTableChart
{
public:
    static void Draw(Gdiplus::Graphics& g, const CRect& plotArea, const ChartConfig& config);

private:
    static void ParseTableData(const CString& dataJson,
                               std::vector<CString>& outColumns,
                               std::vector<std::vector<CString>>& outRows);
};
