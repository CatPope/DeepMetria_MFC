// Infra/GeminiProvider.h - Gemini API 어댑터
#pragma once
#include "ILLMProvider.h"

namespace deepmetria {

class GeminiProvider : public ILLMProvider
{
public:
    LLMRouter::PlanResponse Plan(const LLMRouter::PlanRequest& req,
                                 const std::wstring& apiKey,
                                 const std::wstring& model) const override;
};

} // namespace deepmetria
