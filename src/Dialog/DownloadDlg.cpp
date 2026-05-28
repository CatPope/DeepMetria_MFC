#include "stdafx.h"
#include "DownloadDlg.h"

// ============================================================
// IMPLEMENT_DYNAMIC / 메시지 맵
// ============================================================
IMPLEMENT_DYNAMIC(CDownloadDlg, CDialog)

BEGIN_MESSAGE_MAP(CDownloadDlg, CDialog)
    ON_BN_CLICKED(IDCANCEL, &CDownloadDlg::OnBtnCancel)
END_MESSAGE_MAP()

// ============================================================
// 생성 / 소멸
// ============================================================
CDownloadDlg::CDownloadDlg(CWnd* pParent)
    : CDialog(CDownloadDlg::IDD, pParent)
    , m_bCancelled(FALSE)
    , m_nProgress(0)
{
}

CDownloadDlg::~CDownloadDlg()
{
}

// ============================================================
// DDX
// ============================================================
void CDownloadDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROGRESS_BAR,    m_progressCtrl);
    DDX_Control(pDX, IDC_STATIC_STATUS,   m_staticStatus);
}

// ============================================================
// 초기화
// ============================================================
BOOL CDownloadDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_progressCtrl.SetRange(0, 100);
    m_progressCtrl.SetPos(0);
    m_staticStatus.SetWindowText(_T("준비 중..."));
    m_bCancelled = FALSE;

    return TRUE;
}

// ============================================================
// SetProgress — 진행률 업데이트 (0~100)
// ============================================================
void CDownloadDlg::SetProgress(int percent)
{
    m_nProgress = max(0, min(percent, 100));
    if (::IsWindow(m_progressCtrl.GetSafeHwnd()))
        m_progressCtrl.SetPos(m_nProgress);
}

// ============================================================
// SetStatusMessage — 상태 메시지 업데이트
// ============================================================
void CDownloadDlg::SetStatusMessage(const CString& msg)
{
    if (::IsWindow(m_staticStatus.GetSafeHwnd()))
        m_staticStatus.SetWindowText(msg);
}

// ============================================================
// StartDownload — 다운로드 시작 (UI 프레임워크만 제공)
// ============================================================
BOOL CDownloadDlg::StartDownload(const CString& url, const CString& destPath)
{
    (void)url;
    (void)destPath;

    m_bCancelled = FALSE;
    SetProgress(0);
    SetStatusMessage(_T("다운로드 중..."));

    return TRUE;
}

// ============================================================
// OnBtnCancel — 취소 버튼
// ============================================================
void CDownloadDlg::OnBtnCancel()
{
    m_bCancelled = TRUE;
    CDialog::OnCancel();
}
