#include "stdafx.h"
#include "ClaudeProvider.h"
#include <curl/curl.h>
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
// PostRequest — libcurl HTTP POST
// ============================================================

BOOL ClaudeProvider::PostRequest(const std::string& url,
                                  const std::string& jsonBody,
                                  const std::string& apiKey,
                                  std::string&       outBody,
                                  AppError&          outError) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        outError.Set(_T("CURL_INIT_FAILED"), _T("HTTP 클라이언트 초기화에 실패했습니다."));
        return FALSE;
    }

    outBody.clear();

    // 헤더 설정
    struct curl_slist* headers = nullptr;
    std::string authHeader     = "x-api-key: " + apiKey;
    std::string versionHeader  = "anthropic-version: 2023-06-01";
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, authHeader.c_str());
    headers = curl_slist_append(headers, versionHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,  (long)jsonBody.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &outBody);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        (long)TIMEOUT_SEC);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        CString msg;
        msg.Format(_T("HTTP 요청 실패: %s"), StringUtils::FromUTF8(curl_easy_strerror(res)).GetString());
        outError.Set(_T("HTTP_REQUEST_FAILED"), msg);
        return FALSE;
    }

    if (httpCode != 200) {
        CString msg;
        msg.Format(_T("Claude API 오류 (HTTP %ld): %s"),
                   httpCode, StringUtils::FromUTF8(outBody).GetString());
        outError.Set(_T("API_ERROR"), msg);
        return FALSE;
    }

    return TRUE;
}

// ============================================================
// ParseResponse — content[0].text 추출
// ============================================================

BOOL ClaudeProvider::ParseResponse(const std::string& jsonBody,
                                    CString&           outText,
                                    AppError&          outError) {
    // 경량 문자열 파싱: "content":[{"type":"text","text":"..."}]
    // nlohmann/json 사용 시 더 견고하지만, 의존성 최소화를 위해 수동 파싱
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

// ============================================================
// libcurl 쓰기 콜백
// ============================================================

size_t ClaudeProvider::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* buf = static_cast<std::string*>(userp);
    buf->append(static_cast<char*>(contents), total);
    return total;
}

// ============================================================
// 문자열 변환 헬퍼
// ============================================================

std::string ClaudeProvider::StringUtils::ToUTF8(const CString& str) {
    if (str.IsEmpty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, str, -1, &result[0], len, nullptr, nullptr);
    if (!result.empty() && result.back() == '\0') result.pop_back();
    return result;
}

CString ClaudeProvider::StringUtils::FromUTF8(const std::string& utf8) {
    if (utf8.empty()) return _T("");
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    CString result;
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.GetBufferSetLength(wlen), wlen);
    result.ReleaseBuffer();
    return result;
}

std::string ClaudeProvider::StringUtils::JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:   out += c;      break;
        }
    }
    return out;
}
