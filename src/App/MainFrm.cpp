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
    ON_WM_DROPFILES()
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
            WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        // 툴바 리소스가 없으면 기본 버튼으로 생성
        if (m_wndToolBar.GetSafeHwnd() == nullptr)
        {
            m_wndToolBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS);
        }

        // 프로그래밍 방식으로 기본 버튼 추가
        m_wndToolBar.SetButtons(nullptr, 3);
        m_wndToolBar.SetButtonInfo(0, ID_FILE_OPEN_DATA, TBBS_BUTTON, 0);
        m_wndToolBar.SetButtonInfo(1, ID_SEPARATOR,    TBBS_SEPARATOR, 6);
        m_wndToolBar.SetButtonInfo(2, ID_APP_ABOUT,    TBBS_BUTTON, 1);

        // 버튼 텍스트 표시 (아이콘 리소스가 없는 경우)
        m_wndToolBar.SetButtonText(0, _T("열기"));
        m_wndToolBar.SetButtonText(2, _T("정보"));

        // 텍스트 표시를 위해 크기 조정
        m_wndToolBar.SetSizes(CSize(50, 36), CSize(16, 15));
    }

    // ---- 상태바 생성 ----
    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT)))
    {
        TRACE0("상태바 생성 실패\n");
        return -1;
    }

    // 초기 상태 텍스트
    m_wndStatusBar.SetPaneText(0, _T("준비"));

    DragAcceptFiles(TRUE);

    return 0;
}

// ============================================================
// OnCreateClient — CSplitterWnd 3분할 레이아웃 생성
//
// 레이아웃:
//   좌측 (1열): QueryInputView (요청 창)
//   우상단 (2열 1행): DataSummaryView (데이터 요약)
//   우하단 (2열 2행): DashboardView (차트)
//
//   [QueryInputView | DataSummaryView ]
//   [               | DashboardView   ]
// ============================================================
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    // 외부 Splitter: 1행 2열 (좌/우 분할)
    if (!m_wndSplitter.CreateStatic(this, 1, 2))
    {
        TRACE0("외부 Splitter 생성 실패\n");
        return FALSE;
    }

    // 좌측 패널: QueryInputView (열 0) — 요청 입력 창
    if (!m_wndSplitter.CreateView(0, 0,
            RUNTIME_CLASS(CQueryInputView), CSize(350, 600), pContext))
    {
        TRACE0("QueryInputView 생성 실패\n");
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

    // 우상단: DataSummaryView (행 0) — 데이터 요약
    if (!pRightSplitter->CreateView(0, 0,
            RUNTIME_CLASS(CDataSummaryView), CSize(830, 300), pContext))
    {
        TRACE0("DataSummaryView 생성 실패\n");
        return FALSE;
    }

    // 우하단: DashboardView (행 1) — 차트
    if (!pRightSplitter->CreateView(1, 0,
            RUNTIME_CLASS(CDashboardView), CSize(830, 300), pContext))
    {
        TRACE0("DashboardView 생성 실패\n");
        return FALSE;
    }

    // 초기 열 너비 설정
    m_wndSplitter.SetColumnInfo(0, 350, 150);
    m_wndSplitter.SetColumnInfo(1, 830, 200);
    m_wndSplitter.RecalcLayout();

    // 뷰 간 연결: QueryInputView → DashboardView
    CQueryInputView* pQueryInput = dynamic_cast<CQueryInputView*>(
        m_wndSplitter.GetPane(0, 0));
    CDashboardView* pDashboard = dynamic_cast<CDashboardView*>(
        pRightSplitter->GetPane(1, 0));
    if (pQueryInput && pDashboard)
        pQueryInput->SetDashboardView(pDashboard);

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

// WM_DROPFILES: 드래그앤드롭 파일 열기
void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
    UINT fileCount = DragQueryFile(hDropInfo, 0xFFFFFFFF, nullptr, 0);
    if (fileCount == 0)
    {
        DragFinish(hDropInfo);
        return;
    }

    // 첫 번째 파일만 처리
    TCHAR filePath[MAX_PATH] = {};
    DragQueryFile(hDropInfo, 0, filePath, MAX_PATH);
    DragFinish(hDropInfo);

    // 확장자 확인
    CString path(filePath);
    int dotPos = path.ReverseFind(_T('.'));
    if (dotPos < 0)
    {
        AfxMessageBox(_T("지원하지 않는 파일 형식입니다.\nCSV, Excel, JSON 파일만 드래그할 수 있습니다."),
                      MB_ICONWARNING);
        return;
    }
    CString ext = path.Mid(dotPos + 1);
    ext.MakeLower();

    if (ext != _T("csv") && ext != _T("xlsx") && ext != _T("xls") && ext != _T("json"))
    {
        AfxMessageBox(_T("지원하지 않는 파일 형식입니다.\nCSV, Excel, JSON 파일만 드래그할 수 있습니다."),
                      MB_ICONWARNING);
        return;
    }

    // Document에 파일 로드 요청
    CDocument* pDoc = GetActiveDocument();
    if (pDoc)
    {
        pDoc->OnOpenDocument(filePath);
        SetStatusText(_T("파일 드래그 로드 완료"));
    }
}
