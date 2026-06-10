// Domain/DataSource.cpp
#include "stdafx.h"
#include "DataSource.h"

namespace deepmetria {

void DataSource::Reset()
{
    m_columns.clear();
    m_rows.clear();
    m_sourcePath.clear();
    m_domainSummary.clear();
}

void DataSource::AddColumn(const std::wstring& name)
{
    Column c;
    c.name = name;
    m_columns.push_back(std::move(c));
}

void DataSource::AddRow(std::vector<std::wstring> row)
{
    m_rows.push_back(std::move(row));
}

} // namespace deepmetria
