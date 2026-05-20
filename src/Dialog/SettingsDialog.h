#pragma once

// SettingsDialog.h — LLM API 설정 다이얼로그
// 프로바이더별 API 키를 DPAPI로 암호화하여 레지스트리에 저장
// Architecture §3 / DetailedSpec §3.3 참조

#include "../Resources/resource.h"

// ============================================================
// CSettingsDialog — CDialogEx 파생
// ============================================================
class CSettingsDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CSettingsDialog)

public:
    enum { IDD = IDD_SETTINGS };

    CSettingsDialog(CWnd* pParent = nullptr);
    virtual ~CSettingsDialog();

protected:
    // 컨트롤
    CComboBox m_comboProvider;   // LLM 프로바이더 (Claude/OpenAI/Gemini)
    CEdit     m_editApiKey;     // API 키 입력 (프로바이더별 전환)
    CComboBox m_comboModel;     // 모델 선택
    CButton   m_btnTest;        // API 연결 테스트

    // 프로바이더별 키 캐시 (다이얼로그 내에서 전환 시 유지)
    CString m_claudeKey;
    CString m_openaiKey;
    CString m_geminiKey;

    // CDialogEx 오버라이드
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;

    // 버튼 핸들러
    afx_msg void OnBnClickedTest();
    afx_msg void OnCbnSelchangeProvider();

    DECLARE_MESSAGE_MAP()

private:
    void SaveSettings();
    void LoadSettings();

    // 프로바이더 전환 시 현재 키 캐시 저장 & 새 프로바이더 키 로드
    void CacheCurrentKey();
    void ShowKeyForProvider(const CString& provider);

    // 프로바이더에 따른 모델 목록 갱신
    void UpdateModelList(const CString& provider);

    // API 테스트
    BOOL TestApiConnection(const CString& provider,
                           const CString& apiKey,
                           const CString& model);

    // DPAPI 암호화/복호화
    CString EncryptString(const CString& plainText);
    CString DecryptString(const CString& cipherB64);

    // 현재 선택된 프로바이더
    CString GetSelectedProvider() const;

    static const TCHAR REG_KEY[];
};
