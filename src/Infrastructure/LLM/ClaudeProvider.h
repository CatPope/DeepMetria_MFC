#pragma once
// ClaudeProvider.h — Anthropic Claude API 프로바이더
// libcurl로 https://api.anthropic.com/v1/messages POST

#include "LLMProvider.h"
#include <string>
#include <vector>

// ============================================================
// ClaudeProvider — ILLMProvider 구현
// ============================================================
class ClaudeProvider : public ILLMProvider {
public:
    ClaudeProvider();
    ~ClaudeProvider() override;

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

    CString GetProviderName() const override { return _T("Claude"); }
    CString GetDefaultModel() const override { return _T("claude-opus-4-5"); }
    void    SetApiKey(const CString& apiKey) override { m_apiKey = apiKey; }
    bool    HasApiKey() const override { return !m_apiKey.IsEmpty(); }

private:
    // libcurl POST 요청 실행 (HttpClient 위임)
    BOOL PostRequest(const std::string& url,
                     const std::string& jsonBody,
                     const std::string& apiKey,
                     std::string&       outBody,
                     AppError&          outError);

    // 응답 JSON에서 content[0].text 추출
    BOOL ParseResponse(const std::string& jsonBody,
                       CString&           outText,
                       AppError&          outError);

    CString m_apiKey;
    static const char* API_URL;     // "https://api.anthropic.com/v1/messages"
    static const int   TIMEOUT_SEC; // 30
};
