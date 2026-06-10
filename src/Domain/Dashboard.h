// Domain/Dashboard.h - 시각화 컬렉션 + 레이아웃
#pragma once
#include "Visualization.h"
#include <vector>

namespace deepmetria {

class Dashboard;

class IDashboardObserver {
public:
    virtual ~IDashboardObserver() = default;
    virtual void OnDashboardChanged(const Dashboard& dashboard) = 0;
};

class Dashboard
{
public:
    void Clear() { m_vizs.clear(); m_nextId = 1; }

    int Add(Visualization viz);
    void Remove(int id);

    int HitTest(int px, int py) const;   // -1 if none, otherwise index

    const std::vector<Visualization>& Visualizations() const { return m_vizs; }
    std::vector<Visualization>&       Visualizations()       { return m_vizs; }

private:
    std::vector<Visualization> m_vizs;
    int m_nextId = 1;
};

} // namespace deepmetria
