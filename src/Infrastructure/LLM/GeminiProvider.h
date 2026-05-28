#pragma once
// GeminiProvider.h — Google Gemini API 프로바이더
// libcurl로 https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent POST

#include "LLMProvider.h"
#include <string>
#include <vector>

// ============================================================
// GeminiProvider — ILLMProvider 구현
// ============================================================
class GeminiProvider : public ILLMProvider {
public:
    GeminiProvider();
    ~GeminiProvider() override;

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

    CString GetProviderName() const override { return _T("Gemini"); }
    CString GetDefaultModel() const override { return _T("gemini-2.5-flash"); }
    void    SetApiKey(const CString& apiKey) override { m_apiKey = apiKey; }
    bool    HasApiKey() const override { return !m_apiKey.IsEmpty(); }

private:
    // API URL 구성 (모델명 + API 키 포함)
    std::string BuildUrl(const std::string& model, const std::string& apiKey);

    // libcurl POST 요청 실행 (HttpClient 위임)
    BOOL PostRequest(const std::string& url,
                     const std::string& jsonBody,
                     std::string&       outBody,
                     AppError&          outError);

    // 응답 JSON에서 candidates[0].content.parts[0].text 추출
    BOOL ParseResponse(const std::string& jsonBody,
                       CString&           outText,
                       AppError&          outError);

    CString m_apiKey;
    static const char* API_BASE_URL;
    static const int   TIMEOUT_SEC;
};
