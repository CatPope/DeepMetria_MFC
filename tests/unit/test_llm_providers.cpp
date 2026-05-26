// test_llm_providers.cpp — 개별 LLM 프로바이더 단위 테스트
// ClaudeProvider / OpenAIProvider / GeminiProvider 의 공개 인터페이스를
// 실제 API 호출 없이 검증한다.
// 메타데이터(이름, 기본 모델) 및 API 키 미설정 시 오류 경로를 다룬다.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <windows.h>
#include <atlstr.h>

#include "Common/Types.h"
#include "Infrastructure/LLM/ClaudeProvider.h"
#include "Infrastructure/LLM/OpenAIProvider.h"
#include "Infrastructure/LLM/GeminiProvider.h"

// ============================================================
// ClaudeProvider 테스트
// ============================================================
class ClaudeProviderTest : public ::testing::Test {
protected:
    ClaudeProvider provider_;
};

// TC-CLAUDE-001: GetProviderName()은 "Claude"를 반환한다
TEST_F(ClaudeProviderTest, ProviderName_ReturnsClaude) {
    EXPECT_EQ(provider_.GetProviderName(), CString(_T("Claude")));
}

// TC-CLAUDE-002: GetDefaultModel()은 기대값을 반환한다
TEST_F(ClaudeProviderTest, DefaultModel_ReturnsExpectedModel) {
    CString model = provider_.GetDefaultModel();
    // claude-opus-4-5 (ClaudeProvider.h 29번 줄 기준)
    EXPECT_EQ(model, CString(_T("claude-opus-4-5")));
}

// TC-CLAUDE-003: SetApiKey 후 Chat 호출 시 API 키가 전달된다
// (실제 HTTP는 발생하므로 네트워크 없이 FALSE 반환, 크래시 없음을 검증)
TEST_F(ClaudeProviderTest, SetApiKeyAndChat_DoesNotCrash) {
    provider_.SetApiKey(_T("test-key-claude"));
    CString  response;
    AppError error;
    // 잘못된 키이므로 FALSE 반환 예상, 크래시 없어야 한다
    EXPECT_NO_THROW(provider_.Chat(_T("system"), _T("hi"), _T(""), response, error));
}

// TC-CLAUDE-004: API 키 없이 Chat 호출 시 FALSE 반환 및 오류 설정
TEST_F(ClaudeProviderTest, ChatWithoutApiKey_ReturnsFalseWithError) {
    // 기본 생성 시 m_apiKey 가 비어 있음
    CString  response;
    AppError error;
    BOOL result = provider_.Chat(_T("system"), _T("hello"), _T(""), response, error);
    EXPECT_EQ(result, FALSE);
    EXPECT_TRUE(error.IsError())
        << "오류가 설정되지 않음. code=" << (LPCWSTR)error.code;
}

// TC-CLAUDE-005: API 키 없이 ChatWithHistory 빈 벡터 호출 시 EMPTY_MESSAGES 오류
TEST_F(ClaudeProviderTest, ChatWithHistory_EmptyMessages_ReturnsEmptyMessagesError) {
    std::vector<ChatMessage> messages; // 비어 있음
    CString  response;
    AppError error;
    BOOL result = provider_.ChatWithHistory(messages, _T(""), response, error);
    EXPECT_EQ(result, FALSE);
    EXPECT_EQ(error.code, CString(_T("EMPTY_MESSAGES")));
}

// ============================================================
// OpenAIProvider 테스트
// ============================================================
class OpenAIProviderTest : public ::testing::Test {
protected:
    OpenAIProvider provider_;
};

// TC-OPENAI-001: GetProviderName()은 "OpenAI"를 반환한다
TEST_F(OpenAIProviderTest, ProviderName_ReturnsOpenAI) {
    EXPECT_EQ(provider_.GetProviderName(), CString(_T("OpenAI")));
}

// TC-OPENAI-002: GetDefaultModel()은 "gpt-4o"를 반환한다
TEST_F(OpenAIProviderTest, DefaultModel_ReturnsGpt4o) {
    EXPECT_EQ(provider_.GetDefaultModel(), CString(_T("gpt-4o")));
}

// TC-OPENAI-003: SetApiKey 후 Chat 호출 시 크래시 없음
TEST_F(OpenAIProviderTest, SetApiKeyAndChat_DoesNotCrash) {
    provider_.SetApiKey(_T("sk-test-openai"));
    CString  response;
    AppError error;
    EXPECT_NO_THROW(provider_.Chat(_T("system"), _T("hi"), _T(""), response, error));
}

// TC-OPENAI-004: API 키 없이 Chat 호출 시 FALSE 반환 및 오류 설정
TEST_F(OpenAIProviderTest, ChatWithoutApiKey_ReturnsFalseWithError) {
    CString  response;
    AppError error;
    BOOL result = provider_.Chat(_T("system"), _T("hello"), _T(""), response, error);
    EXPECT_EQ(result, FALSE);
    EXPECT_TRUE(error.IsError())
        << "오류가 설정되지 않음. code=" << (LPCWSTR)error.code;
}

// TC-OPENAI-005: ChatWithHistory 빈 벡터 호출 시 EMPTY_MESSAGES 오류
TEST_F(OpenAIProviderTest, ChatWithHistory_EmptyMessages_ReturnsEmptyMessagesError) {
    std::vector<ChatMessage> messages;
    CString  response;
    AppError error;
    BOOL result = provider_.ChatWithHistory(messages, _T(""), response, error);
    EXPECT_EQ(result, FALSE);
    EXPECT_EQ(error.code, CString(_T("EMPTY_MESSAGES")));
}

// ============================================================
// GeminiProvider 테스트
// ============================================================
class GeminiProviderTest : public ::testing::Test {
protected:
    GeminiProvider provider_;
};

// TC-GEMINI-001: GetProviderName()은 "Gemini"를 반환한다
TEST_F(GeminiProviderTest, ProviderName_ReturnsGemini) {
    EXPECT_EQ(provider_.GetProviderName(), CString(_T("Gemini")));
}

// TC-GEMINI-002: GetDefaultModel()은 "gemini-2.5-flash"를 반환한다
TEST_F(GeminiProviderTest, DefaultModel_ReturnsGemini25Flash) {
    EXPECT_EQ(provider_.GetDefaultModel(), CString(_T("gemini-2.5-flash")));
}

// TC-GEMINI-003: SetApiKey 후 Chat 호출 시 크래시 없음
TEST_F(GeminiProviderTest, SetApiKeyAndChat_DoesNotCrash) {
    provider_.SetApiKey(_T("gemini-api-key-test"));
    CString  response;
    AppError error;
    EXPECT_NO_THROW(provider_.Chat(_T("system"), _T("hi"), _T(""), response, error));
}

// TC-GEMINI-004: API 키 없이 Chat 호출 시 FALSE 반환 및 오류 설정
TEST_F(GeminiProviderTest, ChatWithoutApiKey_ReturnsFalseWithError) {
    CString  response;
    AppError error;
    BOOL result = provider_.Chat(_T("system"), _T("hello"), _T(""), response, error);
    EXPECT_EQ(result, FALSE);
    EXPECT_TRUE(error.IsError())
        << "오류가 설정되지 않음. code=" << (LPCWSTR)error.code;
}

// TC-GEMINI-005: ChatWithHistory 빈 벡터 호출 시 EMPTY_MESSAGES 오류
TEST_F(GeminiProviderTest, ChatWithHistory_EmptyMessages_ReturnsEmptyMessagesError) {
    std::vector<ChatMessage> messages;
    CString  response;
    AppError error;
    BOOL result = provider_.ChatWithHistory(messages, _T(""), response, error);
    EXPECT_EQ(result, FALSE);
    EXPECT_EQ(error.code, CString(_T("EMPTY_MESSAGES")));
}

// ============================================================
// 인터페이스 다형성 테스트 — ILLMProvider 포인터를 통해 동작 검증
// ============================================================

// TC-POLY-001: ILLMProvider 포인터를 통해 각 프로바이더 이름을 올바르게 반환한다
TEST(LLMProviderPolymorphismTest, GetProviderName_ThroughInterface_ReturnsCorrectName) {
    // 각 프로바이더를 ILLMProvider 포인터로 접근한다
    std::unique_ptr<ILLMProvider> claude = std::make_unique<ClaudeProvider>();
    std::unique_ptr<ILLMProvider> openai = std::make_unique<OpenAIProvider>();
    std::unique_ptr<ILLMProvider> gemini = std::make_unique<GeminiProvider>();

    EXPECT_EQ(claude->GetProviderName(), CString(_T("Claude")));
    EXPECT_EQ(openai->GetProviderName(), CString(_T("OpenAI")));
    EXPECT_EQ(gemini->GetProviderName(), CString(_T("Gemini")));
}

// TC-POLY-002: 각 프로바이더의 기본 모델이 서로 다르다
TEST(LLMProviderPolymorphismTest, GetDefaultModel_ThroughInterface_ReturnsDifferentModels) {
    std::unique_ptr<ILLMProvider> claude = std::make_unique<ClaudeProvider>();
    std::unique_ptr<ILLMProvider> openai = std::make_unique<OpenAIProvider>();
    std::unique_ptr<ILLMProvider> gemini = std::make_unique<GeminiProvider>();

    CString claudeModel = claude->GetDefaultModel();
    CString openaiModel = openai->GetDefaultModel();
    CString geminiModel = gemini->GetDefaultModel();

    // 세 프로바이더의 기본 모델은 모두 달라야 한다
    EXPECT_NE(claudeModel, openaiModel);
    EXPECT_NE(openaiModel, geminiModel);
    EXPECT_NE(claudeModel, geminiModel);
}
