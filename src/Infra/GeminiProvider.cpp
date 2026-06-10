// Infra/GeminiProvider.cpp - Gemini API 어댑터 구현
#include "stdafx.h"
#include "GeminiProvider.h"
#include "HttpClient.h"
#include <sstream>

namespace deepmetria {

namespace {

std::string WToU8(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring U8ToW(const std::string& s)
{
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

std::wstring ExtractJsonString(const std::wstring& json, const std::wstring& key)
{
    std::wstring needle = L"\"" + key + L"\"";
    auto p = json.find(needle);
    if (p == std::wstring::npos) return {};
    auto colon = json.find(L':', p);
    if (colon == std::wstring::npos) return {};
    auto q1 = json.find(L'"', colon);
    if (q1 == std::wstring::npos) return {};
    auto q2 = json.find(L'"', q1 + 1);
    if (q2 == std::wstring::npos) return {};
    return json.substr(q1 + 1, q2 - q1 - 1);
}

} // namespace

LLMRouter::PlanResponse GeminiProvider::Plan(const LLMRouter::PlanRequest& req,
                                              const std::wstring& apiKey,
                                              const std::wstring& model) const
{
    LLMRouter::PlanResponse r;
    std::ostringstream body;
    body << "{\"contents\":[{\"parts\":[{\"text\":\""
         << WToU8(req.question)
         << " 응답은 다음 JSON: {\\\"toolName\\\":\\\"...\\\",\\\"params\\\":{...},\\\"title\\\":\\\"자연스러운 한국어 제목\\\",\\\"reasoning\\\":\\\"간단한 사고\\\"}\"}]}]}";

    std::wstring path = L"/v1beta/models/" +
        (model.empty() ? std::wstring(L"gemini-2.5-flash") : model) +
        L":generateContent?key=" + apiKey;

    std::wstring headers = L"Content-Type: application/json\r\n";
    auto resp = HttpClient::Post(L"generativelanguage.googleapis.com", path, headers, body.str());
    if (!resp.ok()) { r.ok = false; r.error = L"Gemini 호출 실패"; return r; }
    std::wstring w = U8ToW(resp.body);
    r.toolName  = ExtractJsonString(w, L"toolName");
    r.title     = ExtractJsonString(w, L"title");
    auto reason = ExtractJsonString(w, L"reasoning");
    r.reasoning = reason.empty() ? L"Gemini 응답 파싱" : reason;
    r.ok = !r.toolName.empty();
    return r;
}

} // namespace deepmetria
