#pragma once
// CSettingsManager.h — LLM 설정 관리 (MFC 독립 싱글턴)
// 레지스트리 기반 설정 저장/로드, DPAPI 암호화/복호화
// SettingsDialog와 독립적으로 동작하며 단위 테스트 가능
// Architecture §3 / DetailedSpec §3.3 참조

#include "../../Common/Types.h"
#include <atlstr.h>
#include <unordered_map>

// CString 해시 펑터 — MSVC PCH 환경에서 namespace std 특수화 금지이므로 커스텀 펑터 사용
struct CStringHasherSettings {
    size_t operator()(const CString& str) const noexcept {
        size_t h = 0;
        for (int i = 0; i < str.GetLength(); i++)
            h = h * 31 + static_cast<size_t>(str[i]);
        return h;
    }
};
struct CStringEqualSettings {
    bool operator()(const CString& a, const CString& b) const noexcept {
        return a == b;
    }
};

// ============================================================
// CSettingsManager — 싱글턴 설정 관리자
// ============================================================
class CSettingsManager {
public:
    // 싱글턴 인스턴스 획득
    static CSettingsManager& Instance();

    // ── 프로바이더 설정 ──────────────────────────────────────
    void    SetProvider(const CString& provider);
    CString GetProvider() const;

    // ── 모델 설정 ────────────────────────────────────────────
    void    SetModel(const CString& model);
    CString GetModel() const;

    // ── API 키 설정 (프로바이더별) ────────────────────────────
    void    SetApiKey(const CString& provider, const CString& key);
    CString GetApiKey(const CString& provider) const;

    // ── 레지스트리 저장/로드 ──────────────────────────────────
    // 성공 시 TRUE, 실패 시 FALSE + outError 설정
    BOOL SaveToRegistry(AppError& outError);
    BOOL LoadFromRegistry(AppError& outError);

    // ── DPAPI 암호화/복호화 (public — 테스트에서 직접 호출 가능) ──
    static CString EncryptDPAPI(const CString& plainText);
    static CString DecryptDPAPI(const CString& cipherB64);

private:
    CSettingsManager();
    ~CSettingsManager();
    CSettingsManager(const CSettingsManager&) = delete;
    CSettingsManager& operator=(const CSettingsManager&) = delete;

    // 레지스트리 키 경로
    static const TCHAR REG_KEY[];

    // 메모리 상태
    CString m_provider;  // 현재 선택된 프로바이더 ("Claude"/"OpenAI"/"Gemini")
    CString m_model;     // 현재 선택된 모델

    // 프로바이더별 API 키 캐시 (복호화된 평문)
    std::unordered_map<CString, CString, CStringHasherSettings, CStringEqualSettings> m_apiKeys;
};
