#pragma once
// LLMProvider.h — LLM 프로바이더 추상 인터페이스
// DetailedSpec §4.4 참조

#include "../../Common/Types.h"
#include <vector>

// ============================================================
// ILLMProvider — 순수 가상 인터페이스
// ClaudeProvider, OpenAIProvider가 구현
// ============================================================
class ILLMProvider {
public:
    virtual ~ILLMProvider() {}

    // 동기 채팅 호출 (백그라운드 스레드에서 실행)
    // messages: 대화 이력 (role: "system"|"user"|"assistant")
    // model: 모델 식별자 (빈 문자열이면 기본 모델 사용)
    // outResponse: LLM 응답 텍스트
    // outError: 오류 정보
    // 반환: TRUE=성공, FALSE=실패
    virtual BOOL Chat(const CString&               systemPrompt,
                      const CString&               userMessage,
                      const CString&               model,
                      CString&                     outResponse,
                      AppError&                    outError) = 0;

    // 전체 메시지 이력을 받는 오버로드
    virtual BOOL ChatWithHistory(const std::vector<ChatMessage>& messages,
                                 const CString&                  model,
                                 CString&                        outResponse,
                                 AppError&                       outError) = 0;

    // 프로바이더 이름 반환 ("Claude" | "OpenAI")
    virtual CString GetProviderName() const = 0;

    // 기본 모델명 반환
    virtual CString GetDefaultModel() const = 0;

    // API 키 설정 (레지스트리/DPAPI에서 복호화된 값)
    virtual void SetApiKey(const CString& apiKey) = 0;

    // API 키가 설정되어 있는지 여부 (크로스 프로바이더 폴백 후보 판별용)
    virtual bool HasApiKey() const = 0;
};
