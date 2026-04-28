#pragma once

// AI 오케스트레이터 — CoT(Chain-of-Thought) 분석 흐름 관리
// Architecture §5 / DetailedSpec §4 참조

#include "../../Common/Types.h"
#include "AnalysisFlow.h"
#include <atomic>   // std::atomic — 스레드 안전 상태 관리

// 전방 선언 (Infrastructure 레이어 — .cpp에서 include)
class LLMRouter;

// Worker Thread 파라미터 구조체
struct OrchestratorThreadParam {
    const DataTable*      pData;          // 입력 데이터 (소유권 없음)
    const DataSummary*    pSummary;       // 데이터 요약 (소유권 없음)
    CString               question;       // 사용자 질문
    HWND                  hNotifyWnd;    // 진행/완료 통보 대상 윈도우
    AnalysisFlow*         pFlow;          // 플로우 상태 (호출자가 소유)
    class AnalysisOrchestrator* pOrchestrator; // 취소 플래그 확인용 (소유권 없음)
};

// ============================================================
// AnalysisOrchestrator — 4단계 CoT 분석 흐름 관리
//   1단계: 질문 + 스키마 → LLM → 분석 계획 수신
//   2단계: 분석 도구 선택 및 AnalysisTools 호출
//   3단계: 결과 + 원본 데이터 → LLM → 인사이트 생성
//   4단계: ChartSelector로 차트 유형 결정
// ============================================================
class AnalysisOrchestrator {
public:
    AnalysisOrchestrator();
    ~AnalysisOrchestrator();

    // Worker Thread를 생성하여 비동기 분석 시작
    // 각 단계마다: PostMessage(hNotifyWnd, WM_ANALYSIS_PROGRESS, step, 0)
    // 완료 시:    PostMessage(hNotifyWnd, WM_ANALYSIS_DONE, 0, (LPARAM)pResult)
    // pResult는 heap 할당된 AnalysisFlow* (수신 측에서 delete)
    BOOL AnalyzeQuestion(const DataTable&   data,
                         const DataSummary& summary,
                         const CString&     question,
                         HWND               hNotifyWnd);

    // 진행 중인 분석 취소 요청
    void CancelAnalysis();

    // 취소 여부 확인 (Worker Thread에서 호출)
    BOOL IsCancelled() const { return m_bCancelled.load(); }

    // 현재 플로우 상태 조회 (스레드 안전 — atomic 읽기)
    AnalysisFlowState GetCurrentState() const;

private:
    // AfxBeginThread 진입점
    static UINT AnalysisThreadProc(LPVOID pParam);

    // 각 단계별 실행 메서드 (스레드 내부)
    static BOOL Step1_Plan(AnalysisFlow&     flow,
                           const DataTable&  data,
                           const DataSummary& summary,
                           HWND              hNotifyWnd,
                           AppError&         outError);

    static BOOL Step2_Execute(AnalysisFlow&    flow,
                              const DataTable& data,
                              HWND             hNotifyWnd,
                              AppError&        outError);

    static BOOL Step3_Interpret(AnalysisFlow&    flow,
                                const DataTable& data,
                                HWND             hNotifyWnd,
                                AppError&        outError);

    static BOOL Step4_Visualize(AnalysisFlow&    flow,
                                const DataTable& data,
                                HWND             hNotifyWnd,
                                AppError&        outError);

    // 스키마를 LLM 프롬프트용 JSON으로 직렬화
    static CString BuildSchemaJson(const DataSummary& summary);

    // LLM 응답에서 도구명과 파라미터 파싱
    static BOOL ParsePlanResponse(const CString& llmResponse,
                                  CString&       outToolName,
                                  CString&       outToolParams);

    // 도구명 + 파라미터 JSON으로 AnalysisTools 디스패치
    static CString DispatchTool(const CString&   toolName,
                                const CString&   toolParamsJson,
                                const DataTable& data);

    std::atomic<BOOL>             m_bCancelled;  // 취소 플래그 — 스레드 안전 atomic 읽기/쓰기
    std::atomic<AnalysisFlowState> m_state;      // 현재 상태 — 스레드 안전 atomic 읽기/쓰기
    CWinThread*                    m_pThread;    // 실행 중인 Worker Thread
};
