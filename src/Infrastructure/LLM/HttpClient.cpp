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
        msg.Format(_T("API 오류 (HTTP %ld): %s"),
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
