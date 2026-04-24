#include "stdafx.h"
#include "AnalysisOrchestrator.h"

// Infrastructure 레이어 — .cpp에서만 include
#include "../../Infrastructure/LLM/LLMRouter.h"

// Domain 레이어 (같은 레이어 내 직접 include)
#include "../Analysis/AnalysisTools.h"
#include "../Analysis/ChartSelector.h"

// ============================================================
// AnalysisOrchestrator 구현
// Architecture §5 / DetailedSpec §4 CoT 흐름 참조
// ============================================================

AnalysisOrchestrator::AnalysisOrchestrator()
    : m_bCancelled(FALSE)
    , m_state(AnalysisFlowState::Idle)
    , m_pThread(nullptr)
{
}

AnalysisOrchestrator::~AnalysisOrchestrator()
{
    CancelAnalysis();
}

BOOL AnalysisOrchestrator::AnalyzeQuestion(const DataTable&   data,
                                            const DataSummary& summary,
                                            const CString&     question,
                                            HWND               hNotifyWnd)
{
    if (m_state != AnalysisFlowState::Idle &&
        m_state != AnalysisFlowState::Done  &&
        m_state != AnalysisFlowState::Error) {
        // 이미 실행 중
        return FALSE;
    }

    m_bCancelled = FALSE;
    m_state      = AnalysisFlowState::Planning;

    // Worker Thread 파라미터 (heap 할당 — 스레드 종료 시 delete)
    OrchestratorThreadParam* pParam = new OrchestratorThreadParam();
    pParam->pData      = &data;
    pParam->pSummary   = &summary;
    pParam->question   = question;
    pParam->hNotifyWnd = hNotifyWnd;
    // AnalysisFlow는 스레드가 heap에 직접 생성하여 완료 메시지 LPARAM으로 전달
    pParam->pFlow = nullptr;

    m_pThread = AfxBeginThread(
        AnalysisThreadProc,
        pParam,
        THREAD_PRIORITY_NORMAL,
        0,
        CREATE_SUSPENDED
    );

    if (!m_pThread) {
        delete pParam;
        m_state = AnalysisFlowState::Error;
        return FALSE;
    }

    m_pThread->m_bAutoDelete = TRUE;
    m_pThread->ResumeThread();
    return TRUE;
}

void AnalysisOrchestrator::CancelAnalysis()
{
    m_bCancelled = TRUE;
    // Worker Thread는 다음 단계 진입 전 m_bCancelled를 확인하여 조기 종료
}

AnalysisFlowState AnalysisOrchestrator::GetCurrentState() const
{
    return m_state;
}

// ============================================================
// Worker Thread 진입점
// ============================================================

UINT AnalysisOrchestrator::AnalysisThreadProc(LPVOID pParam)
{
    OrchestratorThreadParam* p = static_cast<OrchestratorThreadParam*>(pParam);

    // heap 할당 — 완료 메시지 LPARAM으로 수신 측에 전달, 수신 측에서 delete
    AnalysisFlow* pFlow = new AnalysisFlow();
    pFlow->question = p->question;

    HWND hWnd = p->hNotifyWnd;

    // ── 1단계: 분석 계획 수립 ──────────────────────────────
    pFlow->state = AnalysisFlowState::Planning;
    if (::IsWindow(hWnd))
        ::PostMessage(hWnd, WM_ANALYSIS_PROGRESS, 1, 0);

    AppError err;
    if (!Step1_Plan(*pFlow, *p->pData, *p->pSummary, hWnd, err)) {
        pFlow->SetError(err);
        if (::IsWindow(hWnd))
            ::PostMessage(hWnd, WM_ANALYSIS_DONE, 0, reinterpret_cast<LPARAM>(pFlow));
        delete p;
        return 1;
    }

    // ── 2단계: 분석 도구 실행 ─────────────────────────────
    pFlow->state = AnalysisFlowState::Executing;
    if (::IsWindow(hWnd))
        ::PostMessage(hWnd, WM_ANALYSIS_PROGRESS, 2, 0);

    if (!Step2_Execute(*pFlow, *p->pData, hWnd, err)) {
        pFlow->SetError(err);
        if (::IsWindow(hWnd))
            ::PostMessage(hWnd, WM_ANALYSIS_DONE, 0, reinterpret_cast<LPARAM>(pFlow));
        delete p;
        return 1;
    }

    // ── 3단계: 인사이트 생성 ──────────────────────────────
    pFlow->state = AnalysisFlowState::Interpreting;
    if (::IsWindow(hWnd))
        ::PostMessage(hWnd, WM_ANALYSIS_PROGRESS, 3, 0);

    if (!Step3_Interpret(*pFlow, *p->pData, hWnd, err)) {
        pFlow->SetError(err);
        if (::IsWindow(hWnd))
            ::PostMessage(hWnd, WM_ANALYSIS_DONE, 0, reinterpret_cast<LPARAM>(pFlow));
        delete p;
        return 1;
    }

    // ── 4단계: 차트 유형 결정 ─────────────────────────────
    pFlow->state = AnalysisFlowState::Visualizing;
    if (::IsWindow(hWnd))
        ::PostMessage(hWnd, WM_ANALYSIS_PROGRESS, 4, 0);

    if (!Step4_Visualize(*pFlow, *p->pData, hWnd, err)) {
        pFlow->SetError(err);
        if (::IsWindow(hWnd))
            ::PostMessage(hWnd, WM_ANALYSIS_DONE, 0, reinterpret_cast<LPARAM>(pFlow));
        delete p;
        return 1;
    }

    // ── 완료 ──────────────────────────────────────────────
    pFlow->state = AnalysisFlowState::Done;
    if (::IsWindow(hWnd))
        ::PostMessage(hWnd, WM_ANALYSIS_DONE, 0, reinterpret_cast<LPARAM>(pFlow));

    delete p;
    return 0;
}

// ============================================================
// Step 1: 질문 + 스키마 → LLM → 분석 계획 수신
// ============================================================

BOOL AnalysisOrchestrator::Step1_Plan(AnalysisFlow&     flow,
                                       const DataTable&  data,
                                       const DataSummary& summary,
                                       HWND              hNotifyWnd,
                                       AppError&         outError)
{
    // 스키마 JSON 직렬화
    CString schemaJson = BuildSchemaJson(summary);

    // 시스템 프롬프트 (DetailedSpec §4.3)
    CString systemPrompt =
        _T("당신은 DeepMetria의 데이터 분석 오케스트레이터입니다.\n")
        _T("사용자의 자연어 질문을 분석하여 아래 도구 중 하나를 선택하고 ")
        _T("JSON 형식으로 응답하세요.\n\n")
        _T("사용 가능한 도구:\n")
        _T("BasicStats, GroupByAggregate, TimeSeriesAnalysis, CorrelationMatrix,\n")
        _T("TopN, FrequencyDistribution, CrossTabulation, MovingAverage,\n")
        _T("Percentile, DateGroupAggregate, Filtering, PivotTable, OutlierDetection\n\n")
        _T("응답 형식 (JSON만 반환):\n")
        _T("{\"tool\":\"<도구명>\",\"params\":{<파라미터>},\"reasoning\":\"<선택 이유>\"}\n\n");

    CString userPrompt;
    userPrompt.Format(
        _T("데이터 스키마:\n%s\n\n사용자 질문: %s"),
        (LPCTSTR)schemaJson, (LPCTSTR)flow.question
    );

    CString fullPrompt = systemPrompt + userPrompt;

    LLMRouter llmRouter;
    CString   llmResponse;

    if (!llmRouter.Chat(fullPrompt, llmResponse, outError))
        return FALSE;

    flow.plan = llmResponse;

    // 도구명 및 파라미터 파싱
    if (!ParsePlanResponse(llmResponse, flow.toolName, flow.toolParams)) {
        outError = AppError(
            _T("PLAN_PARSE_ERROR"),
            _T("LLM 분석 계획을 파싱할 수 없습니다."),
            2
        );
        return FALSE;
    }

    return TRUE;
}

// ============================================================
// Step 2: 분석 도구 선택 및 AnalysisTools 호출
// ============================================================

BOOL AnalysisOrchestrator::Step2_Execute(AnalysisFlow&    flow,
                                          const DataTable& data,
                                          HWND             hNotifyWnd,
                                          AppError&        outError)
{
    flow.rawResult = DispatchTool(flow.toolName, flow.toolParams, data);

    if (flow.rawResult.IsEmpty()) {
        outError = AppError(
            _T("TOOL_EXECUTE_FAILED"),
            _T("분석 도구 실행 결과가 없습니다."),
            2
        );
        return FALSE;
    }

    // 오류 JSON 반환 확인
    if (flow.rawResult.Find(_T("\"error\"")) >= 0) {
        outError = AppError(
            _T("TOOL_ERROR"),
            _T("분석 도구 실행 중 오류가 발생했습니다: ") + flow.rawResult,
            1 // warning — 계속 진행 가능
        );
        // 치명적이지 않으므로 TRUE 반환
    }

    return TRUE;
}

// ============================================================
// Step 3: 결과 + 원본 데이터 → LLM → 인사이트 생성
// ============================================================

BOOL AnalysisOrchestrator::Step3_Interpret(AnalysisFlow&    flow,
                                            const DataTable& data,
                                            HWND             hNotifyWnd,
                                            AppError&        outError)
{
    CString prompt;
    prompt.Format(
        _T("다음 데이터 분석 결과를 바탕으로 비즈니스 인사이트를 한국어로 3-5문장으로 설명하세요.\n\n")
        _T("사용자 질문: %s\n")
        _T("사용된 분석 도구: %s\n")
        _T("분석 결과 (JSON):\n%s\n\n")
        _T("인사이트는 데이터 수치를 근거로 구체적으로 작성하세요."),
        (LPCTSTR)flow.question,
        (LPCTSTR)flow.toolName,
        (LPCTSTR)flow.rawResult
    );

    LLMRouter llmRouter;
    CString   llmResponse;

    if (!llmRouter.Chat(prompt, llmResponse, outError)) {
        // LLM 실패 시 기본 인사이트 텍스트 사용 (치명적이지 않음)
        flow.insight = _T("분석이 완료되었습니다. 결과를 차트로 확인하세요.");
        return TRUE;
    }

    flow.insight = llmResponse;
    return TRUE;
}

// ============================================================
// Step 4: ChartSelector로 차트 유형 결정
// ============================================================

BOOL AnalysisOrchestrator::Step4_Visualize(AnalysisFlow&    flow,
                                            const DataTable& data,
                                            HWND             hNotifyWnd,
                                            AppError&        outError)
{
    ChartSelector::SelectChart(
        flow.toolName,
        flow.rawResult,
        data,
        flow.chartConfig
    );

    // 인사이트를 차트 제목에 반영 (첫 50자)
    if (!flow.insight.IsEmpty()) {
        CString shortInsight = flow.insight.Left(50);
        if (flow.insight.GetLength() > 50) shortInsight += _T("...");
        // chartConfig.title은 ChartSelector가 설정 — 덮어쓰지 않음
    }

    return TRUE;
}

// ============================================================
// 유틸리티 메서드
// ============================================================

CString AnalysisOrchestrator::BuildSchemaJson(const DataSummary& summary)
{
    CString json;
    json.Format(_T("{\"row_count\":%d,\"column_count\":%d,\"columns\":["),
                summary.rowCount, summary.columnCount);

    for (size_t i = 0; i < summary.schema.size(); ++i) {
        if (i > 0) json += _T(",");
        const ColumnSchema& cs = summary.schema[i];
        CString entry;
        entry.Format(_T("{\"name\":\"%s\",\"type\":\"%s\",\"null_count\":%d}"),
                     (LPCTSTR)cs.name, (LPCTSTR)cs.type, cs.nullCount);
        json += entry;
    }

    json += _T("]}");
    return json;
}

BOOL AnalysisOrchestrator::ParsePlanResponse(const CString& llmResponse,
                                              CString&       outToolName,
                                              CString&       outToolParams)
{
    // JSON에서 "tool" 값 추출
    int toolPos = llmResponse.Find(_T("\"tool\":\""));
    if (toolPos < 0) return FALSE;

    toolPos += (int)_tcslen(_T("\"tool\":\""));
    int toolEnd = llmResponse.Find(_T("\""), toolPos);
    if (toolEnd < 0) return FALSE;

    outToolName = llmResponse.Mid(toolPos, toolEnd - toolPos);
    outToolName.Trim();

    // "params" 블록 추출 (중첩 {} 처리)
    int paramsPos = llmResponse.Find(_T("\"params\":"));
    if (paramsPos < 0) {
        outToolParams = _T("{}");
        return !outToolName.IsEmpty();
    }

    paramsPos += (int)_tcslen(_T("\"params\":"));
    // 공백 건너뜀
    while (paramsPos < llmResponse.GetLength() &&
           llmResponse[paramsPos] == _T(' ')) paramsPos++;

    if (paramsPos >= llmResponse.GetLength() || llmResponse[paramsPos] != _T('{')) {
        outToolParams = _T("{}");
        return !outToolName.IsEmpty();
    }

    // 중첩 {} 매칭
    int depth = 0;
    int start = paramsPos;
    for (int i = paramsPos; i < llmResponse.GetLength(); ++i) {
        if (llmResponse[i] == _T('{'))      depth++;
        else if (llmResponse[i] == _T('}')) depth--;
        if (depth == 0) {
            outToolParams = llmResponse.Mid(start, i - start + 1);
            break;
        }
    }

    if (outToolParams.IsEmpty()) outToolParams = _T("{}");
    return !outToolName.IsEmpty();
}

CString AnalysisOrchestrator::DispatchTool(const CString&   toolName,
                                            const CString&   toolParamsJson,
                                            const DataTable& data)
{
    // 파라미터 JSON에서 값 추출 헬퍼 람다
    auto extractStr = [&](const CString& key) -> CString {
        CString search = _T("\"") + key + _T("\":\"");
        int p = toolParamsJson.Find(search);
        if (p < 0) return _T("");
        p += search.GetLength();
        int q = toolParamsJson.Find(_T("\""), p);
        return (q >= 0) ? toolParamsJson.Mid(p, q - p) : _T("");
    };
    auto extractInt = [&](const CString& key, int defaultVal) -> int {
        CString search = _T("\"") + key + _T("\":");
        int p = toolParamsJson.Find(search);
        if (p < 0) return defaultVal;
        p += search.GetLength();
        if (p < toolParamsJson.GetLength() && toolParamsJson[p] == _T('"'))
            return defaultVal;
        return _ttoi(toolParamsJson.Mid(p));
    };
    auto extractDbl = [&](const CString& key, double defaultVal) -> double {
        CString search = _T("\"") + key + _T("\":");
        int p = toolParamsJson.Find(search);
        if (p < 0) return defaultVal;
        p += search.GetLength();
        if (p < toolParamsJson.GetLength() && toolParamsJson[p] == _T('"'))
            return defaultVal;
        return _tcstod(toolParamsJson.Mid(p), nullptr);
    };
    auto extractBool = [&](const CString& key, bool defaultVal) -> bool {
        CString search = _T("\"") + key + _T("\":");
        int p = toolParamsJson.Find(search);
        if (p < 0) return defaultVal;
        p += search.GetLength();
        CString val = toolParamsJson.Mid(p, 5);
        val.MakeLower();
        if (val.Left(4) == _T("true"))  return true;
        if (val.Left(5) == _T("false")) return false;
        return defaultVal;
    };

    // 도구 디스패치
    if (toolName.CompareNoCase(_T("BasicStats")) == 0) {
        return AnalysisTools::BasicStats(data);
    }
    else if (toolName.CompareNoCase(_T("GroupByAggregate")) == 0) {
        return AnalysisTools::GroupByAggregate(
            data,
            extractStr(_T("groupCol")),
            extractStr(_T("valueCol")),
            extractStr(_T("aggFunc")).IsEmpty() ? _T("sum") : extractStr(_T("aggFunc"))
        );
    }
    else if (toolName.CompareNoCase(_T("TimeSeriesAnalysis")) == 0) {
        return AnalysisTools::TimeSeriesAnalysis(
            data,
            extractStr(_T("dateCol")),
            extractStr(_T("valueCol"))
        );
    }
    else if (toolName.CompareNoCase(_T("CorrelationMatrix")) == 0) {
        // numericCols 배열 파싱 (간단 구현)
        std::vector<CString> cols;
        int arrStart = toolParamsJson.Find(_T("\"numericCols\":["));
        if (arrStart >= 0) {
            arrStart += (int)_tcslen(_T("\"numericCols\":["));
            int arrEnd = toolParamsJson.Find(_T("]"), arrStart);
            if (arrEnd >= 0) {
                CString arrStr = toolParamsJson.Mid(arrStart, arrEnd - arrStart);
                // "col1","col2" 형식 파싱
                int i = 0;
                while (i < arrStr.GetLength()) {
                    int q1 = arrStr.Find(_T("\""), i);
                    if (q1 < 0) break;
                    int q2 = arrStr.Find(_T("\""), q1 + 1);
                    if (q2 < 0) break;
                    cols.push_back(arrStr.Mid(q1 + 1, q2 - q1 - 1));
                    i = q2 + 1;
                }
            }
        }
        // 빈 경우 모든 수치 컬럼 사용
        if (cols.empty()) {
            for (const auto& c : data.columns)
                if (c.type == _T("numeric")) cols.push_back(c.name);
        }
        return AnalysisTools::CorrelationMatrix(data, cols);
    }
    else if (toolName.CompareNoCase(_T("TopN")) == 0) {
        return AnalysisTools::TopN(
            data,
            extractStr(_T("col")),
            extractInt(_T("n"), 10),
            extractBool(_T("ascending"), false)
        );
    }
    else if (toolName.CompareNoCase(_T("FrequencyDistribution")) == 0) {
        return AnalysisTools::FrequencyDistribution(data, extractStr(_T("col")));
    }
    else if (toolName.CompareNoCase(_T("CrossTabulation")) == 0) {
        return AnalysisTools::CrossTabulation(
            data,
            extractStr(_T("rowCol")),
            extractStr(_T("colCol"))
        );
    }
    else if (toolName.CompareNoCase(_T("MovingAverage")) == 0) {
        return AnalysisTools::MovingAverage(
            data,
            extractStr(_T("col")),
            extractInt(_T("window"), 7)
        );
    }
    else if (toolName.CompareNoCase(_T("Percentile")) == 0) {
        std::vector<double> pcts = {25.0, 50.0, 75.0, 90.0, 95.0};
        return AnalysisTools::Percentile(data, extractStr(_T("col")), pcts);
    }
    else if (toolName.CompareNoCase(_T("DateGroupAggregate")) == 0) {
        CString period = extractStr(_T("period"));
        if (period.IsEmpty()) period = _T("month");
        return AnalysisTools::DateGroupAggregate(
            data,
            extractStr(_T("dateCol")),
            extractStr(_T("valueCol")),
            period
        );
    }
    else if (toolName.CompareNoCase(_T("Filtering")) == 0) {
        return AnalysisTools::Filtering(
            data,
            extractStr(_T("col")),
            extractStr(_T("op")),
            extractStr(_T("value"))
        );
    }
    else if (toolName.CompareNoCase(_T("PivotTable")) == 0) {
        return AnalysisTools::PivotTable(
            data,
            extractStr(_T("rowCol")),
            extractStr(_T("colCol")),
            extractStr(_T("valueCol")),
            extractStr(_T("aggFunc")).IsEmpty() ? _T("sum") : extractStr(_T("aggFunc"))
        );
    }
    else if (toolName.CompareNoCase(_T("OutlierDetection")) == 0) {
        return AnalysisTools::OutlierDetection(
            data,
            extractStr(_T("col")),
            extractDbl(_T("threshold"), 1.5)
        );
    }

    // 알 수 없는 도구 — BasicStats로 폴백
    return AnalysisTools::BasicStats(data);
}
