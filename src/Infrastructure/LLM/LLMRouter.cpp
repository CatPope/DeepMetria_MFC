#include "stdafx.h"
#include "LLMRouter.h"
#include "ClaudeProvider.h"
#include "OpenAIProvider.h"
#include <wincrypt.h>
#include <afxmt.h>

#pragma comment(lib, "crypt32.lib")

// 레지스트리 키 경로
static const TCHAR* REG_KEY_PATH = _T("Software\\DeepMetria\\APIKeys");

// ============================================================
// 싱글턴
// ============================================================

LLMRouter& LLMRouter::Instance() {
    static LLMRouter instance;
    return instance;
}

LLMRouter::LLMRouter()
    : m_activeProvider(Provider::Claude)
    , m_pClaude(std::make_unique<ClaudeProvider>())
    , m_pOpenAI(std::make_unique<OpenAIProvider>())
{
}

LLMRouter::~LLMRouter() {}

// ============================================================
// 프로바이더 설정
// ============================================================

void LLMRouter::SetProvider(Provider provider) {
    m_activeProvider = provider;
}

void LLMRouter::SetApiKey(Provider provider, const CString& apiKey) {
    if (provider == Provider::Claude)
        m_pClaude->SetApiKey(apiKey);
    else
        m_pOpenAI->SetApiKey(apiKey);
}

// ============================================================
// 동기 Chat
// ============================================================

BOOL LLMRouter::Chat(const CString& systemPrompt,
                      const CString& userMessage,
                      CString&       outResponse,
                      AppError&      outError) {
    outError.Clear();
    ILLMProvider* prov = ActiveProvider();
    if (!prov) {
        outError.Set(_T("NO_PROVIDER"), _T("활성 LLM 프로바이더가 없습니다."));
        return FALSE;
    }
    return prov->Chat(systemPrompt, userMessage, m_model, outResponse, outError);
}

BOOL LLMRouter::ChatWithHistory(const std::vector<ChatMessage>& messages,
                                 CString&                        outResponse,
                                 AppError&                       outError) {
    outError.Clear();
    ILLMProvider* prov = ActiveProvider();
    if (!prov) {
        outError.Set(_T("NO_PROVIDER"), _T("활성 LLM 프로바이더가 없습니다."));
        return FALSE;
    }
    return prov->ChatWithHistory(messages, m_model, outResponse, outError);
}

// ============================================================
// 비동기 Chat — AfxBeginThread 사용
// ============================================================

void LLMRouter::ChatAsync(const CString& systemPrompt,
                           const CString& userMessage,
                           HWND           hWnd,
                           LPVOID         pContext) {
    AsyncParam* pParam = new AsyncParam();
    pParam->systemPrompt = systemPrompt;
    pParam->userMessage  = userMessage;
    pParam->hWnd         = hWnd;
    pParam->pContext     = pContext;
    pParam->pRouter      = this;

    AfxBeginThread(AsyncThreadProc, pParam, THREAD_PRIORITY_NORMAL, 0, 0, nullptr);
}

UINT LLMRouter::AsyncThreadProc(LPVOID pParam) {
    AsyncParam* p = static_cast<AsyncParam*>(pParam);

    LLMAsyncResult* pResult = new LLMAsyncResult();
    pResult->pContext = p->pContext;

    p->pRouter->Chat(p->systemPrompt, p->userMessage, pResult->response, pResult->error);

    // 완료 메시지 전송 (수신 측에서 pResult delete)
    if (IsWindow(p->hWnd)) {
        ::PostMessage(p->hWnd, WM_LLM_RESPONSE, (WPARAM)p->pContext, (LPARAM)pResult);
    } else {
        delete pResult;
    }

    delete p;
    return 0;
}

// ============================================================
// 레지스트리 API 키 저장/로드 (DPAPI 암호화)
// ============================================================

BOOL LLMRouter::LoadApiKeysFromRegistry(AppError& outError) {
    outError.Clear();

    HKEY hKey = nullptr;
    LONG ret = RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_PATH, 0, KEY_READ, &hKey);
    if (ret != ERROR_SUCCESS) {
        // 키가 없으면 미설정 상태 — 에러 아님
        return TRUE;
    }

    auto loadKey = [&](const TCHAR* valueName, Provider provider) {
        DWORD type = 0, size = 0;
        RegQueryValueEx(hKey, valueName, nullptr, &type, nullptr, &size);
        if (size == 0) return;

        std::vector<BYTE> buf(size);
        if (RegQueryValueEx(hKey, valueName, nullptr, &type, buf.data(), &size) == ERROR_SUCCESS) {
            // 저장된 값은 Base64 인코딩된 DPAPI 암호문
            CString cipher((const TCHAR*)buf.data(), size / sizeof(TCHAR));
            CString plain = DecryptDPAPI(cipher);
            if (!plain.IsEmpty()) SetApiKey(provider, plain);
        }
    };

    loadKey(_T("ClaudeApiKey"), Provider::Claude);
    loadKey(_T("OpenAIApiKey"), Provider::OpenAI);

    RegCloseKey(hKey);
    return TRUE;
}

BOOL LLMRouter::SaveApiKeyToRegistry(Provider       provider,
                                      const CString& apiKey,
                                      AppError&      outError) {
    outError.Clear();

    CString cipher = EncryptDPAPI(apiKey);
    if (cipher.IsEmpty()) {
        outError.Set(_T("ENCRYPT_FAILED"), _T("API 키 암호화에 실패했습니다."));
        return FALSE;
    }

    HKEY hKey = nullptr;
    LONG ret = RegCreateKeyEx(HKEY_CURRENT_USER, REG_KEY_PATH, 0, nullptr,
                               REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (ret != ERROR_SUCCESS) {
        outError.Set(_T("REG_CREATE_FAILED"), _T("레지스트리 키 생성에 실패했습니다."));
        return FALSE;
    }

    const TCHAR* valueName = (provider == Provider::Claude) ? _T("ClaudeApiKey") : _T("OpenAIApiKey");
    DWORD size = (cipher.GetLength() + 1) * sizeof(TCHAR);
    RegSetValueEx(hKey, valueName, 0, REG_SZ, (const BYTE*)cipher.GetString(), size);
    RegCloseKey(hKey);

    // 메모리에도 즉시 반영
    SetApiKey(provider, apiKey);
    return TRUE;
}

// ============================================================
// DPAPI 암호화/복호화
// ============================================================

CString LLMRouter::EncryptDPAPI(const CString& plainText) {
    // UTF-8 변환
    int u8len = WideCharToMultiByte(CP_UTF8, 0, plainText, -1, nullptr, 0, nullptr, nullptr);
    std::vector<BYTE> u8buf(u8len);
    WideCharToMultiByte(CP_UTF8, 0, plainText, -1, (char*)u8buf.data(), u8len, nullptr, nullptr);

    DATA_BLOB input  = { (DWORD)u8buf.size(), u8buf.data() };
    DATA_BLOB output = { 0, nullptr };

    // 플래그 0: 현재 사용자 계정에서만 복호화 가능 (LOCAL_MACHINE 제거로 보안 강화)
    if (!CryptProtectData(&input, L"DeepMetria", nullptr, nullptr, nullptr,
                          0, &output)) {
        return _T("");
    }

    // Base64 인코딩
    DWORD b64Len = 0;
    CryptBinaryToString(output.pbData, output.cbData,
                        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &b64Len);
    CString result;
    CryptBinaryToString(output.pbData, output.cbData,
                        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                        result.GetBufferSetLength(b64Len), &b64Len);
    result.ReleaseBuffer();

    LocalFree(output.pbData);
    return result;
}

CString LLMRouter::DecryptDPAPI(const CString& cipherB64) {
    // Base64 디코딩
    DWORD binLen = 0;
    CryptStringToBinary(cipherB64, cipherB64.GetLength(),
                        CRYPT_STRING_BASE64, nullptr, &binLen, nullptr, nullptr);
    if (binLen == 0) return _T("");

    std::vector<BYTE> binBuf(binLen);
    CryptStringToBinary(cipherB64, cipherB64.GetLength(),
                        CRYPT_STRING_BASE64, binBuf.data(), &binLen, nullptr, nullptr);

    DATA_BLOB input  = { binLen, binBuf.data() };
    DATA_BLOB output = { 0, nullptr };

    // 플래그 0: 현재 사용자 계정에서만 복호화 가능 (LOCAL_MACHINE 제거로 보안 강화)
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                            0, &output)) {
        return _T("");
    }

    // UTF-8 → CString
    int wlen = MultiByteToWideChar(CP_UTF8, 0, (char*)output.pbData, output.cbData, nullptr, 0);
    CString result;
    MultiByteToWideChar(CP_UTF8, 0, (char*)output.pbData, output.cbData,
                        result.GetBufferSetLength(wlen), wlen);
    result.ReleaseBuffer();

    LocalFree(output.pbData);
    return result;
}

// ============================================================
// 내부 헬퍼
// ============================================================

ILLMProvider* LLMRouter::ActiveProvider() {
    switch (m_activeProvider) {
    case Provider::Claude: return m_pClaude.get();
    case Provider::OpenAI: return m_pOpenAI.get();
    default:               return m_pClaude.get();
    }
}
