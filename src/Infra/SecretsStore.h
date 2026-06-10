// Infra/SecretsStore.h - DPAPI 암호화 키 레지스트리 저장/조회
#pragma once
#include <string>
#include <optional>

namespace deepmetria {

class SecretsStore
{
public:
    explicit SecretsStore(std::wstring registrySubPath);

    // HKCU\<registrySubPath>\<valueName> 의 DPAPI 암호 바이너리를 평문 문자열로 복호화
    std::optional<std::wstring> GetSecret(const std::wstring& valueName) const;

    // 일반 문자열 값 (예: provider, model)
    std::optional<std::wstring> GetValue(const std::wstring& valueName) const;

    // 일반 문자열 저장 (평문) — provider/model 등
    bool SetValue(const std::wstring& valueName, const std::wstring& value) const;

    // 키를 DPAPI 암호화하여 저장
    bool SetSecret(const std::wstring& valueName, const std::wstring& plain) const;

private:
    std::wstring m_subPath;   // 예: "Software\\DeepMetria\\Settings"
};

} // namespace deepmetria
