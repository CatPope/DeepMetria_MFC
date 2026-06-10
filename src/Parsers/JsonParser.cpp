// Parsers/JsonParser.cpp - 매우 단순한 객체 배열 파서
// [
//   { "name": "..", "value": 1, ... },
//   ...
// ]
// (중첩 객체/배열은 문자열로 보존)
#include "stdafx.h"
#include "JsonParser.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cwctype>

namespace deepmetria {

namespace {

std::wstring ReadAllWide(const std::wstring& path)
{
    std::ifstream f(std::filesystem::path(path), std::ios::binary);
    if (!f.is_open()) return {};
    std::ostringstream oss; oss << f.rdbuf();
    std::string raw = oss.str();
    if (raw.size() >= 3 &&
        static_cast<unsigned char>(raw[0]) == 0xEF &&
        static_cast<unsigned char>(raw[1]) == 0xBB &&
        static_cast<unsigned char>(raw[2]) == 0xBF)
    {
        raw.erase(0, 3);
    }
    int n = MultiByteToWideChar(CP_UTF8, 0, raw.data(), static_cast<int>(raw.size()), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, raw.data(), static_cast<int>(raw.size()), w.data(), n);
    return w;
}

// 매우 단순한 토크나이저
struct Parser
{
    const std::wstring& s;
    size_t i = 0;
    explicit Parser(const std::wstring& src) : s(src) {}
    void SkipWs() { while (i < s.size() && std::iswspace(s[i])) ++i; }
    bool Eat(wchar_t c) { SkipWs(); if (i < s.size() && s[i] == c) { ++i; return true; } return false; }
    bool Peek(wchar_t c) { SkipWs(); return i < s.size() && s[i] == c; }

    std::wstring ParseString()
    {
        std::wstring out;
        SkipWs();
        if (i >= s.size() || s[i] != L'"') return out;
        ++i;
        while (i < s.size() && s[i] != L'"')
        {
            if (s[i] == L'\\' && i + 1 < s.size())
            {
                wchar_t n = s[i + 1];
                switch (n)
                {
                case L'"':  out += L'"';  break;
                case L'\\': out += L'\\'; break;
                case L'n':  out += L'\n'; break;
                case L't':  out += L'\t'; break;
                default:    out += n;     break;
                }
                i += 2;
            }
            else out += s[i++];
        }
        if (i < s.size()) ++i;   // 닫는 "
        return out;
    }

    std::wstring ParseValue()
    {
        SkipWs();
        if (i >= s.size()) return {};
        if (s[i] == L'"') return ParseString();
        if (s[i] == L'{' || s[i] == L'[')
        {
            // 중첩은 문자열로 한 덩어리 보존
            wchar_t open = s[i], close = (open == L'{') ? L'}' : L']';
            int depth = 1;
            std::wstring buf; buf += s[i++];
            while (i < s.size() && depth > 0)
            {
                if (s[i] == open) ++depth;
                else if (s[i] == close) --depth;
                buf += s[i++];
            }
            return buf;
        }
        // 숫자/불리언/null
        std::wstring out;
        while (i < s.size() && s[i] != L',' && s[i] != L'}' && s[i] != L']' && !std::iswspace(s[i]))
            out += s[i++];
        return out;
    }
};

} // namespace

bool JsonParser::Parse(const std::wstring& path, DataSource& out)
{
    out.Reset();
    out.SetSourcePath(path);

    std::wstring src = ReadAllWide(path);
    if (src.empty()) return false;

    Parser p(src);
    if (!p.Eat(L'[')) return false;

    std::vector<std::wstring> colOrder;
    std::vector<std::unordered_map<std::wstring, std::wstring>> rows;

    while (!p.Peek(L']'))
    {
        if (!p.Eat(L'{')) return false;
        std::unordered_map<std::wstring, std::wstring> row;
        while (!p.Peek(L'}'))
        {
            std::wstring key = p.ParseString();
            if (!p.Eat(L':')) return false;
            std::wstring val = p.ParseValue();
            if (row.find(key) == row.end())
            {
                bool seen = false;
                for (const auto& c : colOrder) if (c == key) { seen = true; break; }
                if (!seen) colOrder.push_back(key);
            }
            row[key] = std::move(val);
            p.Eat(L',');
        }
        p.Eat(L'}');
        rows.push_back(std::move(row));
        p.Eat(L',');
    }

    for (const auto& c : colOrder) out.AddColumn(c);
    for (const auto& r : rows)
    {
        std::vector<std::wstring> row;
        row.reserve(colOrder.size());
        for (const auto& c : colOrder)
        {
            auto it = r.find(c);
            row.push_back(it == r.end() ? std::wstring{} : it->second);
        }
        out.AddRow(std::move(row));
    }
    return true;
}

} // namespace deepmetria
