// Parsers/ParserFactory.cpp
#include "stdafx.h"
#include "ParserFactory.h"
#include "CsvParser.h"
#include "JsonParser.h"
#include "XlsxParser.h"
#include <cwctype>
#include <algorithm>

namespace deepmetria {

std::unique_ptr<IDataParser> ParserFactory::CreateForExtension(const std::wstring& dotExt)
{
    std::wstring lower = dotExt;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });

    if (lower == L".csv")
        return std::make_unique<CsvParser>();
    if (lower == L".json")
        return std::make_unique<JsonParser>();
    if (lower == L".xlsx" || lower == L".xls" || lower == L".xlsm")
        return std::make_unique<XlsxParser>();

    return nullptr;
}

} // namespace deepmetria
