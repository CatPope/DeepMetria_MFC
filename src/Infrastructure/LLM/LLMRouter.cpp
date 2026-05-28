#include "stdafx.h"
#include "LLMRouter.h"
#include "ClaudeProvider.h"
#include "OpenAIProvider.h"
#include "GeminiProvider.h"
#include <wincrypt.h>

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
    , m_pGemini(std::make_unique<GeminiProvider>())
{
}

// ============================================================
// 사용량 한도(QUOTA) 폴백 체인
// 프로바이더별 모델 ID(최상위 → 최하위 순서).
// SettingsDialog::UpdateModelList의 공식 모델 ID와 일치시킨다.
// ============================================================

const std::vector<CString>& LLMRouter::GetFallbackChain(Provider p) {
    static const std::vector<CString> claudeChain = {
        _T("claude-opus-4-5-20250414"),
        _T("claude-sonnet-4-5-20250514"),
        _T("claude-haiku-3-5-20241022"),
    };
    static const std::vector<CString> openaiChain = {
        _T("gpt-4o"),
        _T("gpt-4o-mini"),
        _T("o4-mini"),
    };
    static const std::vector<CString> geminiChain = {
        _T("gemini-3.1-pro-preview"),
        _T("gemini-2.5-pro"),
        _T("gemini-2.5-flash"),
        _T("gemini-3.1-flash-lite"),
    };
    static const std::vector<CString> empty;

    switch (p) {
    case Provider::Claude: return claudeChain;
    case Provider::OpenAI: return openaiChain;
    case Provider::Gemini: return geminiChain;
    default:               return empty;
    }
}

CString LLMRouter::GetFallbackModel(Provider p, const CString& currentModel) const {
    const std::vector<CString>& chain = GetFallbackChain(p);
    if (chain.empty())
        return _T("");

    // 빈 모델 = 프로바이더 기본(최상위) 모델 → 두 번째 항목으로 내려간다.
    if (currentModel.IsEmpty())
        return (chain.size() >= 2) ? chain[1] : _T("");

    // 현재 모델 위치를 찾아 한 단계 아래 모델을 반환한다.
    for (size_t i = 0; i < chain.size(); ++i) {
        if (chain[i].CompareNoCase(currentModel) == 0) {
            return (i + 1 < chain.size()) ? chain[i + 1] : _T("");
        }
    }

    // 알 수 없는 모델 ID
    return _T("");
}

CString LLMRouter::TakeFallbackNotice() {
    CString notice = m_fallbackNotice;
    m_fallbackNotice.Empty();
    return notice;
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
    else if (provider == Provider::OpenAI)
        m_pOpenAI->SetApiKey(apiKey);
    else if (provider == Provider::Gemini)
        m_pGemini->SetApiKey(apiKey);
}

#ifdef DEEPMETRIA_UNIT_TEST
void LLMRouter::SetProviderForTest(Provider p, std::unique_ptr<ILLMProvider> prov) {
    switch (p) {
    case Provider::Claude: m_pClaude = std::move(prov); break;
    case Provider::OpenAI: m_pOpenAI = std::move(prov); break;
    case Provider::Gemini: m_pGemini = std::move(prov); break;
    default: break;
    }
}
#endif

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

BOOL LLMRouter::ChatWithRetry(const CString& systemPrompt,
                               const CString& userMessage,
                               CString&       outResponse,
                               AppError&      outError,
                               int            maxRetries) {
    // 크로스 프로바이더 폴백 시 무한 루프를 방지하기 위해
    // 이미 시도한(소진된) 프로바이더를 기록한다. 프로바이더는 최대 3개이므로 종료가 보장된다.
    bool visited[3] = { false, false, false };

    for (int attempt = 0; attempt <= maxRetries; ++attempt) {
        outError.Clear();
        outResponse.Empty();

        BOOL result = Chat(systemPrompt, userMessage, outResponse, outError);
        if (result)
            return TRUE;

        // ── 사용량 한도(QUOTA) 초과: 모델/프로바이더 폴백 후 재시도 ──
        // 단순 동일 모델 재시도가 아니라 모델 다운그레이드 → 프로바이더 전환이 핵심 동작이다.
        if (outError.code == _T("QUOTA_EXCEEDED")) {
            // (a) 같은 프로바이더 내 하위 모델로 즉시 전환 후 재시도.
            CString nextModel = GetFallbackModel(m_activeProvider, m_model);
            if (!nextModel.IsEmpty()) {
                CString prevModel = m_model.IsEmpty()
                    ? GetFallbackChain(m_activeProvider).front()
                    : m_model;

                CString notice;
                notice.Format(
                    _T("사용량 한도 초과 — 모델을 '%s'에서 '%s'(으)로 전환했습니다."),
                    (LPCTSTR)prevModel, (LPCTSTR)nextModel);
                m_fallbackNotice = notice;

                // 세션 동안 전환된 모델을 유지한다.
                m_model = nextModel;

                // 모델만 교체하고 즉시 재시도 (백오프 없음). attempt를 소비하지 않도록 유지.
                --attempt;
                continue;
            }

            // (b) 현재 프로바이더 체인이 바닥남 — API 키가 있는 다른 프로바이더로 전환한다.
            // 현재 프로바이더는 소진된 것으로 표시한다.
            visited[(int)m_activeProvider] = true;

            // 고정 우선순위: Gemini → OpenAI → Claude (저사양 우선 사용자 요구 반영).
            static const Provider kOrder[3] = {
                Provider::Gemini, Provider::OpenAI, Provider::Claude
            };

            Provider chosen = m_activeProvider;
            bool found = false;
            for (Provider cand : kOrder) {
                if (visited[(int)cand])
                    continue;
                ILLMProvider* p = ProviderInstance(cand);
                if (p && p->HasApiKey()) {
                    chosen = cand;
                    found = true;
                    break;
                }
                // 키 없는 프로바이더도 방문 처리하여 재검토하지 않는다.
                visited[(int)cand] = true;
            }

            if (found) {
                CString prevName = ProviderName(m_activeProvider);
                CString newName  = ProviderName(chosen);

                // 새 프로바이더의 가장 저사양(체인 최하위) 모델로 전환한다.
                const std::vector<CString>& chain = GetFallbackChain(chosen);
                CString newModel = chain.empty() ? _T("") : chain.back();

                SetProvider(chosen);
                m_model = newModel;
                visited[(int)chosen] = true;  // 선택한 프로바이더도 소진 후보로 표시

                CString notice;
                notice.Format(
                    _T("사용량 한도 초과 — 프로바이더 '%s'에서 '%s' 모델 '%s'(으)로 전환했습니다."),
                    (LPCTSTR)prevName, (LPCTSTR)newName, (LPCTSTR)newModel);
                m_fallbackNotice = notice;

                // 프로바이더만 교체하고 즉시 재시도 (백오프 없음).
                --attempt;
                continue;
            }

            // (c) 키가 있는 대체 프로바이더가 없음 — 모든 모델 소진.
            m_fallbackNotice = _T("모든 사용 가능한 모델의 사용량을 초과했습니다.");
            break;
        }

        // 마지막 시도였으면 더 이상 재시도하지 않음
        if (attempt >= maxRetries)
            break;

        // 지수 백오프 대기 (단위 테스트에서는 DEEPMETRIA_UNIT_TEST 매크로로 스킵 가능)
#ifndef DEEPMETRIA_UNIT_TEST
        DWORD waitMs = 1000 * (1 << attempt); // 1s, 2s, 4s, ...
        Sleep(waitMs);
#endif
    }
    return FALSE;
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

#ifndef DEEPMETRIA_UNIT_TEST
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
#endif

#ifndef DEEPMETRIA_UNIT_TEST
UINT LLMRouter::AsyncThreadProc(LPVOID pParam) {
    AsyncParam* p = static_cast<AsyncParam*>(pParam);

    LLMAsyncResult* pResult = new LLMAsyncResult();
    pResult->pContext = p->pContext;

    p->pRouter->Chat(p->systemPrompt, p->userMessage, pResult->response, pResult->error);

    if (IsWindow(p->hWnd)) {
        ::PostMessage(p->hWnd, WM_LLM_RESPONSE, (WPARAM)p->pContext, (LPARAM)pResult);
    } else {
        delete pResult;
    }

    delete p;
    return 0;
}
#endif

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
    loadKey(_T("GeminiApiKey"), Provider::Gemini);

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

    const TCHAR* valueName = (provider == Provider::Claude)  ? _T("ClaudeApiKey")
                           : (provider == Provider::OpenAI) ? _T("OpenAIApiKey")
                           :                                  _T("GeminiApiKey");
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
    return ProviderInstance(m_activeProvider);
}

ILLMProvider* LLMRouter::ProviderInstance(Provider p) {
    switch (p) {
    case Provider::Claude: return m_pClaude.get();
    case Provider::OpenAI: return m_pOpenAI.get();
    case Provider::Gemini: return m_pGemini.get();
    default:               return m_pClaude.get();
    }
}

CString LLMRouter::ProviderName(Provider p) {
    switch (p) {
    case Provider::Claude: return _T("Claude");
    case Provider::OpenAI: return _T("OpenAI");
    case Provider::Gemini: return _T("Gemini");
    default:               return _T("Unknown");
    }
}
