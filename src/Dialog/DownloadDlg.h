#pragma once
// DownloadDlg.h — 파일 다운로드/내보내기 진행 대화상자
// TC-07-03, TC-07-04

// afxwin.h와 Types.h는 stdafx.h(PCH)에서 이미 포함됨
#include "../Resources/resource.h"

// ============================================================
// CDownloadDlg — 다운로드 진행 대화상자
// ============================================================
class CDownloadDlg : public CDialog {
    DECLARE_DYNAMIC(CDownloadDlg)
public:
    CDownloadDlg(CWnd* pParent = nullptr);
    virtual ~CDownloadDlg();

    enum { IDD = IDD_DOWNLOAD_DLG };

    // 다운로드 진행률 설정 (0~100)
    void SetProgress(int percent);

    // 상태 메시지 설정
    void SetStatusMessage(const CString& msg);

    // 다운로드 시작
    BOOL StartDownload(const CString& url, const CString& destPath);

    // 취소 여부 확인
    BOOL IsCancelled() const { return m_bCancelled; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    afx_msg void OnBtnCancel();
    DECLARE_MESSAGE_MAP()

private:
    CProgressCtrl m_progressCtrl;
    CStatic       m_staticStatus;
    BOOL          m_bCancelled;
    int           m_nProgress;
};
