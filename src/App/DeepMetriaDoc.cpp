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
        AfxMessageBox(
            _T("지원하지 않는 파일 형식입니다.\n")
            _T("지원: CSV / JSON / Excel (.xlsx, .xls, .xlsm)"));
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

    // 채팅 이력에 사용자 질문 누적 (LLM 컨텍스트 + UI 채팅 동시)
    AppendChatTurn(true, std::wstring(question.GetString()));

    m_busy = true;
    m_currentFlow = std::make_shared<deepmetria::AnalysisFlow>(
        m_dataSource, m_dashboard, router);

    // 이전 대화 이력을 LLMRouter::ChatTurn 형식으로 변환해 주입
    std::vector<deepmetria::LLMRouter::ChatTurn> hist;
    hist.reserve(m_chatHistory.size());
    for (const auto& t : m_chatHistory)
        hist.push_back({ t.isUser, t.text });
    m_currentFlow->SetHistory(std::move(hist));

    auto self = this;
    // 콜백에서 m_currentFlow 멤버를 직접 참조하면 새 분석 시작 시 reset 으로 인한 race 가
    // 발생할 수 있다. flow 자체를 캡처해 콜백 수명을 명시적으로 보장한다.
    auto flow = m_currentFlow;
    m_currentFlow->Start(
        std::wstring(question.GetString()),
        [](int pct)
        {
            // worker thread → AfxGetMainWnd() 는 thread-local 이라 null 반환.
            // process-wide m_pMainWnd 사용해야 PostMessage 가 UI 큐에 도달.
            CWinApp* app = AfxGetApp();
            CWnd* mw = app ? app->m_pMainWnd : nullptr;
            if (CFrameWnd* p = DYNAMIC_DOWNCAST(CFrameWnd, mw))
                p->PostMessage(deepmetria::WM_USER_ANALYSIS_PROGRESS,
                               static_cast<WPARAM>(pct), 0);
        },
        [self, flow]()
        {
            self->m_busy = false;

            // worker thread → AfxGetMainWnd() 대신 process-wide m_pMainWnd 사용.
            CWinApp* appPtr = AfxGetApp();
            CWnd*    mwPtr  = appPtr ? appPtr->m_pMainWnd : nullptr;
            CFrameWnd* pFrame = DYNAMIC_DOWNCAST(CFrameWnd, mwPtr);

            // LLM 호출 실패 — 시각화/채팅 추가 안 하고 에러 다이얼로그 띄움
            const std::wstring& err = flow->LastError();
            if (!err.empty())
            {
                // PostMessage 후 lifetime 보장 — static buffer
                static CStringW s_lastErr;
                s_lastErr = err.c_str();
                if (pFrame)
                    pFrame->PostMessage(deepmetria::WM_USER_LLM_ERROR,
                                        0, reinterpret_cast<LPARAM>(s_lastErr.GetString()));
                return;
            }

            // 정상 응답 — AI description 을 채팅 이력에 누적
            // 정책: description 은 LLM 이 만든 것만 표시. 비어있으면 추가하지 않음.
            const std::wstring& aiText = flow->LastDescription();
            if (!aiText.empty())
                self->AppendChatTurn(false, aiText);

            if (pFrame)
                pFrame->PostMessage(deepmetria::WM_USER_VISUALIZATION_READY, 0, 0);
        });
    return true;
}
