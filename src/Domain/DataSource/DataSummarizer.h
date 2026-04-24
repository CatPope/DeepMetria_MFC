#pragma once

// AI 데이터 요약 생성 (Worker Thread 실행)
// Architecture §4.1 / DetailedSpec §4.5 참조

#include "../../Common/Types.h"

// 전방 선언 (Infrastructure 레이어 — .cpp에서 include)
class LLMRouter;

// Worker Thread 파라미터 구조체
struct SummarizerThreadParam {
    const DataTable* pData;        // 입력 데이터 (소유권 없음)
    DataSummary*     pOutSummary;  // 출력 요약 (호출자가 소유)
    HWND             hNotifyWnd;   // 완료 통보 대상 윈도우
    AppError*        pOutError;    // 출력 에러 (호출자가 소유)
};

// ============================================================
// DataSummarizer — AnalysisTools::BasicStats + LLM 요약 생성
// ============================================================
class DataSummarizer {
public:
    DataSummarizer();
    ~DataSummarizer();

    // Worker Thread를 생성하여 비동기 요약 시작
    // 완료 시 PostMessage(hNotifyWnd, WM_DATA_LOADED, 0, (LPARAM)pOutSummary)
    BOOL Summarize(const DataTable& data,
                   DataSummary&     outSummary,
                   HWND             hNotifyWnd,
                   AppError&        outError);

private:
    // AfxBeginThread 진입점
    static UINT SummarizerThreadProc(LPVOID pParam);

    // 실제 요약 로직 (스레드 내부 실행)
    static BOOL DoSummarize(const DataTable& data,
                            DataSummary&     outSummary,
                            HWND             hNotifyWnd,
                            AppError&        outError);

    // 스키마 + 통계를 LLM 프롬프트 문자열로 직렬화
    static CString BuildSummaryPrompt(const DataTable& data,
                                      const CString&   statsJson);
};
