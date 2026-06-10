// DeepMetriaDoc.cpp
#include "stdafx.h"
#include "DeepMetria.h"
#include "DeepMetriaDoc.h"
#include "Messages.h"
#include "ParserFactory.h"
#include "CsvParser.h"
#include "JsonParser.h"
#include "AnalysisTool.h"
#include "LLMRouter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CDeepMetriaDoc, CDocument)

BEGIN_MESSAGE_MAP(CDeepMetriaDoc, CDocument)
    ON_COMMAND(ID_AI_SUMMARIZE, &CDeepMetriaDoc::OnAiSummarize)
    ON_COMMAND(ID_AI_ANALYZE,   &CDeepMetriaDoc::OnAiAnalyze)
    ON_COMMAND(ID_EXPORT_PNG,   &CDeepMetriaDoc::OnExportPng)
END_MESSAGE_MAP()

CDeepMetriaDoc::CDeepMetriaDoc() = default;
CDeepMetriaDoc::~CDeepMetriaDoc() = default;

BOOL CDeepMetriaDoc::OnNewDocument()
{
    CDocument::OnNewDocument();
    m_dataSource.Reset();
    m_dashboard.Clear();
    return TRUE;
}

BOOL CDeepMetriaDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    CStringW path(lpszPathName);

    // 마지막 '.' 부터 끝까지 — 모든 확장자 길이 안전 처리
    int dotPos = path.ReverseFind(L'.');
    CStringW ext;
    if (dotPos >= 0) ext = path.Mid(dotPos);  // ".csv", ".json", ".xlsx" 등
    ext.MakeLower();

    auto parser = deepmetria::ParserFactory::CreateForExtension(
        std::wstring(ext.GetString()));
    if (!parser)
    {
        if (ext == L".xlsx" || ext == L".xls")
            AfxMessageBox(_T("Excel 파일 지원은 곧 추가됩니다 (CSV로 저장 후 다시 시도)."));
        else
            AfxMessageBox(_T("지원하지 않는 파일 형식입니다. CSV / JSON 만 지원합니다."));
        return FALSE;
    }

    if (!parser->Parse(std::wstring(path.GetString()), m_dataSource))
    {
        CString detail;
        if (m_dataSource.Columns().empty() && m_dataSource.Rows().empty())
            detail = _T("파일이 비었거나 헤더가 없습니다.");
        else
            detail = _T("형식이 올바른지 확인하세요.");
        AfxMessageBox(_T("파일 파싱에 실패했습니다.\n") + detail);
        return FALSE;
    }

    // 자동 요약 트리거 (동기 로컬 요약 후 WM_USER_SUMMARY_READY 포스트)
    OnAiSummarize();
    SetPathName(lpszPathName, FALSE);
    return TRUE;
}

void CDeepMetriaDoc::Serialize(CArchive& /*ar*/)
{
    // SQLite 저장으로 대체 예정 (post-MVP)
}

void CDeepMetriaDoc::OnAiSummarize()
{
    deepmetria::AnalysisTool::ComputeBasicStatistics(m_dataSource);
    deepmetria::AnalysisTool::InferSchemaTypes(m_dataSource);

    if (CFrameWnd* p = static_cast<CFrameWnd*>(AfxGetMainWnd()))
        p->PostMessage(deepmetria::WM_USER_SUMMARY_READY, 0, 0);
}

void CDeepMetriaDoc::OnAiAnalyze()
{
    AfxMessageBox(_T("자연어 질문은 우측 패널에서 입력하세요."));
}

void CDeepMetriaDoc::OnExportPng()
{
    if (CFrameWnd* p = static_cast<CFrameWnd*>(AfxGetMainWnd()))
        p->PostMessage(deepmetria::WM_USER_EXPORT_REQUEST, 0, 0);
}

bool CDeepMetriaDoc::SubmitNaturalLanguageQuery(const CStringW& question)
{
    if (m_dataSource.IsEmpty())
    {
        AfxMessageBox(_T("먼저 데이터 파일을 불러오세요."));
        return false;
    }
    auto router = theApp.m_llmRouter;
    if (!router || !router->IsConfigured())
    {
        AfxMessageBox(
            _T("LLM이 설정되지 않았습니다.\n")
            _T("리본 우측 [설정]에서 제공자/API 키를 등록한 뒤 다시 시도하세요."),
            MB_OK | MB_ICONWARNING);
        return false;
    }
    if (m_busy)
    {
        AfxMessageBox(
            _T("이전 분석이 아직 진행 중입니다.\n완료된 뒤 다음 질문을 보내주세요."),
            MB_OK | MB_ICONINFORMATION);
        return false;
    }

    m_busy = true;
    m_currentFlow = std::make_shared<deepmetria::AnalysisFlow>(
        m_dataSource, m_dashboard, router);

    auto self = this;
    m_currentFlow->Start(
        std::wstring(question.GetString()),
        [](int pct)
        {
            if (CFrameWnd* p = static_cast<CFrameWnd*>(AfxGetMainWnd()))
                p->PostMessage(deepmetria::WM_USER_ANALYSIS_PROGRESS,
                               static_cast<WPARAM>(pct), 0);
        },
        [self]()
        {
            self->m_busy = false;
            if (CFrameWnd* p = static_cast<CFrameWnd*>(AfxGetMainWnd()))
                p->PostMessage(deepmetria::WM_USER_VISUALIZATION_READY, 0, 0);
        });
    return true;
}
