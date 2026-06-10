// Infra/LLMRouter.h - 멀티 LLM 추상화 (Claude / OpenAI / Gemini)
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace deepmetria {

class SecretsStore;
class ILLMProvider;

class LLMRouter
{
public:
    explicit LLMRouter(std::shared_ptr<SecretsStore> secrets);
    ~LLMRouter();

    enum class Provider { Unknown, Claude, OpenAI, Gemini };

    // 레지스트리에서 provider/model 로드
    void LoadConfig();

    Provider           CurrentProvider() const { return m_provider; }
    const std::wstring& Model() const          { return m_model; }

    // 분석 실행 가능 여부 — provider 식별 + API 키 모두 있어야 함
    bool IsConfigured() const
    {
        return m_provider != Provider::Unknown && !m_apiKey.empty();
    }

    struct ColumnInfo  { std::wstring name; int typeEnum; };
    struct ToolInfo    { std::wstring name; std::wstring description; std::wstring jsonParameters; };

    struct PlanRequest
    {
        std::wstring                question;
        std::wstring                dataSummary;
        std::vector<ColumnInfo>     columns;
        std::vector<ToolInfo>       tools;
    };

    struct PlanResponse
    {
        std::wstring                                            toolName;
        std::unordered_map<std::wstring, std::wstring>          params;
        std::wstring                                            reasoning;
        std::wstring                                            title;     // LLM이 만든 시각화 제목 (옵션)
        std::wstring                                            error;
        bool                                                    ok = false;
    };

    // 자연어 → 도구 호출 계획. 오프라인이면 휴리스틱으로 폴백.
    PlanResponse Plan(const PlanRequest& req);

private:
    PlanResponse PlanLocalHeuristic(const PlanRequest& req) const;

    std::shared_ptr<SecretsStore>                   m_secrets;
    Provider                                        m_provider = Provider::Unknown;
    std::wstring                                    m_model;
    std::wstring                                    m_apiKey;
    std::unordered_map<int, std::unique_ptr<ILLMProvider>> m_providers;
};

} // namespace deepmetria
