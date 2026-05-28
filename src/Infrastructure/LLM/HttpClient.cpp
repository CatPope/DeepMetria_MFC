#include "stdafx.h"
#include "HttpClient.h"
#include <curl/curl.h>
#include "../../Common/StringUtils.h"

// ============================================================
// PostJson — libcurl HTTP POST (공통 구현)
// ============================================================

BOOL HttpClient::PostJson(const std::string&              url,
                           const std::string&              jsonBody,
                           const std::vector<std::string>& headers,
                           std::string&                    outBody,
                           AppError&                       outError,
                           int                             timeoutSec) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        outError.Set(_T("CURL_INIT_FAILED"), _T("HTTP 클라이언트 초기화에 실패했습니다."));
        return FALSE;
    }

    outBody.clear();

    struct curl_slist* curlHeaders = nullptr;
    for (const auto& h : headers) {
        curlHeaders = curl_slist_append(curlHeaders, h.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     curlHeaders);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,  (long)jsonBody.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &outBody);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        (long)timeoutSec);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(curlHeaders);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        CString msg;
        msg.Format(_T("HTTP 요청 실패: %s"),
                   StringUtils::FromUTF8(curl_easy_strerror(res)).GetString());
        outError.Set(_T("HTTP_REQUEST_FAILED"), msg);
        return FALSE;
    }

    if (httpCode != 200) {
        CString msg;
        if (httpCode == 429)
            msg = _T("API 요청 한도를 초과했습니다. 잠시 후 다시 시도해주세요.");
        else if (httpCode == 401 || httpCode == 403)
            msg = _T("API 인증에 실패했습니다. API 키를 확인해주세요.");
        else if (httpCode >= 500)
            msg.Format(_T("API 서버 오류가 발생했습니다. (HTTP %ld)"), httpCode);
        else
            msg.Format(_T("API 오류가 발생했습니다. (HTTP %ld)"), httpCode);

        TRACE(_T("[HttpClient] HTTP %ld response: %s\n"),
              httpCode, StringUtils::FromUTF8(outBody).GetString());
        outError.Set(_T("API_ERROR"), msg);
        return FALSE;
    }

    return TRUE;
}

// ============================================================
// WriteCallback — libcurl 쓰기 콜백
// ============================================================

size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* buf = static_cast<std::string*>(userp);
    buf->append(static_cast<char*>(contents), total);
    return total;
}

// ============================================================
// PostJsonSSE — SSE 스트리밍 POST 요청
// ============================================================

BOOL HttpClient::PostJsonSSE(const std::string&              url,
                              const std::string&              jsonBody,
                              const std::vector<std::string>& headers,
                              SSECallback                     callback,
                              AppError&                       outError,
                              int                             timeoutSec) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        outError.Set(_T("CURL_INIT_FAILED"), _T("HTTP 클라이언트 초기화에 실패했습니다."));
        return FALSE;
    }

    SSEContext ctx;
    ctx.callback = callback;

    struct curl_slist* curlHeaders = nullptr;
    for (const auto& h : headers) {
        curlHeaders = curl_slist_append(curlHeaders, h.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     curlHeaders);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,  (long)jsonBody.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  SSEWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &ctx);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        (long)timeoutSec);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(curlHeaders);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        CString msg;
        msg.Format(_T("HTTP SSE 요청 실패: %s"),
                   StringUtils::FromUTF8(curl_easy_strerror(res)).GetString());
        outError.Set(_T("HTTP_SSE_FAILED"), msg);
        return FALSE;
    }

    if (httpCode != 200) {
        CString msg;
        msg.Format(_T("API SSE 오류 (HTTP %ld)"), httpCode);
        outError.Set(_T("API_SSE_ERROR"), msg);
        return FALSE;
    }

    return TRUE;
}

// ============================================================
// SSEWriteCallback — SSE 청크 처리 콜백
// ============================================================

size_t HttpClient::SSEWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* ctx = static_cast<SSEContext*>(userp);

    // 버퍼에 수신 데이터 추가
    ctx->buffer.append(static_cast<char*>(contents), total);

    // 줄 단위로 처리
    std::string& buf = ctx->buffer;
    std::string::size_type pos = 0;

    while (true) {
        std::string::size_type nl = buf.find('\n', pos);
        if (nl == std::string::npos)
            break;

        std::string line = buf.substr(pos, nl - pos);
        // 줄 끝 '\r' 제거
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        pos = nl + 1;

        if (line.empty()) {
            // 빈 줄: 이벤트 경계 (현재 구현에서는 즉시 처리)
            continue;
        }

        // "data: " 접두사 처리
        if (line.substr(0, 6) == "data: ") {
            std::string data = line.substr(6);

            // 스트림 종료 신호
            if (data == "[DONE]")
                break;

            if (ctx->callback)
                ctx->callback("data", data);
        }
        else if (line.substr(0, 7) == "event: ") {
            std::string eventType = line.substr(7);
            if (ctx->callback)
                ctx->callback("event", eventType);
        }
        else if (line.substr(0, 4) == "id: ") {
            std::string id = line.substr(4);
            if (ctx->callback)
                ctx->callback("id", id);
        }
        else if (line.substr(0, 7) == "retry: ") {
            std::string retry = line.substr(7);
            if (ctx->callback)
                ctx->callback("retry", retry);
        }
    }

    // 처리된 부분 제거, 미처리 부분 유지
    if (pos > 0)
        buf.erase(0, pos);

    return total;
}
