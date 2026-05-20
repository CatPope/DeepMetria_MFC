#include "stdafx.h"
#include "OpenAIProvider.h"
#include "HttpClient.h"
#include "../../Common/StringUtils.h"

// ============================================================
// 상수
// ============================================================

const char* OpenAIProvider::API_URL     = "https://api.openai.com/v1/chat/completions";
const int   OpenAIProvider::TIMEOUT_SEC = 30;

// ============================================================
// 생성자 / 소멸자
// ============================================================

OpenAIProvider::OpenAIProvider() {}

OpenAIProvider::~OpenAIProvider() {}

// ============================================================
// Chat — 단순 system+user 동기 호출
// ============================================================

BOOL OpenAIProvider::Chat(const CString& systemPrompt,
                           const CString& userMessage,
                           const CString& model,
                           CString&       outResponse,
                           AppError&      outError) {
    outError.Clear();

    std::string sSystem = StringUtils::ToUTF8(systemPrompt);
    std::string sUser   = StringUtils::ToUTF8(userMessage);
    std::string sModel  = model.IsEmpty() ? "gpt-4o" : StringUtils::ToUTF8(model);
    std::string sApiKey = StringUtils::ToUTF8(m_apiKey);

    // 요청 JSON: {"model":"...","messages":[{"role":"system","content":"..."},{"role":"user","content":"..."}]}
    std::string body =
        "{\"model\":\"" + StringUtils::JsonEscape(sModel) + "\","
        "\"messages\":["
        "{\"role\":\"system\",\"content\":\"" + StringUtils::JsonEscape(sSystem) + "\"},"
        "{\"role\":\"user\",\"content\":\"" + StringUtils::JsonEscape(sUser) + "\"}"
        "]}";

    std::string responseBody;
    if (!PostRequest(API_URL, body, sApiKey, responseBody, outError)) return FALSE;
    return ParseResponse(responseBody, outResponse, outError);
}

// ============================================================
// ChatWithHistory — 전체 메시지 이력
// ============================================================

BOOL OpenAIProvider::ChatWithHistory(const std::vector<ChatMessage>& messages,
                                      const CString&                  model,
                                      CString&                        outResponse,
                                      AppError&                       outError) {
    outError.Clear();
    if (messages.empty()) {
        outError.Set(_T("EMPTY_MESSAGES"), _T("메시지가 없습니다."));
        return FALSE;
    }

    std::string sModel  = model.IsEmpty() ? "gpt-4o" : StringUtils::ToUTF8(model);
    std::string sApiKey = StringUtils::ToUTF8(m_apiKey);

    std::string messagesJson = "[";
    bool first = true;
    for (const auto& msg : messages) {
        if (!first) messagesJson += ",";
        std::string role    = StringUtils::ToUTF8(msg.role);
        std::string content = StringUtils::ToUTF8(msg.content);
        messagesJson += "{\"role\":\"" + StringUtils::JsonEscape(role) + "\","
                        "\"content\":\"" + StringUtils::JsonEscape(content) + "\"}";
        first = false;
    }
    messagesJson += "]";

    std::string body =
        "{\"model\":\"" + StringUtils::JsonEscape(sModel) + "\","
        "\"messages\":" + messagesJson + "}";

    std::string responseBody;
    if (!PostRequest(API_URL, body, sApiKey, responseBody, outError)) return FALSE;
    return ParseResponse(responseBody, outResponse, outError);
}

// ============================================================
// PostRequest — HttpClient 위임
// ============================================================

BOOL OpenAIProvider::PostRequest(const std::string& url,
                                  const std::string& jsonBody,
                                  const std::string& apiKey,
                                  std::string&       outBody,
                                  AppError&          outError) {
    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "Authorization: Bearer " + apiKey
    };
    return HttpClient::PostJson(url, jsonBody, headers, outBody, outError, TIMEOUT_SEC);
}

// ============================================================
// ParseResponse — choices[0].message.content 추출
// ============================================================

BOOL OpenAIProvider::ParseResponse(const std::string& jsonBody,
                                    CString&           outText,
                                    AppError&          outError) {
    // "choices" 배열 탐색 → message.content 추출
    auto findStr = [&](const std::string& key, size_t from) -> std::string {
        std::string search = "\"" + key + "\":\"";
        size_t pos = jsonBody.find(search, from);
        if (pos == std::string::npos) return "";
        pos += search.size();
        std::string result;
        while (pos < jsonBody.size() && jsonBody[pos] != '"') {
            if (jsonBody[pos] == '\\' && pos + 1 < jsonBody.size()) {
                char esc = jsonBody[pos + 1];
                switch (esc) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += esc;  break;
                }
                pos += 2;
            } else {
                result += jsonBody[pos++];
            }
        }
        return result;
    };

    // "choices" 위치 탐색
    size_t choicesPos = jsonBody.find("\"choices\":");
    if (choicesPos == std::string::npos) {
        std::string errMsg = findStr("message", 0);
        if (errMsg.empty()) errMsg = jsonBody;
        outError.Set(_T("PARSE_ERROR"), StringUtils::FromUTF8(errMsg));
        return FALSE;
    }

    // "message" 블록 탐색 후 "content" 추출
    size_t msgPos = jsonBody.find("\"message\":", choicesPos);
    if (msgPos == std::string::npos) {
        outError.Set(_T("PARSE_ERROR"), _T("응답에서 message를 찾을 수 없습니다."));
        return FALSE;
    }

    std::string content = findStr("content", msgPos);
    if (content.empty()) {
        outError.Set(_T("EMPTY_RESPONSE"), _T("LLM이 빈 응답을 반환했습니다."));
        return FALSE;
    }

    outText = StringUtils::FromUTF8(content);
    return TRUE;
}
