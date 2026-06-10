// RetryDialog.h - LLM 오류/재시도 모달 다이얼로그
#pragma once
#include "resource.h"

class CRetryDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CRetryDialog)
public:
    explicit CRetryDialog(LPCTSTR errorMsg = nullptr, CWnd* pParent = nullptr);
    virtual ~CRetryDialog();

    enum { IDD = IDD_RETRY };

    // 결과: 0=취소, 1=재시도, 2=GPT전환
    int GetResult() const { return m_result; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    afx_msg void OnBtnRetry();
    afx_msg void OnBtnSwitch();
    DECLARE_MESSAGE_MAP()

private:
    CString m_errorMsg;
    int     m_result = 0;

    CButton m_btnRetry;
    CButton m_btnSwitch;
};
