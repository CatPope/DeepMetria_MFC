// Render/IChartStrategy.h - 차트 종류별 그리기 전략 인터페이스 (Strategy 패턴)
#pragma once
namespace Gdiplus { class Graphics; }

namespace deepmetria {

struct Visualization;
class  DataSource;

class IChartStrategy
{
public:
    virtual ~IChartStrategy() = default;
    virtual void Draw(Gdiplus::Graphics& g, const Visualization& v,
                      const DataSource* dsForSummary /*nullable*/) = 0;
};

} // namespace deepmetria
