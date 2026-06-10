// Render/TableChartStrategy.h - 표 전략
#pragma once
#include "IChartStrategy.h"

namespace deepmetria {

class TableChartStrategy : public IChartStrategy
{
public:
    void Draw(Gdiplus::Graphics& g, const Visualization& v,
              const DataSource* dsForSummary) override;
};

} // namespace deepmetria
