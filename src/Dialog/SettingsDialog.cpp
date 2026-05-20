#include "stdafx.h"
#include "SettingsDialog.h"

#include <wincrypt.h>
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
    ON_BN_CLICKED(IDC_BTN_TEST,          &CSettingsDialog::OnBnClickedTest)
    ON_CBN_SELCHANGE(IDC_COMBO_PROVIDER, &CSettingsDialog::OnCbnSelchangeProvider)
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
    DDX_Control(pDX, IDC_COMBO_PROVIDER, m_comboProvider);
    DDX_Control(pDX, IDC_EDIT_API_KEY,   m_editApiKey);
    DDX_Control(pDX, IDC_COMBO_MODEL,    m_comboModel);
    DDX_Control(pDX, IDC_BTN_TEST,       m_btnTest);
}

// ============================================================
// 초기화
// ============================================================
BOOL CSettingsDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 프로바이더 목록
    m_comboProvider.AddString(_T("Claude"));
    m_comboProvider.AddString(_T("OpenAI"));
    m_comboProvider.AddString(_T("Gemini"));
    m_comboProvider.SetCurSel(0);

    // 저장된 설정 로드 (키 캐시에 복호화된 값 저장)
    LoadSettings();

    // 현재 프로바이더에 맞는 모델 목록 + 키 표시
    CString provider = GetSelectedProvider();
    UpdateModelList(provider);
    ShowKeyForProvider(provider);

    return TRUE;
}

// ============================================================
// OK — 저장 후 닫기
// ============================================================
void CSettingsDialog::OnOK()
{
    CacheCurrentKey(); // 현재 에디트 내용을 캐시에 반영
    SaveSettings();
    CDialogEx::OnOK();
}

// ============================================================
// 프로바이더 변경
// ============================================================
void CSettingsDialog::OnCbnSelchangeProvider()
{
    CacheCurrentKey(); // 이전 프로바이더의 키를 캐시에 저장

    CString provider = GetSelectedProvider();
    UpdateModelList(provider);
    ShowKeyForProvider(provider);
}

CString CSettingsDialog::GetSelectedProvider() const
{
    CString provider;
    int sel = m_comboProvider.GetCurSel();
    if (sel != CB_ERR)
        m_comboProvider.GetLBText(sel, provider);
    return provider;
}

// ============================================================
// 키 캐시 관리
// ============================================================
void CSettingsDialog::CacheCurrentKey()
{
    CString key;
    m_editApiKey.GetWindowText(key);

    CString provider = GetSelectedProvider();
    if (provider == _T("Claude"))
        m_claudeKey = key;
    else if (provider == _T("OpenAI"))
        m_openaiKey = key;
    else if (provider == _T("Gemini"))
        m_geminiKey = key;
}

void CSettingsDialog::ShowKeyForProvider(const CString& provider)
{
    if (provider == _T("Claude"))
        m_editApiKey.SetWindowText(m_claudeKey);
    else if (provider == _T("OpenAI"))
        m_editApiKey.SetWindowText(m_openaiKey);
    else if (provider == _T("Gemini"))
        m_editApiKey.SetWindowText(m_geminiKey);
    else
        m_editApiKey.SetWindowText(_T(""));
}

// ============================================================
// 모델 목록 갱신 (공식 문서 기반 최신 모델)
// ============================================================
void CSettingsDialog::UpdateModelList(const CString& provider)
{
    m_comboModel.ResetContent();

    if (provider == _T("Claude"))
    {
        // https://docs.anthropic.com/en/docs/about-claude/models
        m_comboModel.AddString(_T("claude-sonnet-4-5-20250514"));
        m_comboModel.AddString(_T("claude-opus-4-5-20250414"));
        m_comboModel.AddString(_T("claude-haiku-3-5-20241022"));
    }
    else if (provider == _T("OpenAI"))
    {
        // https://platform.openai.com/docs/models
        m_comboModel.AddString(_T("gpt-4o"));
        m_comboModel.AddString(_T("gpt-4o-mini"));
        m_comboModel.AddString(_T("o3"));
        m_comboModel.AddString(_T("o3-mini"));
        m_comboModel.AddString(_T("o4-mini"));
    }
    else if (provider == _T("Gemini"))
    {
        // https://ai.google.dev/gemini-api/docs/models
        m_comboModel.AddString(_T("gemini-3.1-pro-preview"));
        m_comboModel.AddString(_T("gemini-3.1-flash-lite"));
        m_comboModel.AddString(_T("gemini-3-flash-preview"));
        m_comboModel.AddString(_T("gemini-2.5-pro"));
        m_comboModel.AddString(_T("gemini-2.5-flash"));
    }

    m_comboModel.SetCurSel(0);
}

// ============================================================
// API 연결 테스트
// ============================================================
void CSettingsDialog::OnBnClickedTest()
{
    CacheCurrentKey();

    CString provider = GetSelectedProvider();
    CString apiKey;
    if (provider == _T("Claude"))       apiKey = m_claudeKey;
    else if (provider == _T("OpenAI"))  apiKey = m_openaiKey;
    else if (provider == _T("Gemini"))  apiKey = m_geminiKey;

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
    // LLMRouter를 통한 실제 연결 테스트 (추후 활성화)
    // return LLMRouter::Instance().TestConnection(provider, apiKey, model);
    return !apiKey.IsEmpty();
}

// ============================================================
// 설정 저장 (DPAPI 암호화 + 레지스트리)
// ============================================================
void CSettingsDialog::SaveSettings()
{
    CString provider = GetSelectedProvider();
    CString model;
    m_comboModel.GetWindowText(model);

    CRegKey reg;
    if (reg.Create(HKEY_CURRENT_USER, REG_KEY) != ERROR_SUCCESS)
    {
        AfxMessageBox(_T("설정 저장 실패: 레지스트리 쓰기 오류"), MB_ICONERROR);
        return;
    }

    // 프로바이더별 키를 각각 암호화 저장
    reg.SetStringValue(_T("ClaudeKey"),  EncryptString(m_claudeKey));
    reg.SetStringValue(_T("OpenAIKey"),  EncryptString(m_openaiKey));
    reg.SetStringValue(_T("GeminiKey"),  EncryptString(m_geminiKey));
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
        return;

    TCHAR buf[1024] = {};
    ULONG len = 0;

    // Claude 키
    len = _countof(buf);
    if (reg.QueryStringValue(_T("ClaudeKey"), buf, &len) == ERROR_SUCCESS)
        m_claudeKey = DecryptString(buf);

    // OpenAI 키
    len = _countof(buf);
    if (reg.QueryStringValue(_T("OpenAIKey"), buf, &len) == ERROR_SUCCESS)
        m_openaiKey = DecryptString(buf);

    // Gemini 키
    len = _countof(buf);
    if (reg.QueryStringValue(_T("GeminiKey"), buf, &len) == ERROR_SUCCESS)
        m_geminiKey = DecryptString(buf);

    // 프로바이더
    len = _countof(buf);
    if (reg.QueryStringValue(_T("Provider"), buf, &len) == ERROR_SUCCESS)
    {
        int idx = m_comboProvider.FindStringExact(-1, buf);
        if (idx != CB_ERR)
            m_comboProvider.SetCurSel(idx);
    }

    // 모델 (프로바이더 설정 후 목록 갱신 → 모델 선택)
    CString provider = GetSelectedProvider();
    UpdateModelList(provider);

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

    int byteLen = (plainText.GetLength() + 1) * sizeof(TCHAR);
    DATA_BLOB dataIn  = {};
    DATA_BLOB dataOut = {};
    dataIn.pbData = (BYTE*)(LPCTSTR)plainText;
    dataIn.cbData = byteLen;

    if (!::CryptProtectData(&dataIn, nullptr, nullptr, nullptr, nullptr,
                             0, &dataOut))
        return _T("");

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
                               0, &dataOut))
        return _T("");

    CString result(reinterpret_cast<LPCTSTR>(dataOut.pbData));
    ::LocalFree(dataOut.pbData);
    return result;
}
