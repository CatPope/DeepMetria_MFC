// Domain/Dashboard.cpp
#include "stdafx.h"
#include "Dashboard.h"

namespace deepmetria {

int Dashboard::Add(Visualization viz)
{
    viz.id = m_nextId++;
    m_vizs.push_back(std::move(viz));
    return m_vizs.back().id;
}

void Dashboard::Remove(int id)
{
    for (auto it = m_vizs.begin(); it != m_vizs.end(); ++it)
        if (it->id == id) { m_vizs.erase(it); return; }
}

int Dashboard::HitTest(int px, int py) const
{
    // 위에서부터 z순. 마지막에 그려진(=벡터 뒤쪽) 것이 위에 있다고 가정.
    for (int i = static_cast<int>(m_vizs.size()) - 1; i >= 0; --i)
    {
        const auto& v = m_vizs[i];
        if (px >= v.x && px <= v.x + v.width &&
            py >= v.y && py <= v.y + v.height)
            return i;
    }
    return -1;
}

} // namespace deepmetria
