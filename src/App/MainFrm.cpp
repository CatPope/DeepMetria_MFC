// MainFrm.cpp — CMainFrame 구현
// 툴바, 상태바, CSplitterWnd 3분할 레이아웃 생성 및 커스텀 메시지 처리

#include "stdafx.h"
#include "MainFrm.h"
#include "../Resources/resource.h"
#include "../View/DataSummaryView.h"
#include "../View/DashboardView.h"
#include "../View/QueryInputView.h"

// ============================================================
// IMPLEMENT_DYNCREATE
// ============================================================
IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

// ============================================================
// 상태바 인디케이터 배열
// ============================================================
static UINT indicators[] =
{
    ID_SEPARATOR,           // 상태 텍스트 (왼쪽)
    ID_INDICATOR_STATUS,    // 상태 메시지
    ID_INDICATOR_PROGRESS,  // 진행률 표시
};

// ============================================================
// 메시지 맵
// ============================================================
BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    // 커스텀 메시지 핸들러
    ON_MESSAGE(WM_ANALYSIS_PROGRESS,   &CMainFrame::OnAnalysisProgress)
    ON_MESSAGE(WM_ANALYSIS_DONE,       &CMainFrame::OnAnalysisDone)
    ON_MESSAGE(WM_VISUALIZATION_READY, &CMainFrame::OnVisualizationReady)
    ON_MESSAGE(WM_DATA_LOADED,         &CMainFrame::OnDataLoaded)
    ON_MESSAGE(WM_LLM_RESPONSE,        &CMainFrame::OnLlmResponse)
    ON_MESSAGE(WM_EXPORT_COMPLETE,     &CMainFrame::OnExportComplete)
END_MESSAGE_MAP()

// ============================================================
// 생성자
// ============================================================
CMainFrame::CMainFrame()
{
}

// ============================================================
// PreCreateWindow — 윈도우 스타일 사전 설정
// ============================================================
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWnd::PreCreateWindow(cs))
        return FALSE;

    // 기본 크기 설정 (1280 x 800)
    cs.cx = 1280;
    cs.cy = 800;

    return TRUE;
}

// ============================================================
// OnCreate — 툴바 / 상태바 생성
// ============================================================
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // ---- 툴바 생성 ----
    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT,
            WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
        !m_wndToolBar.LoadToolBar(IDR_TOOLBAR))
    {
        TRACE0("툴바 생성 실패\n");
        return -1;
    }

    m_wndToolBar.SetWindowText(_T("툴바"));

    // ---- 상태바 생성 ----
    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT)))
    {
        TRACE0("상태바 생성 실패\n");
        return -1;
    }

    // 초기 상태 텍스트
    m_wndStatusBar.SetPaneText(0, _T("준비"));

    // 툴바 도킹 활성화
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);

    return 0;
}

// ============================================================
// OnCreateClient — CSplitterWnd 3분할 레이아웃 생성
//
// 레이아웃:
//   좌측 (1열): DataSummaryView
//   우상단 (2열 1행): DashboardView
//   우하단 (2열 2행): QueryInputView
//
//   [DataSummaryView | DashboardView  ]
//   [               | QueryInputView  ]
// ============================================================
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    // 외부 Splitter: 1행 2열 (좌/우 분할)
    if (!m_wndSplitter.CreateStatic(this, 1, 2))
    {
        TRACE0("외부 Splitter 생성 실패\n");
        return FALSE;
    }

    // 좌측 패널: DataSummaryView (열 0)
    if (!m_wndSplitter.CreateView(0, 0,
            RUNTIME_CLASS(CDataSummaryView), CSize(280, 600), pContext))
    {
        TRACE0("DataSummaryView 생성 실패\n");
        return FALSE;
    }

    // 우측 내부 Splitter: 2행 1열 (상/하 분할)
    CSplitterWnd* pRightSplitter = new CSplitterWnd();
    if (!pRightSplitter->CreateStatic(&m_wndSplitter, 2, 1,
            WS_CHILD | WS_VISIBLE,
            m_wndSplitter.IdFromRowCol(0, 1)))
    {
        TRACE0("우측 Splitter 생성 실패\n");
        delete pRightSplitter;
        return FALSE;
    }

    // 우상단: DashboardView (행 0)
    if (!pRightSplitter->CreateView(0, 0,
            RUNTIME_CLASS(CDashboardView), CSize(900, 450), pContext))
    {
        TRACE0("DashboardView 생성 실패\n");
        return FALSE;
    }

    // 우하단: QueryInputView (행 1)
    if (!pRightSplitter->CreateView(1, 0,
            RUNTIME_CLASS(CQueryInputView), CSize(900, 150), pContext))
    {
        TRACE0("QueryInputView 생성 실패\n");
        return FALSE;
    }

    // 초기 열 너비 설정
    m_wndSplitter.SetColumnInfo(0, 280, 100);
    m_wndSplitter.SetColumnInfo(1, 900, 200);
    m_wndSplitter.RecalcLayout();

    return TRUE;
}

// ============================================================
// 상태바 유틸리티
// ============================================================
void CMainFrame::SetStatusText(const CString& text)
{
    m_wndStatusBar.SetPaneText(0, text);
}

void CMainFrame::SetProgressValue(int percent)
{
    if (percent < 0)
    {
        // 진행률 숨김
        m_wndStatusBar.SetPaneText(2, _T(""));
    }
    else
    {
        CString progressText;
        progressText.Format(_T("%d%%"), percent);
        m_wndStatusBar.SetPaneText(2, progressText);
    }
}

// ============================================================
// 커스텀 메시지 핸들러
// ============================================================

// WM_ANALYSIS_PROGRESS: WPARAM = 진행률(0~100), LPARAM = CString* 단계 메시지
LRESULT CMainFrame::OnAnalysisProgress(WPARAM wParam, LPARAM lParam)
{
    int percent = static_cast<int>(wParam);
    SetProgressValue(percent);

    if (lParam != 0)
    {
        CString* pMsg = reinterpret_cast<CString*>(lParam);
        SetStatusText(*pMsg);
        // 수신 측에서 메모리 해제하지 않음 (발신 측 책임)
    }
    return 0;
}

// WM_ANALYSIS_DONE: 분석 완료
LRESULT CMainFrame::OnAnalysisDone(WPARAM wParam, LPARAM lParam)
{
    SetProgressValue(-1);
    SetStatusText(_T("분석 완료"));
    // 연결된 뷰에 갱신 요청 (DashboardView 등)
    SendMessageToDescendants(WM_ANALYSIS_DONE, wParam, lParam, FALSE);
    return 0;
}

// WM_VISUALIZATION_READY: 시각화 카드 준비 완료
LRESULT CMainFrame::OnVisualizationReady(WPARAM wParam, LPARAM lParam)
{
    // DashboardView에 전달하여 차트 패널 렌더링 요청
    SendMessageToDescendants(WM_VISUALIZATION_READY, wParam, lParam, FALSE);
    return 0;
}

// WM_DATA_LOADED: 파일 로드 완료
LRESULT CMainFrame::OnDataLoaded(WPARAM wParam, LPARAM lParam)
{
    SetStatusText(_T("데이터 로드 완료"));
    SetProgressValue(-1);
    // DataSummaryView에 전달
    SendMessageToDescendants(WM_DATA_LOADED, wParam, lParam, FALSE);
    return 0;
}

// WM_LLM_RESPONSE: LLM 응답 수신
LRESULT CMainFrame::OnLlmResponse(WPARAM wParam, LPARAM lParam)
{
    SetStatusText(_T("AI 응답 수신"));
    SendMessageToDescendants(WM_LLM_RESPONSE, wParam, lParam, FALSE);
    return 0;
}

// WM_EXPORT_COMPLETE: 내보내기 완료
LRESULT CMainFrame::OnExportComplete(WPARAM wParam, LPARAM lParam)
{
    BOOL bSuccess = static_cast<BOOL>(wParam);
    if (bSuccess)
        SetStatusText(_T("내보내기 완료"));
    else
        SetStatusText(_T("내보내기 실패"));
    return 0;
}
