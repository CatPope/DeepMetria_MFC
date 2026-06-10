// Infra/HttpClient.h - WinHTTP 래퍼
#pragma once
#include <string>
#include <vector>
#include <optional>

namespace deepmetria {

struct HttpResponse
{
    int           status = 0;
    std::string   body;
    std::wstring  error;
    bool          ok() const { return status >= 200 && status < 300; }
};

class HttpClient
{
public:
    // host: "api.anthropic.com", path: "/v1/messages", body: utf-8 JSON
    // headers: 한 줄에 한 헤더 (예: L"Content-Type: application/json\r\n")
    static HttpResponse Post(const std::wstring& host,
                             const std::wstring& path,
                             const std::wstring& headers,
                             const std::string&  utf8Body,
                             int timeoutMs = 60000);
};

} // namespace deepmetria
