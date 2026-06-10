// Infra/LLMRouter.cpp - 멀티 LLM 라우터
#include "stdafx.h"
#include "LLMRouter.h"
#include "SecretsStore.h"
#include "HttpClient.h"
#include "ClaudeProvider.h"
#include "OpenAIProvider.h"
#include "GeminiProvider.h"
#include <sstream>
#include <cctype>
#include <regex>

namespace deepmetria {

namespace {

LLMRouter::Provider ParseProvider(const std::wstring& s)
{
    std::wstring t; t.reserve(s.size());
    for (wchar_t c : s) t.push_back(static_cast<wchar_t>(std::towlower(c)));
    if (t == L"claude")  return LLMRouter::Provider::Claude;
    if (t == L"openai" || t == L"gpt") return LLMRouter::Provider::OpenAI;
    if (t == L"gemini" || t == L"google") return LLMRouter::Provider::Gemini;
    return LLMRouter::Provider::Unknown;
}

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

// 매우 간이한 JSON 추출기 — "key":"value" 패턴 한 줄
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

std::wstring ContainsAny(const std::wstring& q, std::initializer_list<const wchar_t*> kws)
{
    for (auto k : kws) if (q.find(k) != std::wstring::npos) return k;
    return {};
}

} // namespace

LLMRouter::LLMRouter(std::shared_ptr<SecretsStore> secrets)
    : m_secrets(std::move(secrets))
{
    m_providers[(int)Provider::Claude] = std::make_unique<ClaudeProvider>();
    m_providers[(int)Provider::OpenAI] = std::make_unique<OpenAIProvider>();
    m_providers[(int)Provider::Gemini] = std::make_unique<GeminiProvider>();
}

LLMRouter::~LLMRouter() = default;

void LLMRouter::LoadConfig()
{
    if (!m_secrets) return;
    if (auto p = m_secrets->GetValue(L"Provider")) m_provider = ParseProvider(*p);
    if (auto m = m_secrets->GetValue(L"Model"))    m_model = *m;

    // 키 이름: ApiKey.Claude / ApiKey.OpenAI / ApiKey.Gemini
    const wchar_t* name =
        m_provider == Provider::Claude ? L"ApiKey.Claude" :
        m_provider == Provider::OpenAI ? L"ApiKey.OpenAI" :
        m_provider == Provider::Gemini ? L"ApiKey.Gemini" : nullptr;
    if (name)
    {
        if (auto k = m_secrets->GetSecret(name)) m_apiKey = *k;
    }
}

LLMRouter::PlanResponse LLMRouter::Plan(const PlanRequest& req)
{
    if (m_provider == Provider::Unknown || m_apiKey.empty())
        return PlanLocalHeuristic(req);

    PlanResponse r;
    auto it = m_providers.find((int)m_provider);
    if (it != m_providers.end())
        r = it->second->Plan(req, m_apiKey, m_model);
    else
    { r.ok = false; r.error = L"알 수 없는 프로바이더"; }
    // LLM 호출 실패 시 폴백 절대 금지 — error 그대로 반환해서 호출자가 에러 다이얼로그 표시.
    // 자동으로 시각화 만들지 않고, 채팅에도 AI 응답 추가하지 않음.
    return r;
}

LLMRouter::PlanResponse LLMRouter::PlanLocalHeuristic(const PlanRequest& req) const
{
    PlanResponse r;
    r.ok = true;
    r.reasoning = L"오프라인 휴리스틱(키 미등록 또는 API 실패 폴백)";

    const auto& q = req.question;

    // 추이/추세 키워드
    if (!ContainsAny(q, { L"추이", L"추세", L"trend", L"시계열", L"월별", L"일별", L"연도별" }).empty())
    {
        // date+number 컬럼 한 쌍
        std::wstring date, num;
        for (const auto& c : req.columns)
        {
            if (c.typeEnum == 3 /*Date*/   && date.empty()) date = c.name;
            if (c.typeEnum == 1 /*Number*/ && num.empty())  num  = c.name;
        }
        if (!date.empty() && !num.empty())
        {
            r.toolName = L"trend_over_time";
            r.params[L"date"]  = date;
            r.params[L"value"] = num;
            return r;
        }
    }

    // 상위/분포
    if (!ContainsAny(q, { L"상위", L"top", L"순위", L"분포", L"빈도" }).empty())
    {
        std::wstring text;
        for (const auto& c : req.columns)
            if (c.typeEnum == 2 /*Text*/ && text.empty()) text = c.name;
        if (!text.empty())
        {
            r.toolName = L"top_n_distribution";
            r.params[L"column"] = text;
            r.params[L"n"]      = L"5";
            return r;
        }
    }

    // 합계/총합/그룹별
    if (!ContainsAny(q, { L"합계", L"총합", L"sum", L"별", L"그룹" }).empty())
    {
        std::wstring g, v;
        for (const auto& c : req.columns)
        {
            if (g.empty() && c.typeEnum == 2 /*Text*/)   g = c.name;
            if (v.empty() && c.typeEnum == 1 /*Number*/) v = c.name;
        }
        if (!g.empty() && !v.empty())
        {
            r.toolName = L"group_by_sum";
            r.params[L"group"] = g;
            r.params[L"value"] = v;
            return r;
        }
    }

    // 정책: title/description 은 LLM 만 작성. 폴백 휴리스틱은 toolName/params 만 제공.
    // 폴백: 표 미리보기
    r.toolName = L"table_sample";
    r.params[L"rows"] = L"20";
    return r;
}

} // namespace deepmetria
