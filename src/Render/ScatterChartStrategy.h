// Render/ScatterChartStrategy.h — 두 수치 컬럼 산점도
#pragma once
#include "IChartStrategy.h"

namespace deepmetria {

class ScatterChartStrategy : public IChartStrategy
{
public:
    void Draw(Gdiplus::Graphics& g, const Visualization& v,
              const DataSource* dsForSummary) override;
};

} // namespace deepmetria
