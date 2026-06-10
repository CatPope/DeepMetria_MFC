#pragma once
#include "DataSource.h"
#include <string>

namespace deepmetria {

class IDataParser
{
public:
    virtual ~IDataParser() = default;
    virtual bool Parse(const std::wstring& path, DataSource& out) = 0;
};

} // namespace deepmetria
