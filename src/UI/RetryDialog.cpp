// RetryDialog.cpp - LLM 오류/재시도 모달 구현
#include "stdafx.h"
#include "RetryDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CRetryDialog, CDialogEx)

BEGIN_MESSAGE_MAP(CRetryDialog, CDialogEx)
    ON_BN_CLICKED(IDC_RETRY_RETRY,  &CRetryDialog::OnBtnRetry)
    ON_BN_CLICKED(IDC_RETRY_SWITCH, &CRetryDialog::OnBtnSwitch)
END_MESSAGE_MAP()

CRetryDialog::CRetryDialog(LPCTSTR errorMsg, CWnd* pParent)
    : CDialogEx(IDD_RETRY, pParent)
    , m_errorMsg(errorMsg ? errorMsg : _T(""))
{
}

CRetryDialog::~CRetryDialog() = default;

void CRetryDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_RETRY_RETRY,  m_btnRetry);
    DDX_Control(pDX, IDC_RETRY_SWITCH, m_btnSwitch);
}

BOOL CRetryDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 추가 에러 메시지가 있으면 정적 텍스트에 반영
    if (!m_errorMsg.IsEmpty())
    {
        // 다이얼로그 제목에 에러 요약 표시
        CString title;
        GetWindowText(title);
        title += _T(" — ") + m_errorMsg;
        SetWindowText(title);
    }

    return TRUE;
}

void CRetryDialog::OnBtnRetry()
{
    m_result = 1;
    CDialogEx::OnOK();
}

void CRetryDialog::OnBtnSwitch()
{
    m_result = 2;
    CDialogEx::OnOK();
}
