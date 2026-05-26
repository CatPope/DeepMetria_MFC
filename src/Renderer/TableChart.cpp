#include "stdafx.h"

// TableChart.cpp — 테이블 시각화 렌더러
// Architecture §3 Renderer 레이어, DetailedSpec §6 참조
// 데이터 형식: {"columns":["Name","Score"],"rows":[["Alice","90"],["Bob","85"]]}

#include "TableChart.h"
#include <algorithm>

using namespace Gdiplus;

// ============================================================
// JSON 파싱
// 형식: {"columns":["Name","Score"],"rows":[["Alice","90"],["Bob","85"]]}
// ============================================================

void CTableChart::ParseTableData(const CString& dataJson,
                                  std::vector<CString>& outColumns,
                                  std::vector<std::vector<CString>>& outRows)
{
    outColumns.clear();
    outRows.clear();

    std::wstring json = dataJson.GetString();

    // "columns" 배열 파싱
    size_t colPos = json.find(L"\"columns\"");
    if (colPos != std::wstring::npos)
    {
        size_t arrStart = json.find(L'[', colPos);
        size_t arrEnd   = json.find(L']', arrStart);
        if (arrStart != std::wstring::npos && arrEnd != std::wstring::npos)
        {
            std::wstring colArr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
            size_t pos = 0;
            while (pos < colArr.size())
            {
                size_t q1 = colArr.find(L'"', pos);
                if (q1 == std::wstring::npos) break;
                size_t q2 = colArr.find(L'"', q1 + 1);
                if (q2 == std::wstring::npos) break;
                outColumns.push_back(CString(colArr.substr(q1 + 1, q2 - q1 - 1).c_str()));
                pos = q2 + 1;
            }
        }
    }

    // "rows" 배열 파싱
    size_t rowsPos = json.find(L"\"rows\"");
    if (rowsPos != std::wstring::npos)
    {
        size_t outerStart = json.find(L'[', rowsPos);
        if (outerStart == std::wstring::npos) return;

        // 중첩 배열 전체 범위 찾기
        int depth = 0;
        size_t outerEnd = outerStart;
        for (size_t i = outerStart; i < json.size(); ++i)
        {
            if (json[i] == L'[') ++depth;
            else if (json[i] == L']') { --depth; if (depth == 0) { outerEnd = i; break; } }
        }

        // 각 행 [...]  파싱
        size_t pos = outerStart + 1;
        while (pos < outerEnd)
        {
            size_t rowStart = json.find(L'[', pos);
            if (rowStart == std::wstring::npos || rowStart >= outerEnd) break;
            size_t rowEnd = json.find(L']', rowStart);
            if (rowEnd == std::wstring::npos) break;

            std::wstring rowStr = json.substr(rowStart + 1, rowEnd - rowStart - 1);
            std::vector<CString> rowData;
            size_t rpos = 0;
            while (rpos < rowStr.size())
            {
                size_t q1 = rowStr.find(L'"', rpos);
                if (q1 == std::wstring::npos) break;
                size_t q2 = rowStr.find(L'"', q1 + 1);
                if (q2 == std::wstring::npos) break;
                rowData.push_back(CString(rowStr.substr(q1 + 1, q2 - q1 - 1).c_str()));
                rpos = q2 + 1;
            }
            if (!rowData.empty())
                outRows.push_back(rowData);
            pos = rowEnd + 1;
        }
    }
}

// ============================================================
// 메인 Draw
// ============================================================

void CTableChart::Draw(Gdiplus::Graphics& g, const CRect& plotArea, const ChartConfig& config)
{
    std::vector<CString> columns;
    std::vector<std::vector<CString>> rows;
    ParseTableData(config.dataJson, columns, rows);

    if (columns.empty() && rows.empty())
        return;

    const int nCols = (int)columns.size();
    if (nCols == 0) return;

    const REAL rowHeight   = 24.0f;
    const REAL headerH     = 28.0f;
    const REAL colW        = (REAL)plotArea.Width() / nCols;
    const REAL startX      = (REAL)plotArea.left;
    const REAL startY      = (REAL)plotArea.top;

    FontFamily ff(L"맑은 고딕");
    Gdiplus::Font headerFont(&ff, 10, FontStyleBold,    UnitPoint);
    Gdiplus::Font cellFont  (&ff, 10, FontStyleRegular, UnitPoint);

    SolidBrush headerBgBrush(Color(220, 220, 220));
    SolidBrush headerTextBrush(Color(32, 32, 32));
    SolidBrush rowBrushEven(Color(255, 255, 255));
    SolidBrush rowBrushOdd (Color(245, 245, 250));
    SolidBrush cellTextBrush(Color(50, 50, 50));
    Pen gridPen(Color(200, 200, 200), 1.0f);

    StringFormat sf;
    sf.SetAlignment(StringAlignmentNear);
    sf.SetLineAlignment(StringAlignmentCenter);
    sf.SetFormatFlags(StringFormatFlagsNoWrap);
    sf.SetTrimming(StringTrimmingEllipsisCharacter);

    // 헤더 행
    g.FillRectangle(&headerBgBrush,
                    startX, startY,
                    (REAL)plotArea.Width(), headerH);

    for (int c = 0; c < nCols; ++c)
    {
        REAL cellX = startX + c * colW;
        // 셀 테두리
        g.DrawRectangle(&gridPen, cellX, startY, colW, headerH);
        // 텍스트 (4px 내부 패딩)
        RectF textRect(cellX + 4.0f, startY, colW - 8.0f, headerH);
        g.DrawString(columns[c], -1, &headerFont, textRect, &sf, &headerTextBrush);
    }

    // 데이터 행
    for (int r = 0; r < (int)rows.size(); ++r)
    {
        REAL rowY = startY + headerH + r * rowHeight;

        // 행 배경 (교대 색상)
        SolidBrush& rowBg = (r % 2 == 0) ? rowBrushEven : rowBrushOdd;
        g.FillRectangle(&rowBg, startX, rowY, (REAL)plotArea.Width(), rowHeight);

        for (int c = 0; c < nCols; ++c)
        {
            REAL cellX = startX + c * colW;
            g.DrawRectangle(&gridPen, cellX, rowY, colW, rowHeight);

            if (c < (int)rows[r].size())
            {
                RectF textRect(cellX + 4.0f, rowY, colW - 8.0f, rowHeight);
                g.DrawString(rows[r][c], -1, &cellFont, textRect, &sf, &cellTextBrush);
            }
        }
    }

    // 전체 외곽선
    Pen outerPen(Color(160, 160, 160), 1.5f);
    int totalRows = (int)rows.size();
    REAL totalH   = headerH + totalRows * rowHeight;
    g.DrawRectangle(&outerPen, startX, startY, (REAL)plotArea.Width(), totalH);
}
