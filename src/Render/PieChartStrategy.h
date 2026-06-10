// Render/PieChartStrategy.h - 파이 차트 전략
#pragma once
#include "IChartStrategy.h"

namespace deepmetria {

class PieChartStrategy : public IChartStrategy
{
public:
    void Draw(Gdiplus::Graphics& g, const Visualization& v,
              const DataSource* dsForSummary) override;
};

} // namespace deepmetria
