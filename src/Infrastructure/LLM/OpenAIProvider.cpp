#include "stdafx.h"
#include "OpenAIProvider.h"
#include <curl/curl.h>

// ============================================================
// 상수
// ============================================================

const char* OpenAIProvider::API_URL     = "https://api.openai.com/v1/chat/completions";
const int   OpenAIProvider::TIMEOUT_SEC = 30;

// ============================================================
// 생성자 / 소멸자
// ============================================================

OpenAIProvider::OpenAIProvider() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

OpenAIProvider::~OpenAIProvider() {
    curl_global_cleanup();
}

// ============================================================
// Chat — 단순 system+user 동기 호출
// ============================================================

BOOL OpenAIProvider::Chat(const CString& systemPrompt,
                           const CString& userMessage,
                           const CString& model,
                           CString&       outResponse,
                           AppError&      outError) {
    outError.Clear();

    std::string sSystem = CStringToUTF8(systemPrompt);
    std::string sUser   = CStringToUTF8(userMessage);
    std::string sModel  = model.IsEmpty() ? "gpt-4o" : CStringToUTF8(model);
    std::string sApiKey = CStringToUTF8(m_apiKey);

    // 요청 JSON: {"model":"...","messages":[{"role":"system","content":"..."},{"role":"user","content":"..."}]}
    std::string body =
        "{\"model\":\"" + JsonEscape(sModel) + "\","
        "\"messages\":["
        "{\"role\":\"system\",\"content\":\"" + JsonEscape(sSystem) + "\"},"
        "{\"role\":\"user\",\"content\":\"" + JsonEscape(sUser) + "\"}"
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

    std::string sModel  = model.IsEmpty() ? "gpt-4o" : CStringToUTF8(model);
    std::string sApiKey = CStringToUTF8(m_apiKey);

    std::string messagesJson = "[";
    bool first = true;
    for (const auto& msg : messages) {
        if (!first) messagesJson += ",";
        std::string role    = CStringToUTF8(msg.role);
        std::string content = CStringToUTF8(msg.content);
        messagesJson += "{\"role\":\"" + JsonEscape(role) + "\","
                        "\"content\":\"" + JsonEscape(content) + "\"}";
        first = false;
    }
    messagesJson += "]";

    std::string body =
        "{\"model\":\"" + JsonEscape(sModel) + "\","
        "\"messages\":" + messagesJson + "}";

    std::string responseBody;
    if (!PostRequest(API_URL, body, sApiKey, responseBody, outError)) return FALSE;
    return ParseResponse(responseBody, outResponse, outError);
}

// ============================================================
// PostRequest — libcurl HTTP POST
// ============================================================

BOOL OpenAIProvider::PostRequest(const std::string& url,
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

    struct curl_slist* headers = nullptr;
    std::string authHeader = "Authorization: Bearer " + apiKey;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, authHeader.c_str());

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
        msg.Format(_T("HTTP 요청 실패: %s"), UTF8ToCString(curl_easy_strerror(res)).GetString());
        outError.Set(_T("HTTP_REQUEST_FAILED"), msg);
        return FALSE;
    }

    if (httpCode != 200) {
        CString msg;
        msg.Format(_T("OpenAI API 오류 (HTTP %ld): %s"),
                   httpCode, UTF8ToCString(outBody).GetString());
        outError.Set(_T("API_ERROR"), msg);
        return FALSE;
    }

    return TRUE;
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
        outError.Set(_T("PARSE_ERROR"), UTF8ToCString(errMsg));
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

    outText = UTF8ToCString(content);
    return TRUE;
}

// ============================================================
// libcurl 쓰기 콜백
// ============================================================

size_t OpenAIProvider::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* buf = static_cast<std::string*>(userp);
    buf->append(static_cast<char*>(contents), total);
    return total;
}

// ============================================================
// 문자열 변환 헬퍼
// ============================================================

std::string OpenAIProvider::CStringToUTF8(const CString& str) {
    if (str.IsEmpty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, str, -1, &result[0], len, nullptr, nullptr);
    if (!result.empty() && result.back() == '\0') result.pop_back();
    return result;
}

CString OpenAIProvider::UTF8ToCString(const std::string& utf8) {
    if (utf8.empty()) return _T("");
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    CString result;
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.GetBufferSetLength(wlen), wlen);
    result.ReleaseBuffer();
    return result;
}

std::string OpenAIProvider::JsonEscape(const std::string& s) {
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
