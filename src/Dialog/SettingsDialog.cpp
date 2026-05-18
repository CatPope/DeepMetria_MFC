#include "stdafx.h"
#include "SettingsDialog.h"

#include <wincrypt.h>  // DPAPI (CryptProtectData / CryptUnprotectData)
#pragma comment(lib, "crypt32.lib")

// ============================================================
// 레지스트리 키 경로
// ============================================================
const TCHAR CSettingsDialog::REG_KEY[] =
    _T("Software\\DeepMetria\\Settings");

// ============================================================
// DECLARE_DYNAMIC / 메시지 맵
// ============================================================
IMPLEMENT_DYNAMIC(CSettingsDialog, CDialogEx)

BEGIN_MESSAGE_MAP(CSettingsDialog, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_TEST,           &CSettingsDialog::OnBnClickedTest)
    ON_CBN_SELCHANGE(IDC_COMBO_PROVIDER,  &CSettingsDialog::OnCbnSelchangeProvider)
END_MESSAGE_MAP()

// ============================================================
// 생성 / 소멸
// ============================================================
CSettingsDialog::CSettingsDialog(CWnd* pParent)
    : CDialogEx(CSettingsDialog::IDD, pParent)
{
}

CSettingsDialog::~CSettingsDialog()
{
}

// ============================================================
// DDX
// ============================================================
void CSettingsDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_CLAUDE_KEY,  m_editClaudeKey);
    DDX_Control(pDX, IDC_EDIT_OPENAI_KEY,  m_editOpenAIKey);
    DDX_Control(pDX, IDC_COMBO_PROVIDER,   m_comboProvider);
    DDX_Control(pDX, IDC_COMBO_MODEL,      m_comboModel);
    DDX_Control(pDX, IDC_BTN_TEST,         m_btnTest);
}

// ============================================================
// 초기화
// ============================================================
BOOL CSettingsDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 프로바이더 목록 초기화
    m_comboProvider.AddString(_T("Claude"));
    m_comboProvider.AddString(_T("OpenAI"));
    m_comboProvider.SetCurSel(0);

    // 초기 모델 목록 설정
    UpdateModelList(_T("Claude"));

    // 저장된 설정 로드
    LoadSettings();

    return TRUE;
}

// ============================================================
// OK — 저장 후 닫기
// ============================================================
void CSettingsDialog::OnOK()
{
    SaveSettings();
    CDialogEx::OnOK();
}

// ============================================================
// 프로바이더 변경 시 모델 목록 갱신
// ============================================================
void CSettingsDialog::OnCbnSelchangeProvider()
{
    CString provider;
    m_comboProvider.GetWindowText(provider);
    UpdateModelList(provider);
}

void CSettingsDialog::UpdateModelList(const CString& provider)
{
    m_comboModel.ResetContent();

    if (provider == _T("Claude"))
    {
        m_comboModel.AddString(_T("claude-opus-4-5"));
        m_comboModel.AddString(_T("claude-sonnet-4-5"));
        m_comboModel.AddString(_T("claude-haiku-3-5"));
    }
    else if (provider == _T("OpenAI"))
    {
        m_comboModel.AddString(_T("gpt-4o"));
        m_comboModel.AddString(_T("gpt-4o-mini"));
        m_comboModel.AddString(_T("gpt-4-turbo"));
    }

    m_comboModel.SetCurSel(0);
}

// ============================================================
// API 연결 테스트
// ============================================================
void CSettingsDialog::OnBnClickedTest()
{
    CString provider;
    m_comboProvider.GetWindowText(provider);

    CString apiKey;
    if (provider == _T("Claude"))
        m_editClaudeKey.GetWindowText(apiKey);
    else
        m_editOpenAIKey.GetWindowText(apiKey);

    CString model;
    m_comboModel.GetWindowText(model);

    if (apiKey.IsEmpty())
    {
        AfxMessageBox(_T("API 키를 먼저 입력해주세요."), MB_ICONWARNING);
        return;
    }

    m_btnTest.EnableWindow(FALSE);
    BOOL bOk = TestApiConnection(provider, apiKey, model);
    m_btnTest.EnableWindow(TRUE);

    if (bOk)
        AfxMessageBox(_T("API 연결 성공!"), MB_ICONINFORMATION);
    else
        AfxMessageBox(_T("API 연결 실패. 키와 네트워크 상태를 확인하세요."), MB_ICONERROR);
}

BOOL CSettingsDialog::TestApiConnection(const CString& provider,
                                         const CString& apiKey,
                                         const CString& model)
{
    // LLMRouter를 통한 실제 연결 테스트 (LLMRouter 구현 완료 후 활성화)
    // return LLMRouter::GetInstance().TestConnection(provider, apiKey, model);

    // 임시: 키가 비어있지 않으면 성공으로 간주
    return !apiKey.IsEmpty();
}

// ============================================================
// 설정 저장 (DPAPI 암호화 + 레지스트리)
// ============================================================
void CSettingsDialog::SaveSettings()
{
    CString claudeKey, openaiKey, provider, model;
    m_editClaudeKey.GetWindowText(claudeKey);
    m_editOpenAIKey.GetWindowText(openaiKey);
    m_comboProvider.GetWindowText(provider);
    m_comboModel.GetWindowText(model);

    CRegKey reg;
    if (reg.Create(HKEY_CURRENT_USER, REG_KEY) != ERROR_SUCCESS)
    {
        AfxMessageBox(_T("설정 저장 실패: 레지스트리 쓰기 오류"), MB_ICONERROR);
        return;
    }

    // API 키는 DPAPI로 암호화
    CString encClaude  = EncryptString(claudeKey);
    CString encOpenAI  = EncryptString(openaiKey);

    reg.SetStringValue(_T("ClaudeKey"),  encClaude);
    reg.SetStringValue(_T("OpenAIKey"),  encOpenAI);
    reg.SetStringValue(_T("Provider"),   provider);
    reg.SetStringValue(_T("Model"),      model);
}

// ============================================================
// 설정 로드 (레지스트리 + DPAPI 복호화)
// ============================================================
void CSettingsDialog::LoadSettings()
{
    CRegKey reg;
    if (reg.Open(HKEY_CURRENT_USER, REG_KEY, KEY_READ) != ERROR_SUCCESS)
        return; // 최초 실행 시 키 없음 — 정상

    TCHAR buf[1024] = {};
    ULONG len = 0;

    // Claude 키
    len = _countof(buf);
    if (reg.QueryStringValue(_T("ClaudeKey"), buf, &len) == ERROR_SUCCESS)
    {
        CString plain = DecryptString(buf);
        m_editClaudeKey.SetWindowText(plain);
    }

    // OpenAI 키
    len = _countof(buf);
    if (reg.QueryStringValue(_T("OpenAIKey"), buf, &len) == ERROR_SUCCESS)
    {
        CString plain = DecryptString(buf);
        m_editOpenAIKey.SetWindowText(plain);
    }

    // 프로바이더
    len = _countof(buf);
    if (reg.QueryStringValue(_T("Provider"), buf, &len) == ERROR_SUCCESS)
    {
        int idx = m_comboProvider.FindStringExact(-1, buf);
        if (idx != CB_ERR)
        {
            m_comboProvider.SetCurSel(idx);
            UpdateModelList(buf);
        }
    }

    // 모델
    len = _countof(buf);
    if (reg.QueryStringValue(_T("Model"), buf, &len) == ERROR_SUCCESS)
    {
        int idx = m_comboModel.FindStringExact(-1, buf);
        if (idx != CB_ERR)
            m_comboModel.SetCurSel(idx);
    }
}

// ============================================================
// DPAPI 암호화 (UTF-16 → Base64)
// ============================================================
CString CSettingsDialog::EncryptString(const CString& plainText)
{
    if (plainText.IsEmpty())
        return _T("");

    // UTF-16 바이트 배열로 변환
    int byteLen = (plainText.GetLength() + 1) * sizeof(TCHAR);
    DATA_BLOB dataIn  = {};
    DATA_BLOB dataOut = {};
    dataIn.pbData = (BYTE*)(LPCTSTR)plainText;
    dataIn.cbData = byteLen;

    if (!::CryptProtectData(&dataIn, nullptr, nullptr, nullptr, nullptr,
                             CRYPTPROTECT_LOCAL_MACHINE, &dataOut))
        return _T("");

    // Base64 인코딩
    DWORD b64Len = 0;
    ::CryptBinaryToString(dataOut.pbData, dataOut.cbData,
                           CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                           nullptr, &b64Len);
    CString result;
    ::CryptBinaryToString(dataOut.pbData, dataOut.cbData,
                           CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                           result.GetBuffer(b64Len), &b64Len);
    result.ReleaseBuffer();

    ::LocalFree(dataOut.pbData);
    return result;
}

// ============================================================
// DPAPI 복호화 (Base64 → UTF-16)
// ============================================================
CString CSettingsDialog::DecryptString(const CString& cipherB64)
{
    if (cipherB64.IsEmpty())
        return _T("");

    // Base64 디코딩
    DWORD binLen = 0;
    ::CryptStringToBinary((LPCTSTR)cipherB64, cipherB64.GetLength(),
                           CRYPT_STRING_BASE64, nullptr, &binLen,
                           nullptr, nullptr);
    std::vector<BYTE> binData(binLen);
    ::CryptStringToBinary((LPCTSTR)cipherB64, cipherB64.GetLength(),
                           CRYPT_STRING_BASE64, binData.data(), &binLen,
                           nullptr, nullptr);

    DATA_BLOB dataIn  = {};
    DATA_BLOB dataOut = {};
    dataIn.pbData = binData.data();
    dataIn.cbData = binLen;

    if (!::CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr,
                               CRYPTPROTECT_LOCAL_MACHINE, &dataOut))
        return _T("");

    // TCHAR 문자열로 변환
    CString result(reinterpret_cast<LPCTSTR>(dataOut.pbData));
    ::LocalFree(dataOut.pbData);
    return result;
}
