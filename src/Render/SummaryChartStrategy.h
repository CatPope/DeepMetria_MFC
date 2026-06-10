// Render/SummaryChartStrategy.h - 데이터 요약 전략
#pragma once
#include "IChartStrategy.h"

namespace deepmetria {

class SummaryChartStrategy : public IChartStrategy
{
public:
    void Draw(Gdiplus::Graphics& g, const Visualization& v,
              const DataSource* dsForSummary) override;
};

} // namespace deepmetria
