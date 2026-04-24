#pragma once
// OpenAIProvider.h — OpenAI ChatCompletion API 프로바이더
// libcurl로 https://api.openai.com/v1/chat/completions POST
// nlohmann/json으로 요청/응답 JSON 파싱
// DetailedSpec §4.4 참조

#include "LLMProvider.h"

// ============================================================
// OpenAIProvider — ILLMProvider 구현
// ============================================================
class OpenAIProvider : public ILLMProvider {
public:
    OpenAIProvider();
    ~OpenAIProvider() override;

    // ILLMProvider 구현
    BOOL Chat(const CString&               systemPrompt,
              const CString&               userMessage,
              const CString&               model,
              CString&                     outResponse,
              AppError&                    outError) override;

    BOOL ChatWithHistory(const std::vector<ChatMessage>& messages,
                         const CString&                  model,
                         CString&                        outResponse,
                         AppError&                       outError) override;

    CString GetProviderName() const override { return _T("OpenAI"); }
    CString GetDefaultModel() const override { return _T("gpt-4o"); }
    void    SetApiKey(const CString& apiKey) override { m_apiKey = apiKey; }

private:
    // libcurl POST 요청 실행
    BOOL PostRequest(const std::string& url,
                     const std::string& jsonBody,
                     const std::string& apiKey,
                     std::string&       outBody,
                     AppError&          outError);

    // 응답 JSON에서 choices[0].message.content 추출
    BOOL ParseResponse(const std::string& jsonBody,
                       CString&           outText,
                       AppError&          outError);

    // CString → UTF-8 std::string 변환
    static std::string CStringToUTF8(const CString& str);

    // UTF-8 std::string → CString 변환
    static CString UTF8ToCString(const std::string& utf8);

    // JSON 문자열 이스케이프
    static std::string JsonEscape(const std::string& s);

    // libcurl 쓰기 콜백
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    CString m_apiKey;
    static const char* API_URL;     // "https://api.openai.com/v1/chat/completions"
    static const int   TIMEOUT_SEC; // 30
};
