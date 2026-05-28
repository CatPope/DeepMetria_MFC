#include "stdafx.h"
#include "CSettingsManager.h"

#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")

// ============================================================
// 레지스트리 키 경로
// ============================================================
const TCHAR CSettingsManager::REG_KEY[] =
    _T("Software\\DeepMetria\\Settings");

// ============================================================
// 싱글턴
// ============================================================
CSettingsManager& CSettingsManager::Instance()
{
    static CSettingsManager instance;
    return instance;
}

CSettingsManager::CSettingsManager()
    : m_provider(_T("Claude"))
    , m_model(_T(""))
{
}

CSettingsManager::~CSettingsManager()
{
}

// ============================================================
// 프로바이더 설정
// ============================================================
void CSettingsManager::SetProvider(const CString& provider)
{
    m_provider = provider;
}

CString CSettingsManager::GetProvider() const
{
    return m_provider;
}

// ============================================================
// 모델 설정
// ============================================================
void CSettingsManager::SetModel(const CString& model)
{
    m_model = model;
}

CString CSettingsManager::GetModel() const
{
    return m_model;
}

// ============================================================
// API 키 설정 (프로바이더별)
// ============================================================
void CSettingsManager::SetApiKey(const CString& provider, const CString& key)
{
    m_apiKeys[provider] = key;
}

CString CSettingsManager::GetApiKey(const CString& provider) const
{
    auto it = m_apiKeys.find(provider);
    if (it != m_apiKeys.end())
        return it->second;
    return _T("");
}

// ============================================================
// 레지스트리 저장 (DPAPI 암호화)
// ============================================================
BOOL CSettingsManager::SaveToRegistry(AppError& outError)
{
    outError.Clear();

    HKEY hKey = nullptr;
    LONG ret = RegCreateKeyEx(HKEY_CURRENT_USER, REG_KEY, 0, nullptr,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                              &hKey, nullptr);
    if (ret != ERROR_SUCCESS)
    {
        outError.Set(_T("REG_CREATE_FAILED"), _T("레지스트리 키 생성에 실패했습니다."));
        return FALSE;
    }

    // 프로바이더 / 모델 (평문 저장)
    auto writeStr = [&](const TCHAR* name, const CString& val) {
        DWORD size = (val.GetLength() + 1) * sizeof(TCHAR);
        RegSetValueEx(hKey, name, 0, REG_SZ,
                      reinterpret_cast<const BYTE*>(val.GetString()), size);
    };

    writeStr(_T("Provider"), m_provider);
    writeStr(_T("Model"),    m_model);

    // 프로바이더별 API 키 (DPAPI 암호화)
    auto writeKey = [&](const TCHAR* name, const CString& provider) {
        CString plain  = GetApiKey(provider);
        CString cipher = EncryptDPAPI(plain);
        // 빈 키도 빈 문자열로 저장 (삭제가 아닌 덮어쓰기)
        DWORD size = (cipher.GetLength() + 1) * sizeof(TCHAR);
        RegSetValueEx(hKey, name, 0, REG_SZ,
                      reinterpret_cast<const BYTE*>(cipher.GetString()), size);
    };

    writeKey(_T("ClaudeKey"),  _T("Claude"));
    writeKey(_T("OpenAIKey"),  _T("OpenAI"));
    writeKey(_T("GeminiKey"),  _T("Gemini"));

    RegCloseKey(hKey);
    return TRUE;
}

// ============================================================
// 레지스트리 로드 (DPAPI 복호화)
// ============================================================
BOOL CSettingsManager::LoadFromRegistry(AppError& outError)
{
    outError.Clear();

    HKEY hKey = nullptr;
    LONG ret = RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY, 0, KEY_READ, &hKey);
    if (ret != ERROR_SUCCESS)
    {
        // 레지스트리 키가 없으면 미설정 상태 — 오류 아님, 기본값 유지
        return TRUE;
    }

    // 문자열 값 읽기 헬퍼
    auto readStr = [&](const TCHAR* name, CString& out) {
        DWORD type = 0, size = 0;
        if (RegQueryValueEx(hKey, name, nullptr, &type, nullptr, &size) != ERROR_SUCCESS
            || size == 0)
            return;
        std::vector<BYTE> buf(size + sizeof(TCHAR));
        if (RegQueryValueEx(hKey, name, nullptr, &type, buf.data(), &size) == ERROR_SUCCESS)
            out = CString(reinterpret_cast<const TCHAR*>(buf.data()));
    };

    // 프로바이더 / 모델
    readStr(_T("Provider"), m_provider);
    readStr(_T("Model"),    m_model);

    // 프로바이더별 API 키 (DPAPI 복호화)
    auto readKey = [&](const TCHAR* name, const CString& provider) {
        CString cipher;
        readStr(name, cipher);
        if (!cipher.IsEmpty())
        {
            CString plain = DecryptDPAPI(cipher);
            m_apiKeys[provider] = plain;
        }
    };

    readKey(_T("ClaudeKey"),  _T("Claude"));
    readKey(_T("OpenAIKey"),  _T("OpenAI"));
    readKey(_T("GeminiKey"),  _T("Gemini"));

    RegCloseKey(hKey);
    return TRUE;
}

// ============================================================
// DPAPI 암호화 (UTF-8 변환 → CryptProtectData → Base64)
// LLMRouter::EncryptDPAPI와 동일한 방식
// ============================================================
CString CSettingsManager::EncryptDPAPI(const CString& plainText)
{
    if (plainText.IsEmpty())
        return _T("");

    // UTF-16 → UTF-8
    int u8len = WideCharToMultiByte(CP_UTF8, 0, plainText, -1,
                                    nullptr, 0, nullptr, nullptr);
    std::vector<BYTE> u8buf(u8len);
    WideCharToMultiByte(CP_UTF8, 0, plainText, -1,
                        reinterpret_cast<char*>(u8buf.data()), u8len,
                        nullptr, nullptr);

    DATA_BLOB input  = { static_cast<DWORD>(u8buf.size()), u8buf.data() };
    DATA_BLOB output = { 0, nullptr };

    if (!CryptProtectData(&input, L"DeepMetria", nullptr, nullptr, nullptr,
                          0, &output))
        return _T("");

    // Base64 인코딩
    DWORD b64Len = 0;
    CryptBinaryToString(output.pbData, output.cbData,
                        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                        nullptr, &b64Len);
    CString result;
    CryptBinaryToString(output.pbData, output.cbData,
                        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                        result.GetBufferSetLength(b64Len), &b64Len);
    result.ReleaseBuffer();

    LocalFree(output.pbData);
    return result;
}

// ============================================================
// DPAPI 복호화 (Base64 → CryptUnprotectData → UTF-16)
// LLMRouter::DecryptDPAPI와 동일한 방식
// ============================================================
CString CSettingsManager::DecryptDPAPI(const CString& cipherB64)
{
    if (cipherB64.IsEmpty())
        return _T("");

    // Base64 디코딩
    DWORD binLen = 0;
    CryptStringToBinary(cipherB64, cipherB64.GetLength(),
                        CRYPT_STRING_BASE64,
                        nullptr, &binLen, nullptr, nullptr);
    if (binLen == 0)
        return _T("");

    std::vector<BYTE> binBuf(binLen);
    CryptStringToBinary(cipherB64, cipherB64.GetLength(),
                        CRYPT_STRING_BASE64,
                        binBuf.data(), &binLen, nullptr, nullptr);

    DATA_BLOB input  = { binLen, binBuf.data() };
    DATA_BLOB output = { 0, nullptr };

    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                            0, &output))
        return _T("");

    // UTF-8 → UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                   reinterpret_cast<char*>(output.pbData),
                                   output.cbData, nullptr, 0);
    CString result;
    MultiByteToWideChar(CP_UTF8, 0,
                        reinterpret_cast<char*>(output.pbData), output.cbData,
                        result.GetBufferSetLength(wlen), wlen);
    result.ReleaseBuffer();

    LocalFree(output.pbData);
    return result;
}
