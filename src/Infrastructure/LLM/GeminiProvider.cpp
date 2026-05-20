#include "stdafx.h"
#include "GeminiProvider.h"
#include "HttpClient.h"
#include "../../Common/StringUtils.h"

// ============================================================
// 상수
// ============================================================

const char* GeminiProvider::API_BASE_URL = "https://generativelanguage.googleapis.com/v1beta/models/";
const int   GeminiProvider::TIMEOUT_SEC  = 60;

// ============================================================
// 생성자 / 소멸자
// ============================================================

GeminiProvider::GeminiProvider() {}
GeminiProvider::~GeminiProvider() {}

// ============================================================
// BuildUrl — 모델명 + API 키로 URL 구성
// ============================================================

std::string GeminiProvider::BuildUrl(const std::string& model, const std::string& apiKey) {
    // https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={apiKey}
    return std::string(API_BASE_URL) + model + ":generateContent?key=" + apiKey;
}

// ============================================================
// Chat — system + user 동기 호출
// ============================================================

BOOL GeminiProvider::Chat(const CString& systemPrompt,
                           const CString& userMessage,
                           const CString& model,
                           CString&       outResponse,
                           AppError&      outError) {
    outError.Clear();

    std::string sSystem = StringUtils::ToUTF8(systemPrompt);
    std::string sUser   = StringUtils::ToUTF8(userMessage);
    std::string sModel  = model.IsEmpty()
                          ? "gemini-2.5-flash"
                          : StringUtils::ToUTF8(model);
    std::string sApiKey = StringUtils::ToUTF8(m_apiKey);

    // Gemini API 요청 JSON
    // {
    //   "systemInstruction": {"parts": [{"text": "..."}]},
    //   "contents": [{"role": "user", "parts": [{"text": "..."}]}]
    // }
    std::string body =
        "{\"systemInstruction\":{\"parts\":[{\"text\":\""
        + StringUtils::JsonEscape(sSystem) + "\"}]},"
        "\"contents\":[{\"role\":\"user\",\"parts\":[{\"text\":\""
        + StringUtils::JsonEscape(sUser) + "\"}]}]}";

    std::string url = BuildUrl(sModel, sApiKey);
    std::string responseBody;
    if (!PostRequest(url, body, responseBody, outError)) return FALSE;
    return ParseResponse(responseBody, outResponse, outError);
}

// ============================================================
// ChatWithHistory — 전체 메시지 이력
// ============================================================

BOOL GeminiProvider::ChatWithHistory(const std::vector<ChatMessage>& messages,
                                      const CString&                  model,
                                      CString&                        outResponse,
                                      AppError&                       outError) {
    outError.Clear();
    if (messages.empty()) {
        outError.Set(_T("EMPTY_MESSAGES"), _T("메시지가 없습니다."));
        return FALSE;
    }

    std::string sModel  = model.IsEmpty() ? "gemini-2.5-flash" : StringUtils::ToUTF8(model);
    std::string sApiKey = StringUtils::ToUTF8(m_apiKey);

    // system 분리, 나머지는 contents 배열
    std::string systemText;
    std::string contentsJson = "[";
    bool first = true;

    for (const auto& msg : messages) {
        std::string role = StringUtils::ToUTF8(msg.role);
        if (role == "system") {
            systemText = StringUtils::ToUTF8(msg.content);
            continue;
        }
        if (!first) contentsJson += ",";
        // Gemini는 "assistant" 대신 "model" 사용
        std::string geminiRole = (role == "assistant") ? "model" : role;
        contentsJson += "{\"role\":\"" + StringUtils::JsonEscape(geminiRole) + "\","
                        "\"parts\":[{\"text\":\""
                        + StringUtils::JsonEscape(StringUtils::ToUTF8(msg.content)) + "\"}]}";
        first = false;
    }
    contentsJson += "]";

    std::string body;
    if (!systemText.empty()) {
        body = "{\"systemInstruction\":{\"parts\":[{\"text\":\""
               + StringUtils::JsonEscape(systemText) + "\"}]},"
               "\"contents\":" + contentsJson + "}";
    } else {
        body = "{\"contents\":" + contentsJson + "}";
    }

    std::string url = BuildUrl(sModel, sApiKey);
    std::string responseBody;
    if (!PostRequest(url, body, responseBody, outError)) return FALSE;
    return ParseResponse(responseBody, outResponse, outError);
}

// ============================================================
// PostRequest — HttpClient 위임
// ============================================================

BOOL GeminiProvider::PostRequest(const std::string& url,
                                  const std::string& jsonBody,
                                  std::string&       outBody,
                                  AppError&          outError) {
    // Gemini API는 API 키를 URL 파라미터로 전달 (헤더 인증 불필요)
    std::vector<std::string> headers = {
        "Content-Type: application/json"
    };
    return HttpClient::PostJson(url, jsonBody, headers, outBody, outError, TIMEOUT_SEC);
}

// ============================================================
// ParseResponse — candidates[0].content.parts[0].text 추출
// ============================================================

BOOL GeminiProvider::ParseResponse(const std::string& jsonBody,
                                    CString&           outText,
                                    AppError&          outError) {
    // "candidates" 배열 탐색
    size_t candidatesPos = jsonBody.find("\"candidates\":");
    if (candidatesPos == std::string::npos) {
        // 에러 응답에서 message 추출
        size_t errPos = jsonBody.find("\"message\":\"");
        if (errPos != std::string::npos) {
            errPos += 11; // strlen("\"message\":\"")
            size_t errEnd = jsonBody.find("\"", errPos);
            std::string errMsg = (errEnd != std::string::npos)
                ? jsonBody.substr(errPos, errEnd - errPos)
                : jsonBody;
            outError.Set(_T("API_ERROR"), StringUtils::FromUTF8(errMsg));
        } else {
            outError.Set(_T("PARSE_ERROR"), StringUtils::FromUTF8(jsonBody.substr(0, 500)));
        }
        return FALSE;
    }

    // "parts" → "text" 탐색
    size_t partsPos = jsonBody.find("\"parts\":", candidatesPos);
    if (partsPos == std::string::npos) {
        outError.Set(_T("PARSE_ERROR"), _T("응답에서 parts를 찾을 수 없습니다."));
        return FALSE;
    }

    // "text":"..." 값 추출
    size_t textKeyPos = jsonBody.find("\"text\":", partsPos);
    if (textKeyPos == std::string::npos) {
        outError.Set(_T("PARSE_ERROR"), _T("응답에서 text를 찾을 수 없습니다."));
        return FALSE;
    }

    // "text": 다음의 값 추출 (따옴표로 감싸진 문자열)
    size_t valStart = jsonBody.find("\"", textKeyPos + 6); // "text": 이후
    if (valStart == std::string::npos) {
        outError.Set(_T("EMPTY_RESPONSE"), _T("LLM이 빈 응답을 반환했습니다."));
        return FALSE;
    }
    valStart++; // 시작 따옴표 건너뜀

    // 이스케이프 문자 처리하며 값 추출
    std::string text;
    size_t pos = valStart;
    while (pos < jsonBody.size() && jsonBody[pos] != '"') {
        if (jsonBody[pos] == '\\' && pos + 1 < jsonBody.size()) {
            char esc = jsonBody[pos + 1];
            switch (esc) {
            case '"':  text += '"';  break;
            case '\\': text += '\\'; break;
            case 'n':  text += '\n'; break;
            case 'r':  text += '\r'; break;
            case 't':  text += '\t'; break;
            default:   text += esc;  break;
            }
            pos += 2;
        } else {
            text += jsonBody[pos++];
        }
    }

    if (text.empty()) {
        outError.Set(_T("EMPTY_RESPONSE"), _T("LLM이 빈 응답을 반환했습니다."));
        return FALSE;
    }

    outText = StringUtils::FromUTF8(text);
    return TRUE;
}
