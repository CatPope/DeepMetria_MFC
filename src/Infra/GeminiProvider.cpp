// Infra/GeminiProvider.cpp - Gemini API 어댑터 구현
#include "stdafx.h"
#include "GeminiProvider.h"
#include "HttpClient.h"
#include <sstream>
#include <unordered_map>

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

// params 객체 안의 "key":"value" 쌍들을 모두 추출 (params 누락 시 빈 맵)
std::unordered_map<std::wstring, std::wstring>
ExtractParamsMap(const std::wstring& json)
{
    std::unordered_map<std::wstring, std::wstring> out;
    auto p = json.find(L"\"params\"");
    if (p == std::wstring::npos) return out;
    auto open = json.find(L'{', p);
    if (open == std::wstring::npos) return out;
    // 매칭 '}' 찾기 (depth-counter, 문자열 무시)
    int depth = 0; size_t end = open; bool inStr = false;
    for (size_t i = open; i < json.size(); ++i)
    {
        wchar_t c = json[i];
        if (c == L'"' && (i == 0 || json[i-1] != L'\\')) inStr = !inStr;
        if (inStr) continue;
        if (c == L'{') ++depth;
        else if (c == L'}') { --depth; if (depth == 0) { end = i; break; } }
    }
    if (end <= open) return out;
    std::wstring body = json.substr(open + 1, end - open - 1);

    // "key" : "value" 또는 "key" : number  형태 추출
    size_t i = 0;
    while (i < body.size())
    {
        auto k1 = body.find(L'"', i);
        if (k1 == std::wstring::npos) break;
        auto k2 = body.find(L'"', k1 + 1);
        if (k2 == std::wstring::npos) break;
        std::wstring key = body.substr(k1 + 1, k2 - k1 - 1);
        auto colon = body.find(L':', k2);
        if (colon == std::wstring::npos) break;
        // value: 문자열 또는 숫자 또는 true/false
        size_t v = colon + 1;
        while (v < body.size() && (body[v] == L' ' || body[v] == L'\t' || body[v] == L'\n')) ++v;
        std::wstring val;
        if (v < body.size() && body[v] == L'"')
        {
            auto v2 = body.find(L'"', v + 1);
            if (v2 == std::wstring::npos) break;
            val = body.substr(v + 1, v2 - v - 1);
            i = v2 + 1;
        }
        else
        {
            size_t v2 = v;
            while (v2 < body.size() && body[v2] != L',' && body[v2] != L'}' && body[v2] != L'\n')
                ++v2;
            val = body.substr(v, v2 - v);
            // trim
            while (!val.empty() && (val.back() == L' ' || val.back() == L'\t' || val.back() == L'\r'))
                val.pop_back();
            i = v2;
        }
        if (!key.empty()) out[key] = val;
    }
    return out;
}

} // namespace

static std::string JsonEscape(const std::string& s)
{
    std::string out; out.reserve(s.size() + 8);
    for (char c : s)
    {
        switch (c)
        {
        case '"' : out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if ((unsigned char)c < 0x20) out += ' ';
            else out += c;
        }
    }
    return out;
}

LLMRouter::PlanResponse GeminiProvider::Plan(const LLMRouter::PlanRequest& req,
                                              const std::wstring& apiKey,
                                              const std::wstring& model) const
{
    LLMRouter::PlanResponse r;

    // 시스템 프롬프트
    std::wstring sys;
    sys += L"당신은 한국어 데이터 분석 어시스턴트입니다. 컬럼 메타와 도구만 보고 결정합니다.\n";
    sys += L"[데이터] " + req.dataSummary + L"\n";
    if (!req.columns.empty())
    {
        sys += L"[컬럼]\n";
        for (const auto& c : req.columns)
        {
            const wchar_t* t = c.typeEnum == 1 ? L"수치" : c.typeEnum == 2 ? L"텍스트" : c.typeEnum == 3 ? L"날짜" : L"기타";
            sys += L"- " + c.name + L" (" + t + L")";
            if (c.missing > 0) sys += L", 결측 " + std::to_wstring(c.missing);
            if (c.typeEnum == 1)
            {
                wchar_t buf[128];
                swprintf_s(buf, L", 평균 %.2f, min %.2f, max %.2f", c.meanVal, c.minVal, c.maxVal);
                sys += buf;
            }
            else if (c.typeEnum == 2 && c.uniqueCount > 0)
                sys += L", " + std::to_wstring(c.uniqueCount) + L"개 고유값";
            if (!c.sampleValue.empty()) sys += L", 예: \"" + c.sampleValue + L"\"";
            sys += L"\n";
        }
    }
    if (!req.tools.empty())
    {
        sys += L"[도구] (모두 시각화 생성형 — 호출 시 viz 카드가 만들어짐)\n";
        sys += L"각 도구의 params 키 이름은 스키마와 정확히 일치해야 함. 임의로 추측 금지.\n";
        for (const auto& t : req.tools)
        {
            sys += L"- " + t.name + L": " + t.description + L"\n";
            sys += L"    params 스키마: " + t.jsonParameters + L"\n";
        }
    }
    sys += L"\n[데이터 형식 가이드]\n"
           L"- wide 형식 (한 행 = 한 시점, 여러 수치 컬럼이 카테고리 역할): "
           L"compare_columns / compare_rows / select_rows_columns 사용.\n"
           L"  예) 컬럼 = [연도, 중앙정부, 지방자치단체, 적자성채무] 인 경우 \n"
           L"      '2010년의 부처별 예산' → compare_columns(filterColumn=연도, filterValue=2010, valueColumnsCsv=\"중앙정부,지방자치단체,적자성채무\")\n"
           L"- long 형식 (한 행 = 한 측정, group 컬럼 + 단일 value 컬럼): "
           L"group_by_sum / group_by_mean / aggregate 사용.\n"
           L"- 컬럼이 같은 단위 수치 여러 개 = wide. 컬럼이 카테고리 1개 + 값 1개 = long.\n\n"
           L"사용자가 시각화/그래프/표를 명시적으로 요청한 경우에만 도구 호출.\n"
           L"필드 규칙:\n"
           L"- title: 시각화 카드 헤더 (8자 이내 짧은 라벨)\n"
           L"- description: ★★ 반드시 포함 (필수) ★★. 채팅으로 전달할 1-3문장 결과/인사이트.\n"
           L"  title과 절대 같으면 안 되고, '시각화를 만들었습니다' 같은 무의미한 안내문 금지.\n"
           L"  실제 데이터에서 발견한 패턴/수치/경향을 짧게 설명할 것.\n"
           L"- 시각화 필요: {\"toolName\":\"...\",\"params\":{...},\"title\":\"짧은 제목\",\"description\":\"결과 설명\",\"reasoning\":\"사고\"}\n"
           L"- 단순 대화 (사용자가 시각화 요청 안 함): {\"toolName\":\"\",\"description\":\"답변\",\"reasoning\":\"사고\"}\n"
           L"어떤 경우든 description 은 비워두면 안 됨.";

    // contents: history + 새 user (Gemini는 role=user/model 교차)
    std::string contents = "[";
    bool first = true;
    for (const auto& turn : req.history)
    {
        if (!first) contents += ",";
        first = false;
        contents += "{\"role\":\"";
        contents += (turn.isUser ? "user" : "model");
        contents += "\",\"parts\":[{\"text\":\"";
        contents += JsonEscape(WToU8(turn.text));
        contents += "\"}]}";
    }
    if (!first) contents += ",";
    contents += "{\"role\":\"user\",\"parts\":[{\"text\":\"";
    contents += JsonEscape(WToU8(req.question));
    contents += "\"}]}]";

    std::ostringstream body;
    body << "{\"systemInstruction\":{\"parts\":[{\"text\":\""
         << JsonEscape(WToU8(sys)) << "\"}]},"
         << "\"contents\":" << contents << "}";

    std::wstring path = L"/v1beta/models/" +
        (model.empty() ? std::wstring(L"gemini-2.5-flash") : model) +
        L":generateContent?key=" + apiKey;

    std::wstring headers = L"Content-Type: application/json\r\n";
    auto resp = HttpClient::Post(L"generativelanguage.googleapis.com", path, headers, body.str());
    if (!resp.ok())
    {
        r.ok = false;
        wchar_t buf[64]; swprintf_s(buf, L"Gemini HTTP %d", resp.status);
        r.error = buf;
        // 에러 응답 본문에서 "message" 필드를 뽑아 사용자에게 노출
        std::wstring err = U8ToW(resp.body);
        auto msg = ExtractJsonString(err, L"message");
        if (!msg.empty())
        {
            if (msg.size() > 200) msg = msg.substr(0, 200) + L"…";
            r.error += L": " + msg;
        }
        else if (!resp.error.empty())
        {
            r.error += L" (" + resp.error + L")";
        }
        return r;
    }
    std::wstring w = U8ToW(resp.body);
    // Gemini는 LLM 답변을 candidates[0].content.parts[0].text 의 string 값에 넣음.
    // 그 string 안의 JSON이 \" 로 escape 되어 있어 그대로는 toolName 등을 못 찾는다.
    // backslash-doublequote 시퀀스를 일반 doublequote 로 unescape 한 뒤 추출.
    {
        std::wstring tmp; tmp.reserve(w.size());
        for (size_t i = 0; i < w.size(); ++i)
        {
            if (w[i] == L'\\' && i + 1 < w.size())
            {
                wchar_t n = w[i + 1];
                if (n == L'"' || n == L'\\') { tmp += n; ++i; continue; }
                if (n == L'n') { tmp += L'\n'; ++i; continue; }
                if (n == L't') { tmp += L'\t'; ++i; continue; }
                if (n == L'/') { tmp += L'/';  ++i; continue; }
                if (n == L'r') {              ++i; continue; }  // CR drop
            }
            tmp += w[i];
        }
        w.swap(tmp);
    }
    // candidates[0].content.parts[0].text 부터 뽑은 LLM JSON 안에 진짜 답변이 있음.
    // text 필드 다음의 첫 '{' 부터 매칭되는 '}' 까지를 우선 잘라 inner 로 사용.
    // 또한 ```json ... ``` 코드펜스가 LLM 응답에 포함된 경우 펜스 사이의 JSON 만 사용.
    std::wstring inner = w;
    {
        auto textPos = w.find(L"\"text\"");
        if (textPos != std::wstring::npos)
        {
            auto braceL = w.find(L'{', textPos);
            if (braceL != std::wstring::npos)
            {
                // depth-counter 로 매칭 닫는 '}' 찾기 (문자열 내부 무시는 단순화)
                int depth = 0; size_t i = braceL;
                bool inStr = false;
                for (; i < w.size(); ++i)
                {
                    wchar_t c = w[i];
                    if (c == L'"' && (i == 0 || w[i-1] != L'\\')) inStr = !inStr;
                    if (inStr) continue;
                    if (c == L'{') ++depth;
                    else if (c == L'}') { --depth; if (depth == 0) { ++i; break; } }
                }
                if (i > braceL) inner = w.substr(braceL, i - braceL);
            }
        }
        // ```json ... ``` 펜스 제거 (LLM이 markdown 으로 감싼 경우)
        auto fence = inner.find(L"```");
        if (fence != std::wstring::npos)
        {
            auto bodyStart = inner.find(L'\n', fence);
            auto endFence  = inner.find(L"```", fence + 3);
            if (bodyStart != std::wstring::npos && endFence != std::wstring::npos
                && bodyStart < endFence)
                inner = inner.substr(bodyStart + 1, endFence - bodyStart - 1);
        }
    }
    r.toolName    = ExtractJsonString(inner, L"toolName");
    r.title       = ExtractJsonString(inner, L"title");
    r.description = ExtractJsonString(inner, L"description");
    r.params      = ExtractParamsMap(inner);
    // 폴백: inner 에서 못 찾으면 전체 응답에서 재시도 (text 발견 못한 경우)
    if (r.toolName.empty())    r.toolName    = ExtractJsonString(w, L"toolName");
    if (r.title.empty())       r.title       = ExtractJsonString(w, L"title");
    if (r.description.empty()) r.description = ExtractJsonString(w, L"description");
    if (r.params.empty())      r.params      = ExtractParamsMap(w);
    auto reason   = ExtractJsonString(inner, L"reasoning");
    if (reason.empty()) reason = ExtractJsonString(w, L"reasoning");
    r.reasoning   = reason.empty() ? L"Gemini 응답 파싱" : reason;
    // 정책: description 은 반드시 LLM 이 제공해야 함. 누락 시 에러로 처리.
    if (r.description.empty())
    {
        r.ok = false;
        r.error = L"LLM이 description 필드를 반환하지 않았습니다. (Gemini 응답)";
        return r;
    }
    r.ok = true;
    return r;
}

} // namespace deepmetria
