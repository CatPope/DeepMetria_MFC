// Infra/ClaudeProvider.cpp - Claude API 어댑터 구현
#include "stdafx.h"
#include "ClaudeProvider.h"
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

LLMRouter::PlanResponse ClaudeProvider::Plan(const LLMRouter::PlanRequest& req,
                                              const std::wstring& apiKey,
                                              const std::wstring& model) const
{
    LLMRouter::PlanResponse r;
    std::ostringstream body;
    body << "{\"model\":\"" << WToU8(model.empty() ? L"claude-opus-4-7" : model) << "\","
         << "\"max_tokens\":512,"
         << "\"messages\":[{\"role\":\"user\",\"content\":\""
         << WToU8(req.question)
         << " (응답은 다음 JSON 형식: {\\\"toolName\\\":\\\"...\\\",\\\"params\\\":{...},\\\"title\\\":\\\"자연스러운 한국어 제목\\\",\\\"reasoning\\\":\\\"간단한 사고\\\"})\"}]}";

    std::wstring headers =
        L"Content-Type: application/json\r\n"
        L"x-api-key: " + apiKey + L"\r\n"
        L"anthropic-version: 2023-06-01\r\n";

    auto resp = HttpClient::Post(L"api.anthropic.com", L"/v1/messages", headers, body.str());
    if (!resp.ok())
    {
        r.ok = false;
        r.error = resp.error.empty() ? L"Claude API 호출 실패" : resp.error;
        return r;
    }
    std::wstring w = U8ToW(resp.body);
    r.toolName  = ExtractJsonString(w, L"toolName");
    r.title     = ExtractJsonString(w, L"title");
    auto reason = ExtractJsonString(w, L"reasoning");
    r.reasoning = reason.empty() ? L"Claude 응답 파싱" : reason;
    r.ok = !r.toolName.empty();
    return r;
}

} // namespace deepmetria
