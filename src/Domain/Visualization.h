// Domain/Visualization.h - 차트/표 한 단위
#pragma once
#include <string>
#include <vector>

namespace deepmetria {

enum class VizType { Table, Bar, Line, Pie, Summary };

struct Series
{
    std::wstring        label;
    std::vector<double> values;
};

struct Visualization
{
    int          id        = 0;
    VizType      type      = VizType::Table;
    std::wstring title;
    int          x         = 16;
    int          y         = 16;
    int          width     = 480;
    int          height    = 300;
    // 스타일
    DWORD        fillColor  = 0x4F46E5;   // indigo-600
    DWORD        accentColor= 0x10B981;   // emerald-500
    // 표시 옵션
    bool         showTitle  = true;       // 카드 헤더에 title 표시 여부
    bool         showGrid   = true;       // 그리드선 표시 여부 (Bar/Line)
    // 데이터
    std::vector<std::wstring> categories;
    std::vector<Series>       series;
    // 표 전용
    std::vector<std::wstring>                rowHeaders;
    std::vector<std::vector<std::wstring>>   tableCells;
    // 부가 정보
    std::wstring caption;
};

} // namespace deepmetria
