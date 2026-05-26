#pragma once
// MockLLMProvider.h — ILLMProvider 목 구현
// Google Mock을 사용하여 LLM 프로바이더를 테스트 더블로 대체한다.
// 실제 API 호출 없이 Chat / ChatWithHistory 동작을 시뮬레이션한다.

#include <gmock/gmock.h>
#include "Infrastructure/LLM/LLMProvider.h"

// ============================================================
// MockLLMProvider — ILLMProvider 전체 메서드 목
// ============================================================
class MockLLMProvider : public ILLMProvider {
public:
    // ── 순수 가상 메서드 목 선언 ───────────────────────────
    MOCK_METHOD(BOOL, Chat,
        (const CString& systemPrompt,
         const CString& userMessage,
         const CString& model,
         CString&        outResponse,
         AppError&       outError),
        (override));

    MOCK_METHOD(BOOL, ChatWithHistory,
        (const std::vector<ChatMessage>& messages,
         const CString&                  model,
         CString&                        outResponse,
         AppError&                       outError),
        (override));

    MOCK_METHOD(CString, GetProviderName, (), (const, override));
    MOCK_METHOD(CString, GetDefaultModel, (), (const, override));
    MOCK_METHOD(void,    SetApiKey,       (const CString& apiKey), (override));

    // ── 헬퍼: 성공 응답 고정값 설정 ───────────────────────
    // 호출 시 outResponse에 fixedResponse를 기록하고 TRUE를 반환하도록 설정한다.
    void SetupSuccessResponse(const CString& fixedResponse) {
        using ::testing::_;
        using ::testing::DoAll;
        using ::testing::Return;
        using ::testing::SetArgReferee;

        ON_CALL(*this, Chat(_, _, _, _, _))
            .WillByDefault(DoAll(
                SetArgReferee<3>(fixedResponse),
                Return(TRUE)));

        ON_CALL(*this, ChatWithHistory(_, _, _, _))
            .WillByDefault(DoAll(
                SetArgReferee<2>(fixedResponse),
                Return(TRUE)));
    }

    // ── 헬퍼: 오류 응답 고정값 설정 ───────────────────────
    // 호출 시 outError를 채우고 FALSE를 반환하도록 설정한다.
    void SetupErrorResponse(const CString& errorCode,
                            const CString& errorMessage,
                            int severity = 2) {
        using ::testing::_;
        using ::testing::DoAll;
        using ::testing::Return;
        using ::testing::SetArgReferee;

        AppError err(errorCode, errorMessage, severity);

        ON_CALL(*this, Chat(_, _, _, _, _))
            .WillByDefault(DoAll(
                SetArgReferee<4>(err),
                Return(FALSE)));

        ON_CALL(*this, ChatWithHistory(_, _, _, _))
            .WillByDefault(DoAll(
                SetArgReferee<3>(err),
                Return(FALSE)));
    }

    // ── 헬퍼: 프로바이더 이름 / 기본 모델 기본값 설정 ────
    void SetupProviderInfo(const CString& providerName,
                           const CString& defaultModel) {
        using ::testing::Return;
        ON_CALL(*this, GetProviderName()).WillByDefault(Return(providerName));
        ON_CALL(*this, GetDefaultModel()).WillByDefault(Return(defaultModel));
    }
};
