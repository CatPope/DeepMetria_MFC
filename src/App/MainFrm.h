#pragma once

// MainFrm.h — CMainFrame 선언
// 툴바, 상태바, CSplitterWnd 3분할 레이아웃 담당

#include "../Common/Types.h"

// ============================================================
// CMainFrame 클래스 선언
// ============================================================
class CMainFrame : public CFrameWnd
{
    DECLARE_DYNCREATE(CMainFrame)

protected:
    CMainFrame();

public:
    virtual ~CMainFrame() = default;

    // ---- CFrameWnd 오버라이드 ----
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;

    // ---- 상태바 텍스트 업데이트 ----
    void SetStatusText(const CString& text);
    void SetProgressValue(int percent); // 0~100, -1 이면 숨김

protected:
    // ---- MFC 컨트롤 ----
    CToolBar    m_wndToolBar;   // 툴바 (파일 열기, 설정, 내보내기)
    CStatusBar  m_wndStatusBar; // 상태바 (상태 메시지, 진행률)
    CSplitterWnd m_wndSplitter; // 3분할 Splitter

    // ---- 메시지 핸들러 ----
    afx_msg int     OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnAnalysisProgress(WPARAM wParam, LPARAM lParam);   // WM_ANALYSIS_PROGRESS
    afx_msg LRESULT OnAnalysisDone(WPARAM wParam, LPARAM lParam);       // WM_ANALYSIS_DONE
    afx_msg LRESULT OnVisualizationReady(WPARAM wParam, LPARAM lParam); // WM_VISUALIZATION_READY
    afx_msg LRESULT OnDataLoaded(WPARAM wParam, LPARAM lParam);         // WM_DATA_LOADED
    afx_msg LRESULT OnLlmResponse(WPARAM wParam, LPARAM lParam);        // WM_LLM_RESPONSE
    afx_msg LRESULT OnExportComplete(WPARAM wParam, LPARAM lParam);     // WM_EXPORT_COMPLETE
    afx_msg void OnDropFiles(HDROP hDropInfo);

    DECLARE_MESSAGE_MAP()
};
