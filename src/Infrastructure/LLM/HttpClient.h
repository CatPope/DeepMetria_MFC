#pragma once
// HttpClient.h — 공통 HTTP POST 유틸리티 (libcurl 래퍼)

#include "../../Common/Types.h"
#include <string>
#include <vector>

// ============================================================
// HttpClient — LLM 프로바이더 공용 HTTP 전송 유틸리티
// ============================================================
class HttpClient {
public:
    // JSON POST 요청 실행. HTTP 200 반환 시 TRUE.
    // headers : 추가 헤더 목록 (예: "x-api-key: xxx", "Authorization: Bearer xxx")
    // timeoutSec : 요청 타임아웃 (초)
    static BOOL PostJson(const std::string&              url,
                         const std::string&              jsonBody,
                         const std::vector<std::string>& headers,
                         std::string&                    outBody,
                         AppError&                       outError,
                         int                             timeoutSec = 30);

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};
