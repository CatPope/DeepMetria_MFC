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

    struct ColumnInfo
    {
        std::wstring name;
        int          typeEnum   = 0;
        long long    missing    = 0;
        // 수치/날짜 통계 (typeEnum == 1 or 3)
        double       meanVal    = 0.0;
        double       minVal     = 0.0;
        double       maxVal     = 0.0;
        // 텍스트 메타 (typeEnum == 2)
        long long    uniqueCount = 0;
        std::wstring sampleValue;   // 첫 비공백 값 예시
    };
    struct ToolInfo    { std::wstring name; std::wstring description; std::wstring jsonParameters; };
    struct ChatTurn    { bool isUser; std::wstring text; };  // 대화 이력 단위

    struct PlanRequest
    {
        std::wstring                question;
        std::wstring                dataSummary;     // 시스템 프롬프트용 데이터 개요
        std::vector<ColumnInfo>     columns;          // 시스템 프롬프트용 컬럼 메타
        std::vector<ToolInfo>       tools;
        std::vector<ChatTurn>       history;          // 이전 대화 (시간순, 마지막이 가장 최근)
    };

    struct PlanResponse
    {
        std::wstring                                            toolName;
        std::unordered_map<std::wstring, std::wstring>          params;
        std::wstring                                            reasoning;
        std::wstring                                            title;        // LLM이 만든 시각화 제목 (옵션)
        std::wstring                                            description;  // 시각화에 대한 자연어 설명 (hover/채팅용)
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
