// Domain/MemoryCache.h - 단순 TTL 인메모리 캐시 (std::unordered_map)
#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <optional>

namespace deepmetria {

template <typename V>
class MemoryCache
{
public:
    void Put(const std::wstring& key, V value, int ttlSeconds = 600)
    {
        std::lock_guard<std::mutex> lk(m_mu);
        m_store[key] = { std::move(value),
                         std::chrono::steady_clock::now() + std::chrono::seconds(ttlSeconds) };
    }

    std::optional<V> Get(const std::wstring& key)
    {
        std::lock_guard<std::mutex> lk(m_mu);
        auto it = m_store.find(key);
        if (it == m_store.end()) return std::nullopt;
        if (std::chrono::steady_clock::now() > it->second.expires)
        {
            m_store.erase(it);
            return std::nullopt;
        }
        return it->second.value;
    }

private:
    struct Entry
    {
        V                                       value;
        std::chrono::steady_clock::time_point   expires;
    };
    std::unordered_map<std::wstring, Entry> m_store;
    std::mutex                              m_mu;
};

} // namespace deepmetria
