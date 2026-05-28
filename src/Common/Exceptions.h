#pragma once
// Exceptions.h — DeepMetria 커스텀 예외 타입

#include "Types.h"
#include <stdexcept>

// ============================================================
// CToolNotFoundException — 도구를 찾을 수 없을 때 발생하는 예외 (TC-04-07)
// ============================================================
class CToolNotFoundException : public std::runtime_error {
public:
    CToolNotFoundException(const std::string& toolName)
        : std::runtime_error("Tool not found: " + toolName), m_toolName(toolName) {}
    const std::string& GetToolName() const { return m_toolName; }
private:
    std::string m_toolName;
};

// ============================================================
// CUnsupportedProviderException — 지원하지 않는 프로바이더일 때 발생하는 예외 (TC-08-04)
// ============================================================
class CUnsupportedProviderException : public std::runtime_error {
public:
    CUnsupportedProviderException(const std::string& providerName)
        : std::runtime_error("Unsupported provider: " + providerName), m_providerName(providerName) {}
    const std::string& GetProviderName() const { return m_providerName; }
private:
    std::string m_providerName;
};
