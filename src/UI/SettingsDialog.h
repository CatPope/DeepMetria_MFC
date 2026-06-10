// SettingsDialog.h - 설정 모달 다이얼로그
#pragma once
#include "resource.h"

class CSettingsDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CSettingsDialog)
public:
    explicit CSettingsDialog(CWnd* pParent = nullptr);
    virtual ~CSettingsDialog();

    enum { IDD = IDD_SETTINGS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;

    afx_msg void OnCategorySelChange();
    afx_msg void OnProviderSelChange();
    afx_msg void OnBtnApply();           // [적용] — 저장 후 다이얼로그 유지
    afx_msg void OnBtnTestConnection();  // [연결 테스트] — 짧은 요청으로 응답 확인
    DECLARE_MESSAGE_MAP()

private:
    void SaveSettings();
    void PopulateModelsForProvider(int providerIdx);  // 제공자 변경 시 모델 콤보 갱신

    CListBox  m_lstCategory;
    CComboBox m_cmbProvider;
    CComboBox m_cmbModel;
    CEdit     m_edApiKey;
};
