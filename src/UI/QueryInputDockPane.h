// QueryInputDockPane.h - 우측 자연어 질문 패널 (CWnd 직접 child)
#pragma once
#include <vector>
#include <string>

class CQueryInputDockPane : public CWnd
{
public:
    struct ChatMsg { bool isUser; std::wstring text; };

    CQueryInputDockPane();
    virtual ~CQueryInputDockPane();

    BOOL Create(CWnd* pParent, UINT id);

    // 외부에서 채팅 누적 (사용자 질문 / AI 응답)
    void AddUserMessage(LPCWSTR text);
    void AddAiMessage(LPCWSTR text);

    // 분석 진행 중일 때 [분석 실행] 버튼 비활성화
    void SetSubmitEnabled(bool enabled);

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();
    afx_msg void OnBtnSubmit();
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    DECLARE_MESSAGE_MAP()

private:
    void LayoutChildren(int cx, int cy);

    // 질문 입력
    CEdit   m_edQuery;

    // 분석 실행 버튼
    CButton m_btnSubmit;

    // 질문 입력 레이블
    CStatic m_lblQuery;

    // 채팅 영역 높이 (입력칸을 제외한 나머지)
    int m_chatAreaH = 0;

    // 채팅 영역 스크롤 — 양수면 위로 스크롤 (이전 메시지 보기)
    int m_chatScrollY = 0;

    // 총 메시지 그릴 때 필요한 픽셀 (스크롤 한계 계산용)
    int m_chatContentH = 0;

    // 실제 채팅 메시지 큐 (시간순, 무제한 누적)
    std::vector<ChatMsg> m_messages;
};
