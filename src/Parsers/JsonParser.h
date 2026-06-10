// Parsers/JsonParser.h - 간이 JSON 배열 파서 (객체 배열 → 표 형태)
#pragma once
#include "IDataParser.h"
#include "DataSource.h"
#include <string>

namespace deepmetria {

class JsonParser : public IDataParser
{
public:
    bool Parse(const std::wstring& path, DataSource& out) override;
};

} // namespace deepmetria
