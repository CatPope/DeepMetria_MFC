// Infra/HttpClient.cpp - WinHTTP 기반 POST (TLS 1.2+)
#include "stdafx.h"
#include "HttpClient.h"
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

namespace deepmetria {

HttpResponse HttpClient::Post(const std::wstring& host,
                              const std::wstring& path,
                              const std::wstring& headers,
                              const std::string&  utf8Body,
                              int timeoutMs)
{
    HttpResponse resp;

    HINTERNET hSession = WinHttpOpen(L"DeepMetria/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { resp.error = L"WinHttpOpen 실패"; return resp; }

    WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    // TLS 1.2 이상 강제
    DWORD secProto = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | 0x00002000 /*TLS1_3*/;
    WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secProto, sizeof(secProto));

    HINTERNET hConn = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConn) { resp.error = L"WinHttpConnect 실패"; WinHttpCloseHandle(hSession); return resp; }

    HINTERNET hReq = WinHttpOpenRequest(hConn, L"POST", path.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hReq)
    {
        resp.error = L"WinHttpOpenRequest 실패";
        WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSession);
        return resp;
    }

    BOOL sent = WinHttpSendRequest(hReq,
        headers.c_str(), static_cast<DWORD>(-1),
        const_cast<char*>(utf8Body.data()), static_cast<DWORD>(utf8Body.size()),
        static_cast<DWORD>(utf8Body.size()), 0);
    if (!sent || !WinHttpReceiveResponse(hReq, nullptr))
    {
        DWORD ec = GetLastError();
        wchar_t buf[64];
        swprintf_s(buf, L"전송 실패 (ec=%lu)", ec);
        resp.error = buf;
        WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSession);
        return resp;
    }

    // 상태
    DWORD status = 0, statusSize = sizeof(status);
    WinHttpQueryHeaders(hReq,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
    resp.status = static_cast<int>(status);

    // 본문 누적 읽기
    DWORD avail = 0;
    while (WinHttpQueryDataAvailable(hReq, &avail) && avail > 0)
    {
        std::string chunk(avail, '\0');
        DWORD read = 0;
        if (!WinHttpReadData(hReq, chunk.data(), avail, &read)) break;
        chunk.resize(read);
        resp.body.append(chunk);
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSession);
    return resp;
}

} // namespace deepmetria
