#pragma once
#include "IDataParser.h"
#include <memory>
#include <string>

namespace deepmetria {

class ParserFactory
{
public:
    // dotExt 는 ".csv" / ".json" (소문자 권장, 내부에서 lower 변환)
    static std::unique_ptr<IDataParser> CreateForExtension(const std::wstring& dotExt);
};

} // namespace deepmetria
