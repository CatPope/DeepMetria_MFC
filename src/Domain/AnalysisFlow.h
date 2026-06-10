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

    // 외부에서 대화 이력 주입 (Start 호출 전에 설정)
    void SetHistory(std::vector<LLMRouter::ChatTurn> history) { m_history = std::move(history); }

    // 마지막 실행에서 LLM이 만든 설명 (시각화 hover/채팅용)
    const std::wstring& LastDescription() const { return m_lastDescription; }
    // LLM 호출 실패 사유 (비어있으면 정상). 채워져 있으면 호출자가 에러 다이얼로그 표시.
    const std::wstring& LastError() const { return m_lastError; }

private:
    void Run(std::wstring question, ProgressFn onProgress, DoneFn onDone);

    DataSource&                m_ds;
    Dashboard&                 m_dash;
    std::shared_ptr<LLMRouter> m_router;
    ChainOfThinking            m_cot;
    std::thread                m_worker;
    std::atomic_bool           m_cancel{ false };
    std::vector<LLMRouter::ChatTurn> m_history;
    std::wstring                     m_lastDescription;
    std::wstring                     m_lastError;
};

} // namespace deepmetria
