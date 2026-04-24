#pragma once
// LLMRouter.h — LLM 추상화 라우터
// 활성 프로바이더(Claude/OpenAI) 선택, API 키 관리, 동기/비동기 호출
// DetailedSpec §4.4 참조

#include "../../Common/Types.h"
#include "LLMProvider.h"
#include <memory>

// 비동기 완료 콜백 파라미터 구조체 (PostMessage LPARAM로 전달, 수신 측에서 delete)
struct LLMAsyncResult {
    CString   response;     // LLM 응답 텍스트
    AppError  error;        // 오류 정보 (IsError()==FALSE 이면 성공)
    LPVOID    pContext;     // 호출자가 전달한 사용자 데이터
};

// ============================================================
// LLMRouter — 싱글턴 LLM 라우터
// ============================================================
class LLMRouter {
public:
    // 프로바이더 식별자
    enum class Provider {
        Claude,
        OpenAI
    };

    // 싱글턴 인스턴스 획득
    static LLMRouter& Instance();

    // 현재 활성 프로바이더 설정
    void SetProvider(Provider provider);
    Provider GetProvider() const { return m_activeProvider; }

    // API 키 설정 (레지스트리/DPAPI에서 복호화된 값)
    void SetApiKey(Provider provider, const CString& apiKey);

    // 사용할 모델 설정 (빈 문자열이면 프로바이더 기본값 사용)
    void SetModel(const CString& model) { m_model = model; }

    // ── 동기 호출 (백그라운드 스레드에서 실행) ────────────
    // systemPrompt : 시스템 역할 지시
    // userMessage  : 사용자 질문
    // outResponse  : LLM 응답 텍스트
    // outError     : 오류 정보
    BOOL Chat(const CString& systemPrompt,
              const CString& userMessage,
              CString&       outResponse,
              AppError&      outError);

    // 전체 메시지 이력을 받는 오버로드
    BOOL ChatWithHistory(const std::vector<ChatMessage>& messages,
                         CString&                        outResponse,
                         AppError&                       outError);

    // ── 비동기 호출 (내부적으로 AfxBeginThread 사용) ──────
    // 완료 시 hWnd에 WM_LLM_RESPONSE 메시지 PostMessage
    // LPARAM: LLMAsyncResult* (수신 측에서 delete)
    // pContext: WM_LLM_RESPONSE의 WPARAM으로 그대로 전달
    void ChatAsync(const CString& systemPrompt,
                   const CString& userMessage,
                   HWND           hWnd,
                   LPVOID         pContext = nullptr);

    // 레지스트리(DPAPI)에서 API 키 로드
    BOOL LoadApiKeysFromRegistry(AppError& outError);

    // 레지스트리(DPAPI)에 API 키 저장
    BOOL SaveApiKeyToRegistry(Provider       provider,
                              const CString& apiKey,
                              AppError&      outError);

private:
    LLMRouter();
    ~LLMRouter();
    LLMRouter(const LLMRouter&) = delete;
    LLMRouter& operator=(const LLMRouter&) = delete;

    // 현재 활성 프로바이더 인스턴스 반환
    ILLMProvider* ActiveProvider();

    // DPAPI 암호화/복호화
    static CString EncryptDPAPI(const CString& plainText);
    static CString DecryptDPAPI(const CString& cipherB64);

    // 비동기 작업 스레드 함수
    static UINT AsyncThreadProc(LPVOID pParam);

    // 비동기 호출 파라미터 구조체
    struct AsyncParam {
        CString  systemPrompt;
        CString  userMessage;
        HWND     hWnd;
        LPVOID   pContext;
        LLMRouter* pRouter;
    };

    Provider                  m_activeProvider;
    CString                   m_model;          // 빈 문자열이면 프로바이더 기본값
    std::unique_ptr<ILLMProvider> m_pClaude;
    std::unique_ptr<ILLMProvider> m_pOpenAI;
};
