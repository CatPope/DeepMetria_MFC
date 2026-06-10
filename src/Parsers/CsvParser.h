// Parsers/CsvParser.h
#pragma once
#include "IDataParser.h"
#include "DataSource.h"
#include <string>

namespace deepmetria {

class CsvParser : public IDataParser
{
public:
    bool Parse(const std::wstring& path, DataSource& out) override;
};

} // namespace deepmetria
