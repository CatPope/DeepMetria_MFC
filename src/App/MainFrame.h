// MainFrame.h - CMainFrame 선언 (CWnd 패널 3개 포함)
#pragma once

#include "Messages.h"
#include "RibbonBar.h"
#include "DataSummaryPane.h"
#include "QueryInputDockPane.h"
#include "FormatPane.h"

class CMainFrame : public CFrameWndEx
{
protected:
    DECLARE_DYNCREATE(CMainFrame)
public:
    CMainFrame();
    virtual ~CMainFrame();

    virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    virtual void OnUpdateFrameTitle(BOOL bAddToTitle) override;
    virtual void RecalcLayout(BOOL bNotify = TRUE) override;

protected:
    deepmetria::CRibbonBar  m_wndRibbon;
    CDataSummaryPane        m_wndSummary;   // 좌측 요약 패널
    CQueryInputDockPane     m_wndQuery;     // 우측 질문 패널
    CFormatPane             m_wndFormat;    // 우측 서식 패널
    CMFCStatusBar           m_wndStatusBar;

    // 사이드바 폭 (사용자 드래그로 조절 가능)
    int m_widthSummary = 280;
    int m_widthQuery   = 340;
    int m_widthFormat  = 300;

    // 스플리터 드래그 상태
    enum class SplitMode { None, Summary, Query, Format };
    SplitMode m_splitMode = SplitMode::None;
    int       m_splitGripPx = 5;       // 그립 인식 폭

    void LayoutPanes();
    // 화면 좌표 → 어느 스플리터 위인지 hit-test
    SplitMode HitTestSplit(CPoint point) const;

    CString GetActiveSectionName() const;

    afx_msg int     OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnAnalysisProgress(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSummaryReady(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnVisualizationReady(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnLoadSample(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnRibbonTabChanged(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnQuerySubmit(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnVizFormatApplied(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnVizSelected(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnVizDelete(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnExportRequest(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnLlmError(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnAppSettings();
    afx_msg void    OnFileExport();
    afx_msg void    OnFileNew();    // [새로 만들기] — 기존 frame 유지, doc 비우기
    afx_msg void    OnFileOpen();   // [열기] — 기존 frame 유지, doc 교체

    // 사이드바 스플리터 드래그
    afx_msg void    OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void    OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void    OnMouseMove(UINT nFlags, CPoint point);
    afx_msg BOOL    OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

    // 분석 timeout 감시 (네트워크 hang 등으로 onDone 미호출 시 강제 풀림)
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    DECLARE_MESSAGE_MAP()

    // 마지막 활성 탭 — Format 가드 거부 시 복원에 사용
    deepmetria::CRibbonBar::ActiveTab m_lastTab =
        deepmetria::CRibbonBar::ActiveTab::Summary;

public:
    // 상태바 LLM pane을 현재 라우터 설정으로 갱신
    void UpdateLlmStatus();
};
