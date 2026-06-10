// Domain/ChainOfThinking.h - 단계적 추론 기록
#pragma once
#include <string>
#include <vector>

namespace deepmetria {

struct CotStep
{
    int          index   = 0;
    std::wstring thought;
    std::wstring action;
    std::wstring observation;
};

class ChainOfThinking
{
public:
    void Add(std::wstring thought, std::wstring action = L"", std::wstring observation = L"")
    {
        m_steps.push_back({ static_cast<int>(m_steps.size()) + 1,
                            std::move(thought),
                            std::move(action),
                            std::move(observation) });
    }
    const std::vector<CotStep>& Steps() const { return m_steps; }
    void Clear() { m_steps.clear(); }
private:
    std::vector<CotStep> m_steps;
};

} // namespace deepmetria
