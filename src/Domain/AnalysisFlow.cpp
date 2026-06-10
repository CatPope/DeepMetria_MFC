// Domain/AnalysisFlow.cpp
#include "stdafx.h"
#include "AnalysisFlow.h"
#include "AnalysisTool.h"
#include <unordered_set>

namespace deepmetria {

AnalysisFlow::AnalysisFlow(DataSource& ds, Dashboard& dash, std::shared_ptr<LLMRouter> router)
    : m_ds(ds), m_dash(dash), m_router(std::move(router)) {}

AnalysisFlow::~AnalysisFlow()
{
    m_cancel = true;
    if (m_worker.joinable()) m_worker.join();
}

void AnalysisFlow::Start(std::wstring question, ProgressFn onProgress, DoneFn onDone)
{
    if (m_worker.joinable()) m_worker.join();
    m_cancel = false;
    m_worker = std::thread(&AnalysisFlow::Run, this,
                           std::move(question), std::move(onProgress), std::move(onDone));
}

void AnalysisFlow::Run(std::wstring question, ProgressFn onProgress, DoneFn onDone)
{
    // 어떤 실패 경로에서도 onDone 이 반드시 호출되도록 try/catch 로 감싼다.
    // 이게 없으면 LLM 호출이 throw 할 때 m_busy/[분석 실행] 버튼이 영구 잠긴다.
    m_lastError.clear();
    m_lastDescription.clear();
    try
    {
        m_cot.Clear();
        if (onProgress) onProgress(5);

        m_cot.Add(L"질문 해석 및 분석 계획 수립", L"llm.plan", question);

        LLMRouter::PlanRequest req;
        req.question = question;
        req.dataSummary = m_ds.DomainSummary();
        // 컬럼별 통계/메타 정밀 채움 — LLM이 더 정확한 도구 매핑 가능
        for (size_t ci = 0; ci < m_ds.Columns().size(); ++ci)
        {
            const auto& col = m_ds.Columns()[ci];
            LLMRouter::ColumnInfo info;
            info.name     = col.name;
            info.typeEnum = static_cast<int>(col.type);
            info.missing  = col.missing;
            info.meanVal  = col.mean;
            info.minVal   = col.minVal;
            info.maxVal   = col.maxVal;
            // 텍스트 컬럼: 고유값 수 + 첫 비공백 샘플
            if (col.type == ColumnType::Text)
            {
                std::unordered_set<std::wstring> uniq;
                for (const auto& row : m_ds.Rows())
                {
                    if (ci < row.size() && !row[ci].empty())
                    {
                        uniq.insert(row[ci]);
                        if (info.sampleValue.empty()) info.sampleValue = row[ci];
                    }
                }
                info.uniqueCount = static_cast<long long>(uniq.size());
            }
            else
            {
                for (const auto& row : m_ds.Rows())
                {
                    if (ci < row.size() && !row[ci].empty())
                    {
                        info.sampleValue = row[ci];
                        break;
                    }
                }
            }
            req.columns.push_back(std::move(info));
        }
        for (const auto& t : AnalysisTool::Catalog())
            req.tools.push_back({ t.name, t.description, t.jsonParameters });
        // 대화 이력 (외부에서 SetHistory로 주입한 값)
        req.history = m_history;

        if (onProgress) onProgress(35);

        LLMRouter::PlanResponse plan;
        if (m_router) plan = m_router->Plan(req);

        // LLM 호출 실패 — 시각화 생성 안 함, 채팅 응답도 추가 안 함.
        // 호출자(Doc) 가 LastError() 보고 에러 다이얼로그 표시.
        if (!plan.ok)
        {
            if (!plan.error.empty())
                m_lastError = plan.error;
            else
                m_lastError = L"LLM 응답을 파싱하지 못했습니다. "
                              L"모델이 JSON 형식으로 답하지 않았거나 toolName/description이 비어있습니다.";
            m_cot.Add(L"LLM 오류 — 분석 중단", L"abort", m_lastError);
            if (onProgress) onProgress(100);
            if (onDone) onDone();
            return;
        }

        m_cot.Add(L"도구 호출", plan.toolName.empty() ? L"chat-only" : plan.toolName, plan.reasoning);

        if (onProgress) onProgress(70);

        std::optional<Visualization> viz;
        bool toolMatched = true;
        // 정책 가드: 시각화 도구만 viz 생성. Chat/DataEdit 종류는 viz 생성 안 함.
        const auto toolKind = AnalysisTool::KindOf(plan.toolName);

        // LLM이 시각화 불필요로 판단 (toolName="") → 채팅 답변(description)만 누적
        if (plan.toolName.empty())
        {
            m_cot.Add(L"단순 답변 (시각화 없음)", L"chat-only",
                      plan.description.empty() ? L"(설명 없음)" : plan.description);
        }
        else if (toolKind == AnalysisTool::ToolKind::DataEdit)
        {
            // 데이터 편집 도구 — viz 생성 안 함, 향후 데이터셋 변환 처리 위치
            m_cot.Add(L"데이터 편집 도구", plan.toolName,
                      L"(향후 구현 — viz 생성 안 함)");
        }
        else if (plan.toolName == L"group_by_sum")
            viz = AnalysisTool::GroupBySum(m_ds, plan.params[L"group"], plan.params[L"value"]);
        else if (plan.toolName == L"trend_over_time")
            viz = AnalysisTool::TrendOverTime(m_ds, plan.params[L"date"], plan.params[L"value"]);
        else if (plan.toolName == L"top_n_distribution")
        {
            int n = 5;
            if (!plan.params[L"n"].empty())
                n = _wtoi(plan.params[L"n"].c_str());
            viz = AnalysisTool::TopNDistribution(m_ds, plan.params[L"column"], n);
        }
        else if (plan.toolName == L"table_sample")
        {
            int rows = 20;
            if (!plan.params[L"rows"].empty())
                rows = _wtoi(plan.params[L"rows"].c_str());
            viz = AnalysisTool::TableSample(m_ds, rows);
        }
        else if (plan.toolName == L"summary_panel")
            viz = AnalysisTool::SummaryPanel(m_ds);
        else if (plan.toolName == L"filter")
        {
            int lim = 30;
            if (!plan.params[L"rowLimit"].empty())
                lim = _wtoi(plan.params[L"rowLimit"].c_str());
            viz = AnalysisTool::Filter(m_ds,
                plan.params[L"column"], plan.params[L"op"], plan.params[L"value"], lim);
        }
        else if (plan.toolName == L"aggregate")
            viz = AnalysisTool::Aggregate(m_ds,
                plan.params[L"group"], plan.params[L"value"], plan.params[L"func"]);
        else if (plan.toolName == L"group_by_count")
            viz = AnalysisTool::GroupByCount(m_ds, plan.params[L"group"]);
        else if (plan.toolName == L"group_by_mean")
            viz = AnalysisTool::GroupByMean(m_ds,
                plan.params[L"group"], plan.params[L"value"]);
        else if (plan.toolName == L"summary_stats")
            viz = AnalysisTool::SummaryStats(m_ds, plan.params[L"value"]);
        else if (plan.toolName == L"histogram")
        {
            int bins = 10;
            if (!plan.params[L"bins"].empty())
                bins = _wtoi(plan.params[L"bins"].c_str());
            viz = AnalysisTool::Histogram(m_ds, plan.params[L"value"], bins);
        }
        else if (plan.toolName == L"scatter_plot")
            viz = AnalysisTool::ScatterPlot(m_ds,
                plan.params[L"x"], plan.params[L"y"]);
        else if (plan.toolName == L"kpi_card")
            viz = AnalysisTool::KpiCard(m_ds,
                plan.params[L"value"], plan.params[L"func"]);
        else if (plan.toolName == L"compare_columns")
            viz = AnalysisTool::CompareColumns(m_ds,
                plan.params[L"filterColumn"], plan.params[L"filterValue"],
                plan.params[L"valueColumnsCsv"]);
        else if (plan.toolName == L"compare_rows")
            viz = AnalysisTool::CompareRows(m_ds,
                plan.params[L"keyColumn"], plan.params[L"keyValuesCsv"],
                plan.params[L"valueColumn"]);
        else if (plan.toolName == L"select_rows_columns")
            viz = AnalysisTool::SelectRowsColumns(m_ds,
                plan.params[L"filterColumn"], plan.params[L"filterValuesCsv"],
                plan.params[L"valueColumnsCsv"]);
        else
        {
            // 알 수 없는 도구 — 임의 viz 생성 금지(정책: viz는 LLM 호출/사용자 드래그만).
            // 에러로 처리해서 호출자가 다이얼로그 띄움.
            toolMatched = false;
            m_lastError = L"LLM이 알 수 없는 도구를 호출했습니다: \"" + plan.toolName + L"\"";
            m_cot.Add(L"알 수 없는 도구 — 분석 중단", L"abort", m_lastError);
        }

        // 디스패치 결과가 nullopt 인 경우 (필수 파라미터 누락, 컬럼 못 찾음 등)
        if (toolMatched && !plan.toolName.empty() && !viz)
        {
            m_lastError = L"도구 \"" + plan.toolName + L"\" 실행 실패 — "
                          L"필수 파라미터가 누락되었거나 지정한 컬럼을 찾을 수 없습니다.";
            m_cot.Add(L"도구 실행 실패 — 분석 중단", L"abort", m_lastError);
        }

        if (!m_lastError.empty())
        {
            if (onProgress) onProgress(100);
            if (onDone) onDone();
            return;
        }

        if (viz)
        {
            // 정책: title/description 은 LLM 이 제공한 값만 적용. 우리가 임의 생성하지 않음.
            if (!plan.title.empty())       viz->title       = plan.title;
            if (!plan.description.empty()) viz->description = plan.description;

            // 메인 View 의 미리보기 표(y=12, h=180) 아래에 배치되도록 첫 viz 의 시작 y 를 충분히 내림.
            // 이전에는 yOff=16 으로 시작해 첫 viz 가 표 영역에 겹쳐 사용자에게 보이지 않음.
            int yOff = 200;
            for (const auto& exist : m_dash.Visualizations())
                yOff = std::max(yOff, exist.y + exist.height + 16);
            viz->y = yOff;
            m_dash.Add(*viz);
        }

        // 정책: m_lastDescription 은 LLM 의 description 만. 우리가 임의 생성하지 않음.
        // 비어있으면 채팅에 AI 응답 추가 안 함 (Doc::callback 가 가드).
        m_lastDescription = plan.description;

        m_cot.Add(L"시각화 갱신 완료", L"dashboard.add", viz ? viz->title : L"(없음)");
    }
    catch (...)
    {
        m_cot.Add(L"분석 실패", L"exception", L"내부 오류");
        if (m_lastError.empty())
            m_lastError = L"분석 중 내부 예외가 발생했습니다.";
    }

    if (onProgress) onProgress(100);
    if (onDone) onDone();
}

} // namespace deepmetria
