#pragma once
// HttpClient.h — 공통 HTTP POST 유틸리티 (libcurl 래퍼)

#include "../../Common/Types.h"
#include <string>
#include <vector>
#include <functional>

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

    // SSE 이벤트 콜백 타입
    // eventType: "data", "event", "id", "retry" 등
    // data: 이벤트 데이터 문자열
    using SSECallback = std::function<void(const std::string& eventType, const std::string& data)>;

    // SSE 스트리밍 POST 요청 실행
    // callback: 각 SSE 이벤트 수신 시 호출되는 콜백
    // 연결이 유지되며 서버가 스트림을 종료하거나 타임아웃 시 반환
    static BOOL PostJsonSSE(const std::string&              url,
                            const std::string&              jsonBody,
                            const std::vector<std::string>& headers,
                            SSECallback                     callback,
                            AppError&                       outError,
                            int                             timeoutSec = 120);

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t SSEWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    // SSE 스트리밍 컨텍스트
    struct SSEContext {
        SSECallback callback;
        std::string buffer;  // 부분 라인 버퍼
    };
};
