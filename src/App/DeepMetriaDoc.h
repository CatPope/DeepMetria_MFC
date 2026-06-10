// DeepMetriaDoc.h - CDocument 파생. DataSource / Dashboard 보유.
#pragma once

#include "DataSource.h"
#include "Dashboard.h"
#include "AnalysisFlow.h"
#include <vector>
#include <string>

class CDeepMetriaDoc : public CDocument
{
protected:
    DECLARE_DYNCREATE(CDeepMetriaDoc)
    CDeepMetriaDoc();

public:
    virtual BOOL OnNewDocument() override;
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    virtual void Serialize(CArchive& ar) override;

    deepmetria::DataSource& GetDataSource() { return m_dataSource; }
    deepmetria::Dashboard&  GetDashboard()  { return m_dashboard; }

    // 현재 진행/완료된 분석의 CoT (없으면 nullptr)
    const deepmetria::ChainOfThinking* GetCurrentReasoning() const
    {
        return m_currentFlow ? &m_currentFlow->Reasoning() : nullptr;
    }

    // 분석 진행 중 여부 — UI가 추가 입력을 차단할 때 사용
    bool IsAnalysisRunning() const { return m_busy; }

    // 타임아웃 등으로 강제 풀림 (워커 콜백 누락 안전망)
    void ResetBusy() { m_busy = false; }

    // 대화 이력 — LLM 컨텍스트 + UI 채팅 양쪽에서 사용
    struct ChatTurn { bool isUser; std::wstring text; };
    const std::vector<ChatTurn>& ChatHistory() const { return m_chatHistory; }
    void AppendChatTurn(bool isUser, const std::wstring& text)
    {
        m_chatHistory.push_back({ isUser, text });
        if (m_chatHistory.size() > 50)  // 너무 길면 오래된 것부터 비움
            m_chatHistory.erase(m_chatHistory.begin());
    }
    void ClearChatHistory() { m_chatHistory.clear(); }

    // 자연어 질문 → AnalysisFlow 시작 (비동기)
    // LLM 미설정 시 false 반환 + 호출자가 사용자에게 안내
    bool SubmitNaturalLanguageQuery(const CStringW& question);

protected:
    deepmetria::DataSource  m_dataSource;
    deepmetria::Dashboard   m_dashboard;
    std::shared_ptr<deepmetria::AnalysisFlow> m_currentFlow;
    bool                                      m_busy = false;
    std::vector<ChatTurn>                     m_chatHistory;

    afx_msg void OnAiSummarize();
    afx_msg void OnAiAnalyze();
    afx_msg void OnExportPng();
    DECLARE_MESSAGE_MAP()

public:
    virtual ~CDeepMetriaDoc();
};
