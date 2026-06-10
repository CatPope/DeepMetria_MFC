// Infra/SecretsStore.cpp - HKCU + DPAPI
#include "stdafx.h"
#include "SecretsStore.h"
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")
#include <vector>

namespace deepmetria {

SecretsStore::SecretsStore(std::wstring sub) : m_subPath(std::move(sub)) {}

namespace {

bool ReadBinary(const std::wstring& sub, const std::wstring& name, std::vector<BYTE>& out)
{
    HKEY hk = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, sub.c_str(), 0, KEY_READ, &hk) != ERROR_SUCCESS)
        return false;
    DWORD type = 0, sz = 0;
    LONG rc = RegQueryValueExW(hk, name.c_str(), nullptr, &type, nullptr, &sz);
    if (rc != ERROR_SUCCESS || sz == 0)
    {
        RegCloseKey(hk);
        return false;
    }
    out.assign(sz, 0);
    rc = RegQueryValueExW(hk, name.c_str(), nullptr, &type, out.data(), &sz);
    RegCloseKey(hk);
    return rc == ERROR_SUCCESS;
}

bool WriteBinary(const std::wstring& sub, const std::wstring& name,
                 const BYTE* data, DWORD sz, DWORD type)
{
    HKEY hk = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, sub.c_str(), 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hk, nullptr) != ERROR_SUCCESS)
        return false;
    LONG rc = RegSetValueExW(hk, name.c_str(), 0, type, data, sz);
    RegCloseKey(hk);
    return rc == ERROR_SUCCESS;
}

std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

} // namespace

std::optional<std::wstring> SecretsStore::GetSecret(const std::wstring& name) const
{
    std::vector<BYTE> cipher;
    if (!ReadBinary(m_subPath, name, cipher)) return std::nullopt;

    DATA_BLOB in{ static_cast<DWORD>(cipher.size()), cipher.data() };
    DATA_BLOB out{};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out))
        return std::nullopt;

    std::string utf8(reinterpret_cast<char*>(out.pbData), out.cbData);
    LocalFree(out.pbData);
    return Utf8ToWide(utf8);
}

bool SecretsStore::SetSecret(const std::wstring& name, const std::wstring& plain) const
{
    std::string utf8 = WideToUtf8(plain);
    DATA_BLOB in{ static_cast<DWORD>(utf8.size()),
                  reinterpret_cast<BYTE*>(utf8.data()) };
    DATA_BLOB out{};
    if (!CryptProtectData(&in, L"DeepMetria.key", nullptr, nullptr, nullptr, 0, &out))
        return false;

    bool ok = WriteBinary(m_subPath, name, out.pbData, out.cbData, REG_BINARY);
    LocalFree(out.pbData);
    return ok;
}

std::optional<std::wstring> SecretsStore::GetValue(const std::wstring& name) const
{
    HKEY hk = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, m_subPath.c_str(), 0, KEY_READ, &hk) != ERROR_SUCCESS)
        return std::nullopt;
    DWORD type = 0, sz = 0;
    if (RegQueryValueExW(hk, name.c_str(), nullptr, &type, nullptr, &sz) != ERROR_SUCCESS || sz == 0)
    {
        RegCloseKey(hk);
        return std::nullopt;
    }
    std::wstring buf(sz / sizeof(wchar_t), L'\0');
    LONG rc = RegQueryValueExW(hk, name.c_str(), nullptr, &type,
                               reinterpret_cast<BYTE*>(buf.data()), &sz);
    RegCloseKey(hk);
    if (rc != ERROR_SUCCESS) return std::nullopt;
    // 후행 NUL 제거
    while (!buf.empty() && buf.back() == L'\0') buf.pop_back();
    return buf;
}

bool SecretsStore::SetValue(const std::wstring& name, const std::wstring& value) const
{
    DWORD sz = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    return WriteBinary(m_subPath, name,
        reinterpret_cast<const BYTE*>(value.c_str()), sz, REG_SZ);
}

} // namespace deepmetria
