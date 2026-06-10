// Parsers/CsvParser.cpp - UTF-8 / CP949 자동 감지, BOM 스킵, 따옴표 처리
#include "stdafx.h"
#include "CsvParser.h"
#include <fstream>
#include <sstream>
#include <vector>

namespace deepmetria {

namespace {

std::wstring DecodeUtf8OrAcp(const std::string& raw)
{
    // UTF-8로 우선 시도
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                raw.data(), static_cast<int>(raw.size()), nullptr, 0);
    if (n > 0)
    {
        std::wstring w(n, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, raw.data(), static_cast<int>(raw.size()), w.data(), n);
        return w;
    }
    // ACP (CP949 등)
    n = MultiByteToWideChar(CP_ACP, 0, raw.data(), static_cast<int>(raw.size()), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_ACP, 0, raw.data(), static_cast<int>(raw.size()), w.data(), n);
    return w;
}

std::vector<std::wstring> SplitCsvLine(const std::wstring& line)
{
    std::vector<std::wstring> out;
    std::wstring cur;
    bool inQuote = false;
    for (size_t i = 0; i < line.size(); ++i)
    {
        wchar_t c = line[i];
        if (inQuote)
        {
            if (c == L'"')
            {
                if (i + 1 < line.size() && line[i + 1] == L'"') { cur += L'"'; ++i; }
                else inQuote = false;
            }
            else cur += c;
        }
        else
        {
            if (c == L',')
            {
                out.push_back(std::move(cur));
                cur.clear();
            }
            else if (c == L'"')
            {
                inQuote = true;
            }
            else if (c == L'\r')
            {
                // 무시
            }
            else cur += c;
        }
    }
    out.push_back(std::move(cur));
    return out;
}

} // namespace

bool CsvParser::Parse(const std::wstring& path, DataSource& out)
{
    out.Reset();
    out.SetSourcePath(path);

    std::ifstream f(std::filesystem::path(path), std::ios::binary);
    if (!f.is_open()) return false;

    std::ostringstream oss;
    oss << f.rdbuf();
    std::string raw = oss.str();
    if (raw.empty()) return false;

    // UTF-8 BOM 제거
    if (raw.size() >= 3 &&
        static_cast<unsigned char>(raw[0]) == 0xEF &&
        static_cast<unsigned char>(raw[1]) == 0xBB &&
        static_cast<unsigned char>(raw[2]) == 0xBF)
    {
        raw.erase(0, 3);
    }

    std::wstring text = DecodeUtf8OrAcp(raw);

    // 줄 단위 분할
    std::vector<std::wstring> lines;
    {
        std::wstringstream ws(text);
        std::wstring line;
        while (std::getline(ws, line)) lines.push_back(line);
    }
    if (lines.empty()) return false;

    // 헤더
    auto header = SplitCsvLine(lines[0]);
    for (const auto& h : header) out.AddColumn(h);

    // 데이터
    for (size_t i = 1; i < lines.size(); ++i)
    {
        if (lines[i].empty()) continue;
        out.AddRow(SplitCsvLine(lines[i]));
    }
    return true;
}

} // namespace deepmetria
