// Infra/ILLMProvider.h - LLM 프로바이더 어댑터 인터페이스
#pragma once
#include "LLMRouter.h"   // PlanRequest/PlanResponse 정의

namespace deepmetria {

class ILLMProvider
{
public:
    virtual ~ILLMProvider() = default;
    virtual LLMRouter::PlanResponse Plan(const LLMRouter::PlanRequest& req,
                                         const std::wstring& apiKey,
                                         const std::wstring& model) const = 0;
};

} // namespace deepmetria
