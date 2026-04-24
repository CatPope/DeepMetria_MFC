#include "stdafx.h"
#include "DataSummarizer.h"

// Infrastructure 레이어 — .cpp에서만 include
#include "../../Infrastructure/LLM/LLMRouter.h"

// Domain 레이어 (같은 레이어 내 직접 include)
#include "../Analysis/AnalysisTools.h"

// ============================================================
// DataSummarizer 구현
// Architecture §4.1 / DetailedSpec §4.5 참조
// ============================================================

DataSummarizer::DataSummarizer()
{
}

DataSummarizer::~DataSummarizer()
{
}

BOOL DataSummarizer::Summarize(const DataTable& data,
                                DataSummary&     outSummary,
                                HWND             hNotifyWnd,
                                AppError&        outError)
{
    // Worker Thread 파라미터 구성 (heap 할당 — 스레드 종료 시 delete)
    SummarizerThreadParam* pParam = new SummarizerThreadParam();
    pParam->pData       = &data;
    pParam->pOutSummary = &outSummary;
    pParam->hNotifyWnd  = hNotifyWnd;
    pParam->pOutError   = &outError;

    CWinThread* pThread = AfxBeginThread(
        SummarizerThreadProc,
        pParam,
        THREAD_PRIORITY_NORMAL,
        0,
        CREATE_SUSPENDED
    );

    if (!pThread) {
        delete pParam;
        outError = AppError(
            _T("THREAD_CREATE_FAILED"),
            _T("요약 스레드를 생성할 수 없습니다."),
            2
        );
        return FALSE;
    }

    pThread->m_bAutoDelete = TRUE;
    pThread->ResumeThread();
    return TRUE;
}

UINT DataSummarizer::SummarizerThreadProc(LPVOID pParam)
{
    SummarizerThreadParam* p = static_cast<SummarizerThreadParam*>(pParam);

    BOOL result = DoSummarize(*p->pData, *p->pOutSummary,
                              p->hNotifyWnd, *p->pOutError);

    // 완료 메시지 전송 (WM_DATA_LOADED, LPARAM = DataSummary 포인터)
    // 수신 측에서 내용을 복사 후 삭제할 필요 없음 (outSummary는 호출자 소유)
    if (::IsWindow(p->hNotifyWnd))
        ::PostMessage(p->hNotifyWnd, WM_DATA_LOADED, (WPARAM)result, 0);

    delete p;
    return 0;
}

BOOL DataSummarizer::DoSummarize(const DataTable& data,
                                  DataSummary&     outSummary,
                                  HWND             hNotifyWnd,
                                  AppError&        outError)
{
    // 1. 기본 메타 정보 채우기
    outSummary.rowCount    = data.rowCount;
    outSummary.columnCount = static_cast<int>(data.columns.size());

    // 2. 컬럼 스키마 구성
    outSummary.schema.clear();
    for (const auto& col : data.columns) {
        ColumnSchema cs;
        cs.name = col.name;
        cs.type = col.type;

        // 결측치 수 계산
        cs.nullCount = 0;
        for (const auto& v : col.values) {
            if (v.IsEmpty()) cs.nullCount++;
        }

        // 샘플값 최대 5개 (콤마 구분)
        int sampleCount = min(5, (int)col.values.size());
        for (int i = 0; i < sampleCount; ++i) {
            if (i > 0) cs.sampleValues += _T(", ");
            cs.sampleValues += col.values[i];
        }

        outSummary.schema.push_back(cs);
    }

    // 3. AnalysisTools::BasicStats 호출하여 기본 통계 수집
    CString statsJson = AnalysisTools::BasicStats(data);

    // 4. LLM에 스키마 + 통계 전송하여 자연어 요약 생성
    CString prompt = BuildSummaryPrompt(data, statsJson);

    LLMRouter llmRouter;
    AppError  llmError;
    CString   llmResponse;

    if (!llmRouter.Chat(prompt, llmResponse, llmError)) {
        // LLM 실패 시 기본 요약 텍스트 사용 (치명적 오류 아님)
        outSummary.aiSummaryText.Format(
            _T("데이터셋 요약: %d행 %d열. LLM 요약 생성 실패."),
            data.rowCount, (int)data.columns.size()
        );
    } else {
        outSummary.aiSummaryText = llmResponse;
    }

    return TRUE;
}

CString DataSummarizer::BuildSummaryPrompt(const DataTable& data,
                                            const CString&   statsJson)
{
    // 컬럼 목록 직렬화
    CString schemaText;
    for (const auto& col : data.columns) {
        CString colLine;
        colLine.Format(_T("  - %s (%s)\n"), (LPCTSTR)col.name, (LPCTSTR)col.type);
        schemaText += colLine;
    }

    CString prompt;
    prompt.Format(
        _T("다음 데이터셋을 분석하여 한국어로 간결하게 요약해주세요.\n\n")
        _T("파일명: %s\n")
        _T("행 수: %d\n")
        _T("컬럼 목록:\n%s\n")
        _T("기본 통계 (JSON):\n%s\n\n")
        _T("요약은 3-5문장으로, 데이터의 주요 특성과 도메인을 설명하세요."),
        (LPCTSTR)data.fileName,
        data.rowCount,
        (LPCTSTR)schemaText,
        (LPCTSTR)statsJson
    );

    return prompt;
}
