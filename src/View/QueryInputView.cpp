#include "stdafx.h"
#include "QueryInputView.h"
#include "DashboardView.h"

// AnalysisFlow 타입 참조 (WM_ANALYSIS_DONE LPARAM 처리에 필요)
#include "../Domain/Orchestrator/AnalysisFlow.h"

// ============================================================
// IMPLEMENT_DYNCREATE / 메시지 맵
// ============================================================
IMPLEMENT_DYNCREATE(CQueryInputView, CFormView)

BEGIN_MESSAGE_MAP(CQueryInputView, CFormView)
    ON_BN_CLICKED(IDC_BTN_ANALYZE, &CQueryInputView::OnBnClickedAnalyze)
    ON_MESSAGE(WM_ANALYSIS_PROGRESS, &CQueryInputView::OnAnalysisProgress)
    ON_MESSAGE(WM_ANALYSIS_DONE,     &CQueryInputView::OnAnalysisDone)
END_MESSAGE_MAP()

// ============================================================
// 생성 / 소멸
// ============================================================
CQueryInputView::CQueryInputView()
    : CFormView(CQueryInputView::IDD)
    , m_bAnalyzing(FALSE)
    , m_pDashboardView(nullptr)
{
}

CQueryInputView::~CQueryInputView()
{
}

// ============================================================
// DDX
// ============================================================
void CQueryInputView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_QUERY,    m_editQuery);
    DDX_Control(pDX, IDC_BTN_ANALYZE,   m_btnAnalyze);
    DDX_Control(pDX, IDC_STATIC_STATUS, m_staticStatus);
    DDX_Control(pDX, IDC_PROGRESS_BAR,  m_progressBar);
}

// ============================================================
// 초기화
// ============================================================
void CQueryInputView::OnInitialUpdate()
{
    CFormView::OnInitialUpdate();

    // 프로그레스 바 범위 설정
    m_progressBar.SetRange(0, 100);
    m_progressBar.SetPos(0);

    // 상태 초기화
    UpdateStatusText(_T("질문을 입력하고 분석 시작 버튼을 누르세요."));
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

    // AnalysisOrchestrator 호출 (구현 완료 후 활성화)
    // 워커 스레드에서 PostMessage(WM_ANALYSIS_PROGRESS / WM_ANALYSIS_DONE) 전송
    // AnalysisOrchestrator::GetInstance().StartAsync(
    //     m_datasourceId, question, GetSafeHwnd());
}

// ============================================================
// Enter 키로 분석 시작
// ============================================================
BOOL CQueryInputView::PreTranslateMessage(MSG* pMsg)
{
    // Ctrl+Enter 또는 단순 Enter (멀티라인 에디트 내에서는 Ctrl+Enter)
    if (pMsg->message == WM_KEYDOWN &&
        pMsg->wParam  == VK_RETURN  &&
        (::GetKeyState(VK_CONTROL) & 0x8000))
    {
        StartAnalysis();
        return TRUE;
    }
    return CFormView::PreTranslateMessage(pMsg);
}

// ============================================================
// 커스텀 메시지 핸들러
// ============================================================
// WM_ANALYSIS_PROGRESS: WPARAM = 단계 번호(1-4), LPARAM = 단계 메시지 CString* 또는 0
// 메모리 소유권: AnalysisOrchestrator(송신 측)에서 new, 이 핸들러(수신 측)에서 delete.
// MainFrm 등 다른 곳에서는 절대 delete 하지 않는다 — 이중 해제 방지.
LRESULT CQueryInputView::OnAnalysisProgress(WPARAM wParam, LPARAM lParam)
{
    // WPARAM(단계 1-4)을 진행률 %로 환산 (1→25, 2→50, 3→75, 4→100)
    int step     = (int)wParam;
    int progress = (step > 0 && step <= 4) ? step * 25 : (int)wParam;
    m_progressBar.SetPos(progress);

    if (lParam != 0)
    {
        // 소유권 이전: AnalysisOrchestrator가 new, 여기서 delete — 유일한 해제 지점
        CString* pMsg = reinterpret_cast<CString*>(lParam);
        UpdateStatusText(*pMsg);
        delete pMsg;  // 이 핸들러만 해제 — 다른 핸들러에서 중복 해제 금지
        pMsg = nullptr;
    }
    return 0;
}

// WM_ANALYSIS_DONE: WPARAM = 0(취소/오류) 또는 1(성공),
//                  LPARAM = heap 할당된 AnalysisFlow* 또는 0(취소 시)
// 메모리 소유권: AnalysisOrchestrator(송신 측)에서 new, 이 핸들러(수신 측)에서 delete.
// MainFrm 등 다른 곳에서는 절대 delete 하지 않는다 — 이중 해제 방지.
LRESULT CQueryInputView::OnAnalysisDone(WPARAM wParam, LPARAM lParam)
{
    SetAnalyzingState(FALSE);

    // 소유권 이전: AnalysisOrchestrator가 new, 여기서 delete — 유일한 해제 지점
    AnalysisFlow* pFlow = reinterpret_cast<AnalysisFlow*>(lParam);

    if (pFlow == nullptr)
    {
        // lParam == 0: 취소 또는 오류로 인한 조기 종료
        m_progressBar.SetPos(0);
        UpdateStatusText(_T("분석이 취소되었습니다."));
        return 0;
    }

    m_progressBar.SetPos(100);

    if (pFlow->state == AnalysisFlowState::Done)
    {
        UpdateStatusText(_T("분석 완료."));
        // DashboardView에 시각화 추가 요청 (pFlow 소유권을 DashboardView로 이전)
        if (m_pDashboardView && ::IsWindow(m_pDashboardView->GetSafeHwnd()))
        {
            // SendMessage: DashboardView가 pFlow를 처리하고 delete 책임을 가짐
            m_pDashboardView->SendMessage(WM_VISUALIZATION_READY, wParam,
                                          reinterpret_cast<LPARAM>(pFlow));
            pFlow = nullptr;  // 소유권 이전 완료 — 이 핸들러에서 delete 하지 않음
        }
        else
        {
            // DashboardView 없음 — 여기서 해제
            delete pFlow;
            pFlow = nullptr;
        }
    }
    else
    {
        // 오류 상태: 로그 후 해제
        UpdateStatusText(_T("분석 중 오류가 발생했습니다."));
        delete pFlow;  // 오류 시 이 핸들러에서 해제
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
