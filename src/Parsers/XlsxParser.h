// Parsers/XlsxParser.h - Excel xlsx/xls 로더
// 구현 전략: PowerShell + Excel COM 자동화로 임시 CSV 추출 후 CsvParser 재사용.
// Microsoft Excel이 설치된 환경에서만 동작 (대부분의 Windows 데스크톱 환경).
#pragma once

#include "IDataParser.h"

namespace deepmetria {

class XlsxParser : public IDataParser
{
public:
    bool Parse(const std::wstring& path, DataSource& out) override;
};

} // namespace deepmetria
