// Domain/AnalysisFlow.cpp
#include "stdafx.h"
#include "AnalysisFlow.h"
#include "AnalysisTool.h"

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
    try
    {
        m_cot.Clear();
        if (onProgress) onProgress(5);

        m_cot.Add(L"질문 해석 및 분석 계획 수립", L"llm.plan", question);

        LLMRouter::PlanRequest req;
        req.question = question;
        req.dataSummary = m_ds.DomainSummary();
        for (const auto& c : m_ds.Columns())
            req.columns.push_back({ c.name, static_cast<int>(c.type) });
        for (const auto& t : AnalysisTool::Catalog())
            req.tools.push_back({ t.name, t.description, t.jsonParameters });

        if (onProgress) onProgress(35);

        LLMRouter::PlanResponse plan;
        if (m_router) plan = m_router->Plan(req);

        m_cot.Add(L"도구 호출", plan.toolName.empty() ? L"local.fallback" : plan.toolName, plan.reasoning);

        if (onProgress) onProgress(70);

        std::optional<Visualization> viz;
        if (plan.toolName == L"group_by_sum")
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
        else
        {
            std::wstring g, v;
            for (const auto& c : m_ds.Columns())
            {
                if (g.empty() && c.type == ColumnType::Text)   g = c.name;
                if (v.empty() && c.type == ColumnType::Number) v = c.name;
            }
            if (!g.empty() && !v.empty())
                viz = AnalysisTool::GroupBySum(m_ds, g, v);
            else
                viz = AnalysisTool::TableSample(m_ds, 20);
        }

        if (viz)
        {
            // LLM이 만든 제목이 있으면 우선 적용 (없으면 AnalysisTool의 기본 제목 유지)
            if (!plan.title.empty())
                viz->title = plan.title;

            int yOff = 16;
            for (const auto& exist : m_dash.Visualizations())
                yOff = std::max(yOff, exist.y + exist.height + 16);
            viz->y = yOff;
            m_dash.Add(*viz);
        }

        m_cot.Add(L"시각화 갱신 완료", L"dashboard.add", viz ? viz->title : L"(없음)");
    }
    catch (...)
    {
        m_cot.Add(L"분석 실패", L"exception", L"내부 오류");
    }

    if (onProgress) onProgress(100);
    if (onDone) onDone();
}

} // namespace deepmetria
