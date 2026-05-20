#include "stdafx.h"
#include "ClaudeProvider.h"
#include "HttpClient.h"
#include "../../Common/StringUtils.h"

// ============================================================
// 상수
// ============================================================

const char* ClaudeProvider::API_URL     = "https://api.anthropic.com/v1/messages";
const int   ClaudeProvider::TIMEOUT_SEC = 30;

// ============================================================
// 생성자 / 소멸자
// ============================================================

ClaudeProvider::ClaudeProvider() {}

ClaudeProvider::~ClaudeProvider() {}

// ============================================================
// Chat — 단순 system+user 동기 호출
// ============================================================

BOOL ClaudeProvider::Chat(const CString& systemPrompt,
                           const CString& userMessage,
                           const CString& model,
                           CString&       outResponse,
                           AppError&      outError) {
    outError.Clear();

    std::string sSystem = StringUtils::ToUTF8(systemPrompt);
    std::string sUser   = StringUtils::ToUTF8(userMessage);
    std::string sModel  = model.IsEmpty()
                          ? "claude-opus-4-5"
                          : StringUtils::ToUTF8(model);
    std::string sApiKey = StringUtils::ToUTF8(m_apiKey);

    // 요청 JSON 구성
    // {"model":"...","max_tokens":4096,"system":"...","messages":[{"role":"user","content":"..."}]}
    std::string body =
        "{\"model\":\"" + StringUtils::JsonEscape(sModel) + "\","
        "\"max_tokens\":4096,"
        "\"system\":\"" + StringUtils::JsonEscape(sSystem) + "\","
        "\"messages\":[{\"role\":\"user\",\"content\":\"" + StringUtils::JsonEscape(sUser) + "\"}]}";

    std::string responseBody;
    if (!PostRequest(API_URL, body, sApiKey, responseBody, outError)) return FALSE;
    return ParseResponse(responseBody, outResponse, outError);
}

// ============================================================
// ChatWithHistory — 전체 메시지 이력
// ============================================================

BOOL ClaudeProvider::ChatWithHistory(const std::vector<ChatMessage>& messages,
                                      const CString&                  model,
                                      CString&                        outResponse,
                                      AppError&                       outError) {
    outError.Clear();
    if (messages.empty()) {
        outError.Set(_T("EMPTY_MESSAGES"), _T("메시지가 없습니다."));
        return FALSE;
    }

    std::string sModel  = model.IsEmpty() ? "claude-opus-4-5" : StringUtils::ToUTF8(model);
    std::string sApiKey = StringUtils::ToUTF8(m_apiKey);

    // system 메시지 분리, 나머지는 messages 배열로
    std::string systemText;
    std::string messagesJson = "[";
    bool first = true;

    for (const auto& msg : messages) {
        std::string role = StringUtils::ToUTF8(msg.role);
        if (role == "system") {
            systemText = StringUtils::ToUTF8(msg.content);
            continue;
        }
        if (!first) messagesJson += ",";
        messagesJson += "{\"role\":\"" + StringUtils::JsonEscape(role) + "\","
                        "\"content\":\"" + StringUtils::JsonEscape(StringUtils::ToUTF8(msg.content)) + "\"}";
        first = false;
    }
    messagesJson += "]";

    std::string body =
        "{\"model\":\"" + StringUtils::JsonEscape(sModel) + "\","
        "\"max_tokens\":4096,"
        "\"system\":\"" + StringUtils::JsonEscape(systemText) + "\","
        "\"messages\":" + messagesJson + "}";

    std::string responseBody;
    if (!PostRequest(API_URL, body, sApiKey, responseBody, outError)) return FALSE;
    return ParseResponse(responseBody, outResponse, outError);
}

// ============================================================
// PostRequest — HttpClient 위임
// ============================================================

BOOL ClaudeProvider::PostRequest(const std::string& url,
                                  const std::string& jsonBody,
                                  const std::string& apiKey,
                                  std::string&       outBody,
                                  AppError&          outError) {
    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "x-api-key: " + apiKey,
        "anthropic-version: 2023-06-01"
    };
    return HttpClient::PostJson(url, jsonBody, headers, outBody, outError, TIMEOUT_SEC);
}

// ============================================================
// ParseResponse — content[0].text 추출
// ============================================================

BOOL ClaudeProvider::ParseResponse(const std::string& jsonBody,
                                    CString&           outText,
                                    AppError&          outError) {
    // 경량 문자열 파싱: "content":[{"type":"text","text":"..."}]
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

    // "content" 배열 위치 탐색
    size_t contentPos = jsonBody.find("\"content\":");
    if (contentPos == std::string::npos) {
        // 에러 응답 파싱 시도
        std::string errMsg = findStr("message", 0);
        if (errMsg.empty()) errMsg = jsonBody;
        outError.Set(_T("PARSE_ERROR"), StringUtils::FromUTF8(errMsg));
        return FALSE;
    }

    // type="text" 블록의 text 값 추출
    size_t typePos = jsonBody.find("\"type\":\"text\"", contentPos);
    if (typePos == std::string::npos) {
        outError.Set(_T("PARSE_ERROR"), _T("응답에서 텍스트 콘텐츠를 찾을 수 없습니다."));
        return FALSE;
    }

    std::string text = findStr("text", typePos);
    if (text.empty()) {
        outError.Set(_T("EMPTY_RESPONSE"), _T("LLM이 빈 응답을 반환했습니다."));
        return FALSE;
    }

    outText = StringUtils::FromUTF8(text);
    return TRUE;
}
