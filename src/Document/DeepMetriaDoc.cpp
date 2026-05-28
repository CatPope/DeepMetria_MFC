// DeepMetriaDoc.cpp — CDeepMetriaDoc 구현
// 파일 로드, 데이터 모델 관리, 직렬화 담당

#include "stdafx.h"
#include "DeepMetriaDoc.h"
#include "../Infrastructure/Parser/CSVParser.h"
#include "../Infrastructure/Parser/ExcelParser.h"
#include "../Infrastructure/Parser/JsonParser.h"
#include <set>
#include <cfloat>

// ============================================================
// IMPLEMENT_DYNCREATE
// ============================================================
IMPLEMENT_DYNCREATE(CDeepMetriaDoc, CDocument)

BEGIN_MESSAGE_MAP(CDeepMetriaDoc, CDocument)
END_MESSAGE_MAP()

// ============================================================
// 생성자
// ============================================================
CDeepMetriaDoc::CDeepMetriaDoc()
{
}

// ============================================================
// OnNewDocument — 새 문서 초기화
// ============================================================
BOOL CDeepMetriaDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    // 내부 상태 초기화
    DeleteContents();

    return TRUE;
}

// ============================================================
// DeleteContents — 문서 내용 초기화
// ============================================================
void CDeepMetriaDoc::DeleteContents()
{
    m_dataTable      = DataTable();
    m_dataSummary    = DataSummary();
    m_visualizations.clear();

    CDocument::DeleteContents();
}

// ============================================================
// LoadFile — 파일 로드 및 DataTable 생성
// filePath: 전체 파일 경로 (.csv / .xlsx)
// outError: 실패 시 오류 정보 반환
// ============================================================
BOOL CDeepMetriaDoc::LoadFile(const CString& filePath, AppError& outError)
{
    if (filePath.IsEmpty())
    {
        outError = AppError(_T("INVALID_PATH"), _T("파일 경로가 비어 있습니다."), 2);
        return FALSE;
    }

    // 파일 존재 여부 확인
    if (GetFileAttributes(filePath) == INVALID_FILE_ATTRIBUTES)
    {
        outError = AppError(_T("FILE_NOT_FOUND"),
                            _T("파일을 찾을 수 없습니다: ") + filePath, 2);
        return FALSE;
    }

    // 확장자 추출
    CString ext = filePath.Mid(filePath.ReverseFind(L'.') + 1);
    ext.MakeLower();

    // 파일 형식 확인
    if (ext != _T("csv") && ext != _T("xlsx") && ext != _T("xls") && ext != _T("json"))
    {
        outError = AppError(_T("UNSUPPORTED_FILE_TYPE"),
                            _T("지원하지 않는 파일 형식입니다: .") + ext, 2);
        return FALSE;
    }

    // CSV 파서 호출
    if (ext == _T("csv"))
    {
        m_dataTable = CSVParser::Parse(filePath, outError);
        if (m_dataTable.headers.empty() && outError.IsError())
            return FALSE;
    }
    else if (ext == _T("xlsx") || ext == _T("xls"))
    {
        m_dataTable = ExcelParser::Parse(filePath, outError);
        if (m_dataTable.headers.empty() && outError.IsError())
            return FALSE;
    }
    else if (ext == _T("json"))
    {
        m_dataTable = JsonParser::Parse(filePath, outError);
        if (m_dataTable.headers.empty() && outError.IsError())
            return FALSE;
    }

    m_dataTable.fileName = filePath;

    // 행 기반 데이터 → 컬럼 기반 데이터 변환 (columns 필드 채우기)
    m_dataTable.columns.clear();
    for (int c = 0; c < m_dataTable.colCount; ++c)
    {
        DataColumn dc;
        dc.name = (c < (int)m_dataTable.headers.size()) ? m_dataTable.headers[c] : _T("");

        // 값 수집
        for (const auto& row : m_dataTable.rows)
        {
            if (c < (int)row.size())
                dc.values.push_back(row[c]);
            else
                dc.values.push_back(_T(""));
        }

        // 타입 추론: date → numeric → text 순서
        bool allNumeric = true;
        bool allDate = true;
        bool hasNonEmpty = false;

        for (const auto& v : dc.values)
        {
            if (v.IsEmpty()) continue;
            hasNonEmpty = true;

            // 날짜 패턴: YYYY-MM-DD, YYYY/MM/DD, YYYY.MM.DD
            if (allDate)
            {
                bool isDate = false;
                if (v.GetLength() >= 10)
                {
                    TCHAR sep = v[4];
                    if ((sep == _T('-') || sep == _T('/') || sep == _T('.')) &&
                        v[7] == sep &&
                        _istdigit(v[0]) && _istdigit(v[1]) &&
                        _istdigit(v[2]) && _istdigit(v[3]) &&
                        _istdigit(v[5]) && _istdigit(v[6]) &&
                        _istdigit(v[8]) && _istdigit(v[9]))
                    {
                        isDate = true;
                    }
                }
                if (!isDate) allDate = false;
            }

            // 숫자 검사
            if (allNumeric)
            {
                TCHAR* endPtr = nullptr;
                _tcstod(v, &endPtr);
                if (endPtr == (LPCTSTR)v || *endPtr != _T('\0'))
                    allNumeric = false;
            }
        }

        if (!hasNonEmpty)
            dc.type = _T("text");
        else if (allDate)
            dc.type = _T("date");
        else if (allNumeric)
            dc.type = _T("numeric");
        else
            dc.type = _T("text");

        m_dataTable.columns.push_back(dc);
    }

    // DataSummary 생성
    m_dataSummary = DataSummary();
    m_dataSummary.datasourceId = filePath;
    m_dataSummary.rowCount     = m_dataTable.rowCount;
    m_dataSummary.columnCount  = m_dataTable.colCount;

    for (int c = 0; c < (int)m_dataTable.columns.size(); ++c)
    {
        const DataColumn& dc = m_dataTable.columns[c];
        ColumnSchema cs;
        cs.name  = dc.name;
        cs.type  = dc.type;
        cs.index = c;

        // 결측치 계산 + 고유값 수 + 최솟값/최댓값
        cs.nullCount = 0;
        std::set<CString> uniqueVals;
        double minNum = DBL_MAX, maxNum = -DBL_MAX;
        CString minStr, maxStr;
        bool hasValue = false;

        for (const auto& v : dc.values)
        {
            if (v.IsEmpty()) { cs.nullCount++; continue; }
            uniqueVals.insert(v);

            if (dc.type == _T("numeric"))
            {
                double d = _tcstod(v, nullptr);
                if (d < minNum) { minNum = d; minStr = v; }
                if (d > maxNum) { maxNum = d; maxStr = v; }
                hasValue = true;
            }
            else if (dc.type == _T("date") || dc.type == _T("text"))
            {
                if (!hasValue || v < minStr) minStr = v;
                if (!hasValue || v > maxStr) maxStr = v;
                hasValue = true;
            }
        }

        cs.uniqueCount = (int)uniqueVals.size();
        if (hasValue)
        {
            cs.minValue = minStr;
            cs.maxValue = maxStr;
        }

        // 샘플값 (최대 5개)
        int sampleCount = (std::min)(5, (int)dc.values.size());
        for (int i = 0; i < sampleCount; ++i)
        {
            if (i > 0) cs.sampleValues += _T(", ");
            cs.sampleValues += dc.values[i];
        }

        m_dataSummary.schema.push_back(cs);
    }

    // 문서 수정 플래그 해제 (저장 프롬프트 방지)
    SetModifiedFlag(FALSE);

    // 연결된 뷰에 변경 알림
    UpdateAllViews(nullptr);

    return TRUE;
}

// ============================================================
// AddVisualization / RemoveVisualization / ClearVisualizations
// ============================================================
void CDeepMetriaDoc::AddVisualization(const VisualizationInfo& viz)
{
    m_visualizations.push_back(viz);
    SetModifiedFlag(TRUE);
    UpdateAllViews(nullptr);
}

void CDeepMetriaDoc::RemoveVisualization(const CString& vizId)
{
    auto it = std::remove_if(m_visualizations.begin(), m_visualizations.end(),
        [&vizId](const VisualizationInfo& v) { return v.id == vizId; });

    if (it != m_visualizations.end())
    {
        m_visualizations.erase(it, m_visualizations.end());
        SetModifiedFlag(TRUE);
        UpdateAllViews(nullptr);
    }
}

void CDeepMetriaDoc::ClearVisualizations()
{
    m_visualizations.clear();
    SetModifiedFlag(TRUE);
    UpdateAllViews(nullptr);
}

// ============================================================
// Serialize — 직렬화 (DataTable + DataSummary + Visualizations)
// ============================================================
void CDeepMetriaDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // 기본 정보
        ar << m_dataTable.fileName;
        ar << m_dataTable.rowCount;
        ar << m_dataTable.colCount;

        // 헤더
        ar << (int)m_dataTable.headers.size();
        for (const auto& h : m_dataTable.headers)
            ar << h;

        // 행 데이터
        ar << (int)m_dataTable.rows.size();
        for (const auto& row : m_dataTable.rows)
        {
            ar << (int)row.size();
            for (const auto& cell : row)
                ar << cell;
        }

        // DataSummary
        ar << m_dataSummary.datasourceId;
        ar << m_dataSummary.rowCount;
        ar << m_dataSummary.columnCount;
        ar << (int)m_dataSummary.schema.size();
        for (const auto& cs : m_dataSummary.schema)
        {
            ar << cs.name;
            ar << cs.type;
            ar << cs.index;
            ar << cs.nullCount;
            ar << cs.sampleValues;
            ar << cs.uniqueCount;
            ar << cs.minValue;
            ar << cs.maxValue;
        }

        // Visualizations
        ar << (int)m_visualizations.size();
        for (const auto& viz : m_visualizations)
        {
            ar << viz.id;
            ar << viz.dashboardId;
            ar << viz.vizType;
            ar << viz.title;
            ar << viz.chartConfig.dataJson;
            ar << viz.position.x;
            ar << viz.position.y;
            ar << viz.position.w;
            ar << viz.position.h;
        }
    }
    else
    {
        // 역직렬화 — 저장 순서와 동일하게
        DeleteContents();

        ar >> m_dataTable.fileName;
        ar >> m_dataTable.rowCount;
        ar >> m_dataTable.colCount;

        int headerCount = 0;
        ar >> headerCount;
        m_dataTable.headers.resize(headerCount);
        for (int i = 0; i < headerCount; ++i)
            ar >> m_dataTable.headers[i];

        int rowCountActual = 0;
        ar >> rowCountActual;
        m_dataTable.rows.resize(rowCountActual);
        for (int i = 0; i < rowCountActual; ++i)
        {
            int cellCount = 0;
            ar >> cellCount;
            m_dataTable.rows[i].resize(cellCount);
            for (int j = 0; j < cellCount; ++j)
                ar >> m_dataTable.rows[i][j];
        }

        ar >> m_dataSummary.datasourceId;
        ar >> m_dataSummary.rowCount;
        ar >> m_dataSummary.columnCount;
        int schemaCount = 0;
        ar >> schemaCount;
        m_dataSummary.schema.resize(schemaCount);
        for (int i = 0; i < schemaCount; ++i)
        {
            ar >> m_dataSummary.schema[i].name;
            ar >> m_dataSummary.schema[i].type;
            ar >> m_dataSummary.schema[i].index;
            ar >> m_dataSummary.schema[i].nullCount;
            ar >> m_dataSummary.schema[i].sampleValues;
            ar >> m_dataSummary.schema[i].uniqueCount;
            ar >> m_dataSummary.schema[i].minValue;
            ar >> m_dataSummary.schema[i].maxValue;
        }

        int vizCount = 0;
        ar >> vizCount;
        m_visualizations.resize(vizCount);
        for (int i = 0; i < vizCount; ++i)
        {
            ar >> m_visualizations[i].id;
            ar >> m_visualizations[i].dashboardId;
            ar >> m_visualizations[i].vizType;
            ar >> m_visualizations[i].title;
            ar >> m_visualizations[i].chartConfig.dataJson;
            ar >> m_visualizations[i].position.x;
            ar >> m_visualizations[i].position.y;
            ar >> m_visualizations[i].position.w;
            ar >> m_visualizations[i].position.h;
        }

        // columns 필드 재구성
        m_dataTable.columns.clear();
        for (int c = 0; c < m_dataTable.colCount; ++c)
        {
            DataColumn dc;
            dc.name = (c < (int)m_dataTable.headers.size()) ? m_dataTable.headers[c] : _T("");
            for (const auto& row : m_dataTable.rows)
            {
                if (c < (int)row.size())
                    dc.values.push_back(row[c]);
                else
                    dc.values.push_back(_T(""));
            }
            bool allNumeric = true;
            bool allDate = true;
            bool hasNonEmpty = false;
            for (const auto& v : dc.values)
            {
                if (v.IsEmpty()) continue;
                hasNonEmpty = true;
                if (allDate)
                {
                    bool isDate = false;
                    if (v.GetLength() >= 10)
                    {
                        TCHAR sep = v[4];
                        if ((sep == _T('-') || sep == _T('/') || sep == _T('.')) &&
                            v[7] == sep &&
                            _istdigit(v[0]) && _istdigit(v[1]) &&
                            _istdigit(v[2]) && _istdigit(v[3]) &&
                            _istdigit(v[5]) && _istdigit(v[6]) &&
                            _istdigit(v[8]) && _istdigit(v[9]))
                            isDate = true;
                    }
                    if (!isDate) allDate = false;
                }

                if (allNumeric)
                {
                    TCHAR* endPtr = nullptr;
                    _tcstod(v, &endPtr);
                    if (endPtr == (LPCTSTR)v || *endPtr != _T('\0'))
                        allNumeric = false;
                }
            }
            if (!hasNonEmpty) dc.type = _T("text");
            else if (allDate) dc.type = _T("date");
            else if (allNumeric) dc.type = _T("numeric");
            else dc.type = _T("text");
            m_dataTable.columns.push_back(dc);
        }
    }
}
