// DeepMetriaDoc.cpp — CDeepMetriaDoc 구현
// 파일 로드, 데이터 모델 관리, 직렬화 담당

#include "stdafx.h"
#include "DeepMetriaDoc.h"

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

    // ---- 파서 호출 (스켈레톤: 실제 파서는 Infrastructure/Parser에서 구현) ----
    // CSV:  CSVParser::Parse(filePath, m_dataTable, outError)
    // XLSX: ExcelParser::Parse(filePath, m_dataTable, outError)
    if (ext != _T("csv") && ext != _T("xlsx") && ext != _T("xls"))
    {
        outError = AppError(_T("UNSUPPORTED_FILE_TYPE"),
                            _T("지원하지 않는 파일 형식입니다: .") + ext, 2);
        return FALSE;
    }

    // 스켈레톤: 파일 이름만 DataTable에 기록
    m_dataTable          = DataTable();
    m_dataTable.fileName = filePath;
    m_dataTable.rowCount = 0;

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
// Serialize — 직렬화 (현재 스켈레톤: 기본 구현만 유지)
// ============================================================
void CDeepMetriaDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // 저장: 추후 구현
        ar << m_dataTable.fileName;
        ar << m_dataTable.rowCount;
    }
    else
    {
        // 불러오기: 추후 구현
        ar >> m_dataTable.fileName;
        ar >> m_dataTable.rowCount;
    }
}
