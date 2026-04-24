#pragma once

// SettingsDialog.h — LLM API 키 설정 다이얼로그
// DPAPI로 암호화하여 레지스트리에 저장
// Architecture §3 / DetailedSpec §3.3 참조

#include "../stdafx.h"

// ============================================================
// CSettingsDialog — CDialogEx 파생
// ============================================================
class CSettingsDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CSettingsDialog)

public:
    // IDD 리소스 ID (리소스 파일에 정의 필요)
    enum { IDD = IDD_SETTINGS };

    CSettingsDialog(CWnd* pParent = nullptr);
    virtual ~CSettingsDialog();

protected:
    // 컨트롤
    CEdit     m_editClaudeKey;   // Claude API 키 입력
    CEdit     m_editOpenAIKey;   // OpenAI API 키 입력
    CComboBox m_comboProvider;   // LLM 프로바이더 선택 (Claude/OpenAI)
    CComboBox m_comboModel;      // 모델 선택
    CButton   m_btnTest;         // API 연결 테스트

    // CDialogEx 오버라이드
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;

    // 버튼 핸들러
    afx_msg void OnBnClickedTest();
    afx_msg void OnCbnSelchangeProvider();

    DECLARE_MESSAGE_MAP()

private:
    // 설정 저장/로드 (DPAPI + 레지스트리)
    void SaveSettings();
    void LoadSettings();

    // DPAPI 암호화/복호화
    CString EncryptString(const CString& plainText);
    CString DecryptString(const CString& cipherB64);

    // 프로바이더에 따른 모델 목록 갱신
    void UpdateModelList(const CString& provider);

    // API 테스트 (선택된 프로바이더로 "Hello" 요청)
    BOOL TestApiConnection(const CString& provider,
                           const CString& apiKey,
                           const CString& model);

    // 레지스트리 키 경로
    static const TCHAR REG_KEY[];
};
