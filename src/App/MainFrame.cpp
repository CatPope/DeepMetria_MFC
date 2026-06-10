// MainFrame.cpp - CFrameWndEx 기반 메인 프레임 윈도우
#include "stdafx.h"
#include <algorithm>
#include "DeepMetria.h"
#include "MainFrame.h"
#include "DeepMetriaDoc.h"
#include "DeepMetriaView.h"
#include "SettingsDialog.h"
#include "ExportDialog.h"
#include "LLMRouter.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_COMMAND(ID_APP_SETTINGS, &CMainFrame::OnAppSettings)
    ON_COMMAND(ID_FILE_EXPORT,  &CMainFrame::OnFileExport)
    ON_COMMAND(ID_FILE_NEW,     &CMainFrame::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN,    &CMainFrame::OnFileOpen)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_WM_TIMER()
    ON_MESSAGE(deepmetria::WM_USER_ANALYSIS_PROGRESS,   &CMainFrame::OnAnalysisProgress)
    ON_MESSAGE(deepmetria::WM_USER_SUMMARY_READY,       &CMainFrame::OnSummaryReady)
    ON_MESSAGE(deepmetria::WM_USER_VISUALIZATION_READY, &CMainFrame::OnVisualizationReady)
    ON_MESSAGE(deepmetria::WM_USER_LOAD_SAMPLE,         &CMainFrame::OnLoadSample)
    ON_MESSAGE(deepmetria::WM_USER_RIBBON_TAB_CHANGED,  &CMainFrame::OnRibbonTabChanged)
    ON_MESSAGE(deepmetria::WM_USER_QUERY_SUBMIT,        &CMainFrame::OnQuerySubmit)
    ON_MESSAGE(deepmetria::WM_USER_VIZ_FORMAT_APPLIED,  &CMainFrame::OnVizFormatApplied)
    ON_MESSAGE(deepmetria::WM_USER_VIZ_SELECTED,        &CMainFrame::OnVizSelected)
    ON_MESSAGE(deepmetria::WM_USER_EXPORT_REQUEST,      &CMainFrame::OnExportRequest)
    ON_MESSAGE(deepmetria::WM_USER_LLM_ERROR,           &CMainFrame::OnLlmError)
    ON_MESSAGE(deepmetria::WM_USER_VIZ_DELETE,          &CMainFrame::OnVizDelete)
END_MESSAGE_MAP()

CMainFrame::CMainFrame() = default;
CMainFrame::~CMainFrame() = default;

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 리본 바 (상단 64px child)
    if (!m_wndRibbon.Create(this))
        TRACE0("리본 생성 실패\n");

    // 좌측 요약 패널 (CWnd 직접 child)
    if (!m_wndSummary.Create(this, ID_VIEW_DATA_SUMMARY))
        TRACE0("요약 패널 생성 실패\n");

    // 우측 질문 패널 (CWnd 직접 child)
    if (!m_wndQuery.Create(this, ID_VIEW_QUERY_INPUT))
        TRACE0("질문 패널 생성 실패\n");

    // 우측 서식 패널 (CWnd 직접 child)
    if (!m_wndFormat.Create(this, ID_FORMAT_VISUALIZATION))
        TRACE0("서식 패널 생성 실패\n");

    // 상태바 3-pane
    static UINT indicators[] = {
        ID_SEPARATOR,       // pane 0: 메인 상태
        ID_INDICATOR_AI,    // pane 1: 메타정보
        ID_INDICATOR_LLM,   // pane 2: LLM 상태
    };
    if (!m_wndStatusBar.Create(this))
        TRACE0("상태 표시줄 생성 실패\n");
    else
    {
        m_wndStatusBar.SetIndicators(indicators, _countof(indicators));
        m_wndStatusBar.SetPaneWidth(1, 120);
        m_wndStatusBar.SetPaneWidth(2, 180);
        m_wndStatusBar.SetPaneText(0, _T("준비됨"));
        m_wndStatusBar.SetPaneText(1, _T(""));
    }

    // 표준 Win32 메뉴 완전 제거
    SetMenu(nullptr);

    UpdateLlmStatus();
    return 0;
}

void CMainFrame::LayoutPanes()
{
    CRect rcClient;
    GetClientRect(&rcClient);

    // 상태바 높이 제외
    if (m_wndStatusBar.GetSafeHwnd())
    {
        CRect rcStatus;
        m_wndStatusBar.GetWindowRect(&rcStatus);
        rcClient.bottom -= rcStatus.Height();
    }

    const int kRibH  = deepmetria::CRibbonBar::kHeight;  // 64
    const int W_SUM  = m_widthSummary;
    const int W_Q    = m_widthQuery;
    const int W_F    = m_widthFormat;

    // 리본 바: 최상단 + z-order top
    if (m_wndRibbon.GetSafeHwnd())
        m_wndRibbon.SetWindowPos(&CWnd::wndTop,
            rcClient.left, rcClient.top,
            rcClient.Width(), kRibH,
            SWP_NOACTIVATE | SWP_SHOWWINDOW);

    int contentTop = rcClient.top + kRibH;
    int contentH   = rcClient.Height() - kRibH;
    if (contentH < 0) contentH = 0;

    using Tab = deepmetria::CRibbonBar::ActiveTab;
    bool showSum = m_wndRibbon.GetSafeHwnd() && m_wndRibbon.IsTabOpen(Tab::Summary);
    bool showQ   = m_wndRibbon.GetSafeHwnd() && m_wndRibbon.IsTabOpen(Tab::Query);
    bool showF   = m_wndRibbon.GetSafeHwnd() && m_wndRibbon.IsTabOpen(Tab::Format)
                   && m_wndRibbon.IsTabEnabled(Tab::Format);

    // 메인 영역 최소 폭 보장 (400px). 부족하면 마지막에 켠 사이드바를 축소.
    const int kMainMin = 400;
    int wSum = showSum ? m_widthSummary : 0;
    int wQ   = showQ   ? m_widthQuery   : 0;
    int wF   = showF   ? m_widthFormat  : 0;
    int totalW = rcClient.Width();
    int sidebarsW = wSum + wQ + wF;
    int overflow = sidebarsW + kMainMin - totalW;
    if (overflow > 0)
    {
        // 우측부터 축소 (Format → Query → Summary 순)
        auto shrink = [&](int& w){
            int take = std::min(overflow, std::max(0, w - 160));
            w -= take; overflow -= take;
        };
        if (showF) shrink(wF);
        if (overflow > 0 && showQ)   shrink(wQ);
        if (overflow > 0 && showSum) shrink(wSum);
    }

    int leftEdge  = rcClient.left;
    int rightEdge = rcClient.right;

    if (m_wndSummary.GetSafeHwnd())
    {
        if (showSum)
        {
            m_wndSummary.SetWindowPos(nullptr,
                rcClient.left, contentTop, wSum, contentH,
                SWP_NOZORDER | SWP_SHOWWINDOW);
            leftEdge += wSum;
        }
        else m_wndSummary.ShowWindow(SW_HIDE);
    }

    if (m_wndQuery.GetSafeHwnd())
    {
        if (showQ)
        {
            m_wndQuery.SetWindowPos(nullptr,
                rightEdge - wQ, contentTop, wQ, contentH,
                SWP_NOZORDER | SWP_SHOWWINDOW);
            rightEdge -= wQ;
        }
        else m_wndQuery.ShowWindow(SW_HIDE);
    }

    if (m_wndFormat.GetSafeHwnd())
    {
        if (showF)
        {
            m_wndFormat.SetWindowPos(nullptr,
                rightEdge - wF, contentTop, wF, contentH,
                SWP_NOZORDER | SWP_SHOWWINDOW);
            rightEdge -= wF;
        }
        else m_wndFormat.ShowWindow(SW_HIDE);
    }

    if (CView* pView = GetActiveView())
    {
        if (pView->GetSafeHwnd())
        {
            int viewW = rightEdge - leftEdge;
            if (viewW < 0) viewW = 0;
            pView->SetWindowPos(nullptr,
                leftEdge, contentTop, viewW, contentH,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            // 사이드바 토글로 View가 리사이즈되면 child window(미리보기 표)가
            // 일시적으로 사라져 보일 수 있어 즉시 강제 재그리기 (자식 포함)
            pView->RedrawWindow(nullptr, nullptr,
                RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
    }
}

void CMainFrame::RecalcLayout(BOOL bNotify)
{
    CFrameWndEx::RecalcLayout(bNotify);
    LayoutPanes();
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWndEx::PreCreateWindow(cs))
        return FALSE;
    cs.style &= ~FWS_ADDTOTITLE;
    cs.cx = 1280;
    cs.cy = 800;
    return TRUE;
}

CString CMainFrame::GetActiveSectionName() const
{
    if (!m_wndRibbon.GetSafeHwnd()) return _T("");
    switch (m_wndRibbon.GetActiveTab())
    {
    case deepmetria::CRibbonBar::ActiveTab::Summary:  return _T("요약");
    case deepmetria::CRibbonBar::ActiveTab::Query:    return _T("질문");
    case deepmetria::CRibbonBar::ActiveTab::Format:   return _T("서식");
    default: return _T("");
    }
}

void CMainFrame::OnUpdateFrameTitle(BOOL /*bAddToTitle*/)
{
    CString title = _T("DeepMetria");

    if (CDocument* pDoc = GetActiveDocument())
    {
        CString path = pDoc->GetPathName();
        if (!path.IsEmpty())
        {
            int p = path.ReverseFind(_T('\\'));
            CString fileName = (p >= 0) ? path.Mid(p + 1) : path;
            title += _T(" — ") + fileName;
        }
    }

    CString section = GetActiveSectionName();
    if (!section.IsEmpty())
        title += _T(" · ") + section;

    SetWindowText(title);
}

// --- WM_USER 핸들러 ---

LRESULT CMainFrame::OnAnalysisProgress(WPARAM wParam, LPARAM /*lParam*/)
{
    int pct = static_cast<int>(wParam);
    CString msg;
    msg.Format(_T("● 분석 중... 도구 호출 (%d%%)"), pct);
    if (m_wndStatusBar.GetSafeHwnd())
    {
        m_wndStatusBar.SetPaneText(0, msg);
        m_wndStatusBar.SetPaneText(1, _T("CoT 활성"));
    }
    // View에 진행률 전파 → CoT 카드 갱신
    if (CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView()))
        pView->SetAnalysisProgress(pct);

    // 안전망: 100% 도달 시 VISUALIZATION_READY 도착이 누락되어도 버튼 풀림 보장
    if (pct >= 100)
    {
        m_wndQuery.SetSubmitEnabled(true);
        KillTimer(0xA001);  // timeout 타이머도 정리
    }
    return 0;
}

LRESULT CMainFrame::OnSummaryReady(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (m_wndStatusBar.GetSafeHwnd())
    {
        m_wndStatusBar.SetPaneText(0, _T("● 데이터 요약 완료 — 기본 대시보드가 구성되었습니다"));

        // 행/열 수를 DataSource에서 동적으로 표시
        CString metaText;
        if (CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView()))
        {
            if (CDeepMetriaDoc* pDoc = pView->GetDocument())
            {
                const auto& ds = pDoc->GetDataSource();
                metaText.Format(_T("행 %d · 열 %d"),
                    static_cast<int>(ds.Rows().size()),
                    static_cast<int>(ds.Columns().size()));
            }
        }
        if (metaText.IsEmpty()) metaText = _T("데이터 로드됨");
        m_wndStatusBar.SetPaneText(1, metaText);
    }

    // 요약 탭 자동 전환
    if (m_wndRibbon.GetSafeHwnd())
        m_wndRibbon.SetActiveTab(deepmetria::CRibbonBar::ActiveTab::Summary);

    OnUpdateFrameTitle(TRUE);
    RecalcLayout();

    // DataSource로 패널 갱신 + 미리보기 리스트 동적 채움
    if (CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView()))
    {
        if (CDeepMetriaDoc* pDoc = pView->GetDocument())
        {
            const auto& ds = pDoc->GetDataSource();
            m_wndSummary.SetSummary(ds);
            pView->RefreshPreviewList(ds);
        }
        pView->Invalidate();
    }
    return 0;
}

LRESULT CMainFrame::OnVisualizationReady(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // 분석 응답은 Query 패널 채팅에 표시되므로 강제로 열어둠
    using Tab = deepmetria::CRibbonBar::ActiveTab;
    if (m_wndRibbon.GetSafeHwnd() && !m_wndRibbon.IsTabOpen(Tab::Query))
    {
        m_wndRibbon.SetTabOpen(Tab::Query, true);
        RecalcLayout();
    }

    // View 진행률 완료 표시
    if (CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView()))
    {
        pView->SetAnalysisProgress(100);

        // AI 응답: ChatHistory의 가장 마지막 항목이 AI 인 경우(=이번 분석에서 방금 추가됨)만 표시.
        // Doc 의 워커 콜백이 LastDescription() 을 AppendChatTurn(false, ...) 으로 누적한 직후
        // PostMessage 가 도착하므로, 정상이면 back() 이 이번 분석의 AI 응답이다.
        // 이전 분석의 AI 메시지를 다시 채팅에 push 하지 않도록 rbegin 순회 대신 back() 만 확인.
        CDeepMetriaDoc* pDoc = pView->GetDocument();
        CString aiMsg;
        if (pDoc)
        {
            const auto& hist = pDoc->ChatHistory();
            if (!hist.empty() && !hist.back().isUser)
                aiMsg = hist.back().text.c_str();
        }
        if (!aiMsg.IsEmpty())
            m_wndQuery.AddAiMessage(aiMsg);
    }

    if (m_wndStatusBar.GetSafeHwnd())
    {
        m_wndStatusBar.SetPaneText(0, _T("● 분석 완료 — 시각화 추가됨"));
        m_wndStatusBar.SetPaneText(1, _T("CoT 완료 ✓"));
    }

    // [분석 실행] 버튼 재활성화 — 다음 질문 가능
    m_wndQuery.SetSubmitEnabled(true);

    // 즉시 메인 화면 재그리기 + 새 viz 가 viewport 밖이면 자동 스크롤
    if (CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView()))
    {
        pView->UpdateScrollBars();
        // 마지막에 추가된 viz로 스크롤
        if (CDeepMetriaDoc* pDoc = pView->GetDocument())
        {
            const auto& vizList = pDoc->GetDashboard().Visualizations();
            if (!vizList.empty())
            {
                const auto& last = vizList.back();
                CRect cli; pView->GetClientRect(&cli);
                int needX = std::max(0, last.x + last.width  - cli.Width());
                int needY = std::max(0, last.y + last.height - cli.Height());
                if (pView->GetScrollPos(SB_HORZ) < needX)
                    pView->SetScrollPos(SB_HORZ, needX, TRUE);
                if (pView->GetScrollPos(SB_VERT) < needY)
                    pView->SetScrollPos(SB_VERT, needY, TRUE);
            }
        }
        pView->Invalidate();
        pView->UpdateWindow();
    }
    return 0;
}

LRESULT CMainFrame::OnRibbonTabChanged(WPARAM wParam, LPARAM /*lParam*/)
{
    using Tab = deepmetria::CRibbonBar::ActiveTab;
    Tab activeTab = static_cast<Tab>(static_cast<int>(wParam));

    m_lastTab = activeTab;
    OnUpdateFrameTitle(TRUE);
    RecalcLayout();

    if (CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView()))
        pView->SetActiveTab(activeTab);
    else if (CView* pView = GetActiveView())
        pView->Invalidate();
    return 0;
}

LRESULT CMainFrame::OnLoadSample(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (m_wndStatusBar.GetSafeHwnd())
        m_wndStatusBar.SetPaneText(0, _T("준비됨"));
    return 0;
}

LRESULT CMainFrame::OnQuerySubmit(WPARAM /*wParam*/, LPARAM lParam)
{
    if (lParam == 0) return 0;
    CStringW question(reinterpret_cast<LPCWSTR>(lParam));

    if (m_wndStatusBar.GetSafeHwnd())
        m_wndStatusBar.SetPaneText(0, _T("● 분석 요청 중..."));

    if (CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView()))
    {
        if (CDeepMetriaDoc* pDoc = pView->GetDocument())
        {
            // Doc::SubmitNaturalLanguageQuery는 진행 중이거나 LLM 미설정 시 false 반환.
            // 성공적으로 시작되었을 때만 입력 차단.
            if (pDoc->SubmitNaturalLanguageQuery(question))
            {
                m_wndQuery.SetSubmitEnabled(false);
                // 60초 후 자동 풀림 (네트워크 hang 안전망)
                SetTimer(0xA001, 60 * 1000, nullptr);
            }
        }
    }
    return 0;
}

// 분석 timeout — onDone이 누락되더라도 60초 후 강제 풀림
void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 0xA001)
    {
        KillTimer(0xA001);
        m_wndQuery.SetSubmitEnabled(true);
        // Doc 의 busy 플래그도 강제 풀림 — 안 그러면 SubmitNaturalLanguageQuery 가드가 막음
        if (CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView()))
            if (CDeepMetriaDoc* pDoc = pView->GetDocument())
                pDoc->ResetBusy();
        if (m_wndStatusBar.GetSafeHwnd())
            m_wndStatusBar.SetPaneText(0, _T("● 분석 타임아웃 — 다시 시도 가능"));
        return;
    }
    CFrameWndEx::OnTimer(nIDEvent);
}

LRESULT CMainFrame::OnVizSelected(WPARAM wParam, LPARAM /*lParam*/)
{
    int vizIndex = static_cast<int>(wParam);
    using Tab = deepmetria::CRibbonBar::ActiveTab;

    // viz 선택 해제 (-1) → 서식/내보내기 탭 비활성화 + 사이드바 닫음
    if (vizIndex < 0)
    {
        if (m_wndRibbon.GetSafeHwnd())
        {
            m_wndRibbon.SetTabEnabled(Tab::Format, false);
            m_wndRibbon.SetTabEnabled(Tab::Export, false);
        }
        RecalcLayout();
        return 0;
    }

    if (CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView()))
    {
        CDeepMetriaDoc* pDoc = pView->GetDocument();
        if (pDoc)
        {
            auto& vizList = pDoc->GetDashboard().Visualizations();
            if (vizIndex < static_cast<int>(vizList.size()))
            {
                const auto& viz = vizList[vizIndex];
                CString title(viz.title.c_str());
                m_wndFormat.SetSelectedViz(vizIndex, title);
                m_wndFormat.FillFromViz(viz);

                // viz 선택됨 → 서식 + 내보내기 활성화. 서식만 자동 펼침
                if (m_wndRibbon.GetSafeHwnd())
                {
                    m_wndRibbon.SetTabEnabled(Tab::Format, true);
                    m_wndRibbon.SetTabEnabled(Tab::Export, true);
                    m_wndRibbon.SetTabOpen(Tab::Format, true);
                }
                RecalcLayout();

                if (m_wndStatusBar.GetSafeHwnd())
                {
                    CString msg;
                    msg.Format(_T("● 선택: %s"), title.GetString());
                    m_wndStatusBar.SetPaneText(0, msg);
                }
            }
        }
    }
    return 0;
}

LRESULT CMainFrame::OnVizFormatApplied(WPARAM wParam, LPARAM /*lParam*/)
{
    int vizIndex = static_cast<int>(wParam);

    // FormatPane에서 편집한 값을 실제 Visualization에 반영
    if (CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView()))
    {
        CDeepMetriaDoc* pDoc = pView->GetDocument();
        if (pDoc)
        {
            auto& vizList = pDoc->GetDashboard().Visualizations();
            if (vizIndex >= 0 && vizIndex < static_cast<int>(vizList.size()))
                m_wndFormat.ApplyToViz(vizList[vizIndex]);
        }
    }

    if (m_wndStatusBar.GetSafeHwnd())
        m_wndStatusBar.SetPaneText(0, _T("● 서식 적용됨"));
    if (CView* pView = GetActiveView())
        pView->Invalidate();
    return 0;
}

void CMainFrame::OnAppSettings()
{
    CSettingsDialog dlg(this);
    dlg.DoModal();
    // 저장 후 라우터 재로드 결과를 상태바에 반영
    UpdateLlmStatus();
}

void CMainFrame::OnFileExport()
{
    CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView());
    if (!pView) return;
    CDeepMetriaDoc* pDoc = pView->GetDocument();
    if (!pDoc) return;

    int selectedIdx = pView->GetSelectedViz();
    CExportDialog dlg(pDoc->GetDataSource(), pDoc->GetDashboard(),
                      selectedIdx, this);
    dlg.DoModal();
}

// [새로 만들기] — 기존 frame 유지, 현재 Document만 초기화
// SDI 표준 OnFileNew를 가로채는 이유:
// InitInstance가 FileNothing으로 우회했기 때문에 SingleDocTemplate이
// m_pOnlyDoc/m_pOnlyFrame을 모름 → 표준 OnFileNew는 새 frame을 또 만듦
void CMainFrame::OnFileNew()
{
    CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView());
    if (!pView) return;
    CDeepMetriaDoc* pDoc = pView->GetDocument();
    if (!pDoc) return;

    // Document 비우기
    pDoc->OnNewDocument();
    pDoc->SetPathName(_T(""), FALSE);
    pDoc->SetModifiedFlag(FALSE);

    // 패널/View 초기 상태로
    m_wndSummary.SetSummary(pDoc->GetDataSource());
    pView->RefreshPreviewList(pDoc->GetDataSource());
    pView->SetAnalysisProgress(-1);

    OnUpdateFrameTitle(TRUE);
    pView->Invalidate();

    if (m_wndStatusBar.GetSafeHwnd())
        m_wndStatusBar.SetPaneText(0, _T("준비됨 — 새 문서"));
}

// [열기] — 기존 frame 유지, 현재 Document에 파일 로드
// SDI 표준 OnFileOpen 가로채기 — 새 창 안 뜨도록
void CMainFrame::OnFileOpen()
{
    CFileDialog dlg(TRUE, nullptr, nullptr,
        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
        _T("데이터 파일 (*.csv;*.json;*.xlsx;*.xls;*.xlsm)|*.csv;*.json;*.xlsx;*.xls;*.xlsm|")
        _T("CSV 파일 (*.csv)|*.csv|")
        _T("JSON 파일 (*.json)|*.json|")
        _T("Excel 파일 (*.xlsx;*.xls;*.xlsm)|*.xlsx;*.xls;*.xlsm|")
        _T("모든 파일 (*.*)|*.*||"));
    if (dlg.DoModal() != IDOK) return;

    CString path = dlg.GetPathName();

    CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView());
    if (!pView) return;
    CDeepMetriaDoc* pDoc = pView->GetDocument();
    if (!pDoc) return;

    if (m_wndStatusBar.GetSafeHwnd())
        m_wndStatusBar.SetPaneText(0, _T("● 파일 로드 중..."));

    // 기존 doc 재사용 — OnOpenDocument는 내부에서 Reset 후 파싱
    if (pDoc->OnOpenDocument(path))
    {
        // 자동 요약은 OnOpenDocument 내부에서 트리거되어
        // WM_USER_SUMMARY_READY → OnSummaryReady가 후속 처리
        if (m_wndStatusBar.GetSafeHwnd())
            m_wndStatusBar.SetPaneText(0, _T("● 파일 로드 완료"));
    }
    else
    {
        if (m_wndStatusBar.GetSafeHwnd())
            m_wndStatusBar.SetPaneText(0, _T("● 파일 로드 실패"));
    }
}

LRESULT CMainFrame::OnExportRequest(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // Doc::OnExportPng()에서 PostMessage로 호출
    OnFileExport();
    return 0;
}

LRESULT CMainFrame::OnVizDelete(WPARAM wParam, LPARAM /*lParam*/)
{
    int vizIdx = static_cast<int>(wParam);
    using Tab = deepmetria::CRibbonBar::ActiveTab;

    CDeepMetriaView* pView = dynamic_cast<CDeepMetriaView*>(GetActiveView());
    if (!pView) return 0;
    CDeepMetriaDoc* pDoc = pView->GetDocument();
    if (!pDoc) return 0;

    auto& vizList = pDoc->GetDashboard().Visualizations();
    if (vizIdx < 0 || vizIdx >= (int)vizList.size()) return 0;

    int id = vizList[vizIdx].id;
    pDoc->GetDashboard().Remove(id);

    // 선택 해제 + 서식/내보내기 비활성화
    if (m_wndRibbon.GetSafeHwnd())
    {
        m_wndRibbon.SetTabEnabled(Tab::Format, false);
        m_wndRibbon.SetTabEnabled(Tab::Export, false);
    }
    pView->UpdateScrollBars();
    pView->Invalidate();
    pView->UpdateWindow();
    RecalcLayout();

    if (m_wndStatusBar.GetSafeHwnd())
        m_wndStatusBar.SetPaneText(0, _T("● 시각화 삭제됨"));
    return 0;
}

LRESULT CMainFrame::OnLlmError(WPARAM /*wParam*/, LPARAM lParam)
{
    // 분석 실행 잠금 해제 + 타이머 정리 (다이얼로그 전 먼저 해야 닫혀도 안 잠김)
    m_wndQuery.SetSubmitEnabled(true);
    KillTimer(0xA001);

    CString reason = _T("(원인 불명)");
    if (lParam) reason = reinterpret_cast<LPCWSTR>(lParam);

    if (m_wndStatusBar.GetSafeHwnd())
    {
        CString sb;
        sb.Format(_T("● LLM 오류: %s"), reason.GetString());
        m_wndStatusBar.SetPaneText(0, sb);
        m_wndStatusBar.SetPaneText(1, _T("실패"));
    }

    // 명시적 에러 다이얼로그 — 폴백 시각화/채팅 추가 없음
    CString msg;
    msg.Format(
        _T("LLM 호출에 실패했습니다.\n\n%s\n\n")
        _T("[설정]에서 제공자/모델/API 키를 확인하세요.\n")
        _T("모델 ID가 잘못된 경우 ([설정] → 모델 콤보의 목록에서 선택) 가장 흔한 원인입니다."),
        reason.GetString());
    AfxMessageBox(msg, MB_OK | MB_ICONWARNING);
    return 0;
}

// 어느 스플리터 위인지 hit-test (현재 보이는 모든 사이드바의 안쪽 경계 ± m_splitGripPx)
CMainFrame::SplitMode CMainFrame::HitTestSplit(CPoint point) const
{
    using Tab = deepmetria::CRibbonBar::ActiveTab;
    if (!m_wndRibbon.GetSafeHwnd()) return SplitMode::None;

    CRect rc; GetClientRect(&rc);
    const int kRibH = deepmetria::CRibbonBar::kHeight;
    int contentTop = rc.top + kRibH;
    int contentBottom = rc.bottom;
    if (m_wndStatusBar.GetSafeHwnd())
    {
        CRect rcSt; m_wndStatusBar.GetWindowRect(&rcSt);
        contentBottom -= rcSt.Height();
    }
    if (point.y < contentTop || point.y > contentBottom) return SplitMode::None;

    int leftEdge  = rc.left;
    int rightEdge = rc.right;
    const long px = point.x;

    if (m_wndRibbon.IsTabOpen(Tab::Summary))
    {
        int x = leftEdge + m_widthSummary;
        if (std::abs((int)(px - x)) <= m_splitGripPx) return SplitMode::Summary;
    }
    if (m_wndRibbon.IsTabOpen(Tab::Query))
    {
        int x = rightEdge - m_widthQuery;
        if (std::abs((int)(px - x)) <= m_splitGripPx) return SplitMode::Query;
        rightEdge -= m_widthQuery;
    }
    if (m_wndRibbon.IsTabOpen(Tab::Format) && m_wndRibbon.IsTabEnabled(Tab::Format))
    {
        int x = rightEdge - m_widthFormat;
        if (std::abs((int)(px - x)) <= m_splitGripPx) return SplitMode::Format;
    }
    return SplitMode::None;
}

void CMainFrame::OnLButtonDown(UINT nFlags, CPoint point)
{
    SplitMode m = HitTestSplit(point);
    if (m != SplitMode::None)
    {
        m_splitMode = m;
        SetCapture();
        return;
    }
    CFrameWndEx::OnLButtonDown(nFlags, point);
}

void CMainFrame::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_splitMode != SplitMode::None)
    {
        m_splitMode = SplitMode::None;
        ReleaseCapture();
        return;
    }
    CFrameWndEx::OnLButtonUp(nFlags, point);
}

void CMainFrame::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_splitMode != SplitMode::None)
    {
        CRect rc; GetClientRect(&rc);
        const int kMin = 160, kMax = 800;
        const int px = static_cast<int>(point.x);
        switch (m_splitMode)
        {
        case SplitMode::Summary:
            m_widthSummary = std::max(kMin, std::min(px, kMax));
            break;
        case SplitMode::Query:
            m_widthQuery   = std::max(kMin, std::min((int)rc.right - px, kMax));
            break;
        case SplitMode::Format:
            m_widthFormat  = std::max(kMin, std::min((int)rc.right - px, kMax));
            break;
        default: break;
        }
        LayoutPanes();
        return;
    }
    CFrameWndEx::OnMouseMove(nFlags, point);
}

BOOL CMainFrame::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (pWnd == this && nHitTest == HTCLIENT)
    {
        CPoint pt; GetCursorPos(&pt); ScreenToClient(&pt);
        if (HitTestSplit(pt) != SplitMode::None)
        {
            ::SetCursor(::LoadCursor(nullptr, IDC_SIZEWE));
            return TRUE;
        }
    }
    return CFrameWndEx::OnSetCursor(pWnd, nHitTest, message);
}

void CMainFrame::UpdateLlmStatus()
{
    if (!m_wndStatusBar.GetSafeHwnd()) return;

    CString text = _T("LLM: (미설정)");
    auto router = theApp.m_llmRouter;
    if (router)
    {
        using P = deepmetria::LLMRouter::Provider;
        LPCTSTR provName = _T("?");
        switch (router->CurrentProvider())
        {
        case P::Claude:  provName = _T("Claude"); break;
        case P::OpenAI:  provName = _T("OpenAI"); break;
        case P::Gemini:  provName = _T("Gemini"); break;
        default:         provName = _T("미설정"); break;
        }
        const std::wstring& model = router->Model();
        if (model.empty())
            text.Format(_T("LLM: %s"), provName);
        else
            text.Format(_T("LLM: %s · %s"), provName, model.c_str());
    }
    m_wndStatusBar.SetPaneText(2, text);
}
