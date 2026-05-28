#include "stdafx.h"
#include "QueryInputView.h"
#include "DashboardView.h"
#include "../Document/DeepMetriaDoc.h"

// AnalysisFlow 타입 참조 (WM_ANALYSIS_DONE LPARAM 처리에 필요)
#include "../Domain/Orchestrator/AnalysisFlow.h"

// ============================================================
// IMPLEMENT_DYNCREATE / 메시지 맵
// ============================================================
IMPLEMENT_DYNCREATE(CQueryInputView, CView)

BEGIN_MESSAGE_MAP(CQueryInputView, CView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_BTN_ANALYZE, &CQueryInputView::OnBnClickedAnalyze)
    ON_MESSAGE(WM_ANALYSIS_PROGRESS, &CQueryInputView::OnAnalysisProgress)
    ON_MESSAGE(WM_ANALYSIS_DONE,     &CQueryInputView::OnAnalysisDone)
END_MESSAGE_MAP()

// ============================================================
// 생성 / 소멸
// ============================================================
CQueryInputView::CQueryInputView()
    : m_bAnalyzing(FALSE)
    , m_pDashboardView(nullptr)
{
}

CQueryInputView::~CQueryInputView()
{
}

// ============================================================
// OnCreate — 컨트롤 수동 생성
// ============================================================
int CQueryInputView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 에디트 컨트롤 (질문 입력) — Tab 순서 1
    m_editQuery.Create(
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | WS_GROUP | ES_AUTOHSCROLL,
        CRect(0, 0, 100, 24), this, IDC_EDIT_QUERY);

    // 분석 시작 버튼 — Tab 순서 2
    m_btnAnalyze.Create(
        _T("분석"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(0, 0, 60, 24), this, IDC_BTN_ANALYZE);

    // 상태 텍스트
    m_staticStatus.Create(
        _T("질문을 입력하고 분석 버튼을 누르세요."),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        CRect(0, 0, 100, 16), this, IDC_STATIC_STATUS);

    // 프로그레스 바
    m_progressBar.Create(
        WS_CHILD | WS_VISIBLE,
        CRect(0, 0, 100, 14), this, IDC_PROGRESS_BAR);
    m_progressBar.SetRange(0, 100);
    m_progressBar.SetPos(0);

    // 폰트 설정 (기본 GUI 폰트)
    CFont* pFont = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
    m_editQuery.SetFont(pFont);
    m_btnAnalyze.SetFont(pFont);
    m_staticStatus.SetFont(pFont);

    return 0;
}

// ============================================================
// OnSize — 레이아웃 재배치
// ============================================================
void CQueryInputView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);
    LayoutControls();
}

void CQueryInputView::LayoutControls()
{
    if (!m_editQuery.GetSafeHwnd())
        return;

    CRect rc;
    GetClientRect(&rc);

    int margin = 4;
    int btnW = 60;
    int editH = 24;
    int statusH = 16;
    int progressH = 14;

    int editW = rc.Width() - btnW - margin * 3;

    // 에디트 + 버튼 (1행)
    m_editQuery.MoveWindow(margin, margin, editW, editH);
    m_btnAnalyze.MoveWindow(margin + editW + margin, margin, btnW, editH);

    // 상태 텍스트 (2행)
    int row2Y = margin + editH + margin;
    m_staticStatus.MoveWindow(margin, row2Y, rc.Width() - margin * 2, statusH);

    // 프로그레스 바 (3행)
    int row3Y = row2Y + statusH + margin;
    m_progressBar.MoveWindow(margin, row3Y, rc.Width() - margin * 2, progressH);
}

// ============================================================
// OnDraw — 배경만 그림
// ============================================================
void CQueryInputView::OnDraw(CDC* pDC)
{
    CRect rc;
    GetClientRect(&rc);
    pDC->FillSolidRect(&rc, ::GetSysColor(COLOR_BTNFACE));
}

// ============================================================
// OnUpdate — 문서 변경 시 datasource ID 동기화
// ============================================================
void CQueryInputView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    TRACE(_T("[QueryInputView] OnUpdate 호출됨\n"));

    CDeepMetriaDoc* pDoc = dynamic_cast<CDeepMetriaDoc*>(GetDocument());
    if (pDoc)
    {
        const CString& dsId = pDoc->GetDataSummary().datasourceId;
        TRACE(_T("[QueryInputView] datasourceId = '%s'\n"), (LPCTSTR)dsId);

        if (!dsId.IsEmpty() && dsId != m_datasourceId)
        {
            SetDataSourceId(dsId);
            UpdateStatusText(_T("데이터 로드 완료. 질문을 입력하세요."));
            TRACE(_T("[QueryInputView] datasourceId 설정 완료\n"));
        }
    }
    else
    {
        TRACE(_T("[QueryInputView] GetDocument() 실패\n"));
    }
}

// ============================================================
// 공개 인터페이스
// ============================================================
void CQueryInputView::SetDashboardView(CDashboardView* pDashboardView)
{
    m_pDashboardView = pDashboardView;
}

void CQueryInputView::SetDataSourceId(const CString& datasourceId)
{
    m_datasourceId = datasourceId;
}

// ============================================================
// 분석 시작 버튼
// ============================================================
void CQueryInputView::OnBnClickedAnalyze()
{
    StartAnalysis();
}

void CQueryInputView::StartAnalysis()
{
    if (m_bAnalyzing)
        return;

    CString question;
    m_editQuery.GetWindowText(question);
    question.Trim();

    if (question.IsEmpty())
    {
        AfxMessageBox(_T("질문을 입력해주세요."), MB_ICONWARNING);
        m_editQuery.SetFocus();
        return;
    }

    if (m_datasourceId.IsEmpty())
    {
        AfxMessageBox(_T("데이터 파일을 먼저 불러오세요."), MB_ICONWARNING);
        return;
    }

    SetAnalyzingState(TRUE);
    UpdateStatusText(_T("분석 중..."));

    // 문서에서 데이터 가져오기
    CDeepMetriaDoc* pDoc = dynamic_cast<CDeepMetriaDoc*>(GetDocument());
    if (!pDoc)
    {
        SetAnalyzingState(FALSE);
        UpdateStatusText(_T("문서를 찾을 수 없습니다."));
        return;
    }

    // AnalysisOrchestrator 비동기 분석 시작
    if (!m_orchestrator.AnalyzeQuestion(
            pDoc->GetDataTable(),
            pDoc->GetDataSummary(),
            question,
            GetSafeHwnd()))
    {
        SetAnalyzingState(FALSE);
        UpdateStatusText(_T("분석 시작에 실패했습니다."));
    }
}

// ============================================================
// Enter 키로 분석 시작
// ============================================================
BOOL CQueryInputView::PreTranslateMessage(MSG* pMsg)
{
    // Ctrl+Enter로 분석 시작
    if (pMsg->message == WM_KEYDOWN &&
        pMsg->wParam  == VK_RETURN  &&
        (::GetKeyState(VK_CONTROL) & 0x8000))
    {
        StartAnalysis();
        return TRUE;
    }

    // Tab 키로 컨트롤 간 포커스 이동 (CView는 다이얼로그 루프가 없으므로 수동 처리)
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB)
    {
        if (::IsDialogMessage(GetSafeHwnd(), pMsg))
            return TRUE;
    }

    return CView::PreTranslateMessage(pMsg);
}

// ============================================================
// 커스텀 메시지 핸들러
// ============================================================
LRESULT CQueryInputView::OnAnalysisProgress(WPARAM wParam, LPARAM lParam)
{
    int step     = (int)wParam;
    int progress = (step > 0 && step <= 4) ? step * 25 : (int)wParam;
    m_progressBar.SetPos(progress);

    if (lParam != 0)
    {
        CString* pMsg = reinterpret_cast<CString*>(lParam);
        UpdateStatusText(*pMsg);
        delete pMsg;
        pMsg = nullptr;
    }
    return 0;
}

LRESULT CQueryInputView::OnAnalysisDone(WPARAM wParam, LPARAM lParam)
{
    SetAnalyzingState(FALSE);

    AnalysisFlow* pFlow = reinterpret_cast<AnalysisFlow*>(lParam);

    if (pFlow == nullptr)
    {
        m_progressBar.SetPos(0);
        UpdateStatusText(_T("분석이 취소되었습니다."));
        return 0;
    }

    m_progressBar.SetPos(100);

    if (pFlow->state == AnalysisFlowState::Done)
    {
        UpdateStatusText(_T("분석 완료."));
        if (m_pDashboardView && ::IsWindow(m_pDashboardView->GetSafeHwnd()))
        {
            m_pDashboardView->SendMessage(WM_VISUALIZATION_READY, wParam,
                                          reinterpret_cast<LPARAM>(pFlow));
            pFlow = nullptr;
        }
        else
        {
            delete pFlow;
            pFlow = nullptr;
        }
    }
    else
    {
        CString errMsg = _T("분석 중 오류가 발생했습니다.");
        if (!pFlow->lastError.message.IsEmpty())
            errMsg += _T("\n") + pFlow->lastError.message;

        TRACE(_T("[QueryInputView] 분석 오류: %s - %s\n"),
              (LPCTSTR)pFlow->lastError.code, (LPCTSTR)pFlow->lastError.message);
        UpdateStatusText(_T("분석 중 오류가 발생했습니다."));
        AfxMessageBox(errMsg, MB_ICONERROR);

        delete pFlow;
        pFlow = nullptr;
    }

    return 0;
}

// ============================================================
// 헬퍼
// ============================================================
void CQueryInputView::SetAnalyzingState(BOOL bAnalyzing)
{
    m_bAnalyzing = bAnalyzing;
    m_btnAnalyze.EnableWindow(!bAnalyzing);
    m_editQuery.SetReadOnly(bAnalyzing);
    if (!bAnalyzing)
        m_progressBar.SetPos(0);
}

void CQueryInputView::UpdateStatusText(const CString& text)
{
    m_staticStatus.SetWindowText(text);
}
