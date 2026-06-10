// Domain/DataSource.h - 불러온 데이터 원천 (메모리 보관)
#pragma once
#include <string>
#include <vector>
#include <optional>

namespace deepmetria {

enum class ColumnType { Unknown, Number, Text, Date };

struct Column
{
    std::wstring name;
    ColumnType   type = ColumnType::Unknown;
    long long    rowCount = 0;
    long long    missing  = 0;
    double       sum      = 0.0;
    double       mean     = 0.0;
    double       minVal   = 0.0;
    double       maxVal   = 0.0;
    std::wstring inferredDomain;   // FR-02 도메인 추정 (예: "월별 매출")
};

class DataSource
{
public:
    DataSource() = default;

    void Reset();
    bool IsEmpty() const { return m_rows.empty() && m_columns.empty(); }

    void AddColumn(const std::wstring& name);
    void AddRow(std::vector<std::wstring> row);

    const std::vector<Column>& Columns() const { return m_columns; }
    std::vector<Column>&       MutableColumns() { return m_columns; }
    const std::vector<std::vector<std::wstring>>& Rows() const { return m_rows; }

    const std::wstring& SourcePath() const { return m_sourcePath; }
    void SetSourcePath(std::wstring p) { m_sourcePath = std::move(p); }

    const std::wstring& DomainSummary() const { return m_domainSummary; }
    void SetDomainSummary(std::wstring s) { m_domainSummary = std::move(s); }

private:
    std::vector<Column>                          m_columns;
    std::vector<std::vector<std::wstring>>       m_rows;
    std::wstring                                 m_sourcePath;
    std::wstring                                 m_domainSummary;
};

} // namespace deepmetria
