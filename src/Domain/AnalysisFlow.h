// Domain/AnalysisFlow.h - 자연어 질문 → CoT → 도구 호출 → 시각화 추가
#pragma once
#include "DataSource.h"
#include "Dashboard.h"
#include "ChainOfThinking.h"
#include "LLMRouter.h"
#include <memory>
#include <functional>
#include <thread>
#include <atomic>

namespace deepmetria {

class AnalysisFlow : public std::enable_shared_from_this<AnalysisFlow>
{
public:
    AnalysisFlow(DataSource& ds, Dashboard& dash, std::shared_ptr<LLMRouter> router);
    ~AnalysisFlow();

    using ProgressFn = std::function<void(int)>;
    using DoneFn     = std::function<void()>;

    // 비동기 시작
    void Start(std::wstring question, ProgressFn onProgress, DoneFn onDone);

    const ChainOfThinking& Reasoning() const { return m_cot; }

private:
    void Run(std::wstring question, ProgressFn onProgress, DoneFn onDone);

    DataSource&                m_ds;
    Dashboard&                 m_dash;
    std::shared_ptr<LLMRouter> m_router;
    ChainOfThinking            m_cot;
    std::thread                m_worker;
    std::atomic_bool           m_cancel{ false };
};

} // namespace deepmetria
