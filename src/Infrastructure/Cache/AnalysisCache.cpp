#include "stdafx.h"
#include "AnalysisCache.h"
#include <ctime>

// ============================================================
// 생성자 / 소멸자
// ============================================================

AnalysisCache::AnalysisCache(int ttlSeconds, int maxSize)
    : m_ttlSeconds(ttlSeconds)
    , m_maxSize(maxSize)
{
}

AnalysisCache::~AnalysisCache() {
    Clear();
}

// ============================================================
// Get — 캐시 조회
// ============================================================

BOOL AnalysisCache::Get(const CString& key, CachedResult& outResult) {
    auto it = m_map.find(key);
    if (it == m_map.end()) return FALSE;   // 미스

    // 만료 확인
    if (IsExpired(it->second->result)) {
        // 만료된 항목 제거
        m_lruList.erase(it->second);
        m_map.erase(it);
        return FALSE;
    }

    // LRU 갱신: 찾은 노드를 리스트 앞으로 이동
    m_lruList.splice(m_lruList.begin(), m_lruList, it->second);
    outResult = it->second->result;
    return TRUE;
}

// ============================================================
// Set — 캐시 저장
// ============================================================

void AnalysisCache::Set(const CString& key, const CachedResult& result) {
    // 이미 존재하는 키면 제거 후 재삽입
    auto it = m_map.find(key);
    if (it != m_map.end()) {
        m_lruList.erase(it->second);
        m_map.erase(it);
    }

    // 최대 크기 초과 시 LRU 제거
    if ((int)m_lruList.size() >= m_maxSize) {
        EvictLRU();
    }

    // 리스트 앞에 삽입 (가장 최근)
    LRUNode node;
    node.key    = key;
    node.result = result;
    node.result.createdAt = time(nullptr); // 저장 시각 갱신
    m_lruList.push_front(node);
    m_map[key] = m_lruList.begin();
}

// ============================================================
// Remove — 특정 키 제거
// ============================================================

void AnalysisCache::Remove(const CString& key) {
    auto it = m_map.find(key);
    if (it == m_map.end()) return;
    m_lruList.erase(it->second);
    m_map.erase(it);
}

// ============================================================
// Clear — 전체 비움
// ============================================================

void AnalysisCache::Clear() {
    m_lruList.clear();
    m_map.clear();
}

// ============================================================
// Evict — 만료 항목 일괄 제거
// ============================================================

void AnalysisCache::Evict() {
    auto it = m_lruList.begin();
    while (it != m_lruList.end()) {
        if (IsExpired(it->result)) {
            m_map.erase(it->key);
            it = m_lruList.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================
// Size
// ============================================================

int AnalysisCache::Size() const {
    return (int)m_lruList.size();
}

// ============================================================
// 내부 헬퍼
// ============================================================

BOOL AnalysisCache::IsExpired(const CachedResult& result) const {
    if (result.createdAt == 0) return FALSE; // 시각 미설정이면 만료 아님
    time_t now = time(nullptr);
    return (now - result.createdAt) > m_ttlSeconds;
}

void AnalysisCache::EvictLRU() {
    if (m_lruList.empty()) return;
    // 리스트 뒤 = 가장 오래된 항목
    auto last = m_lruList.end();
    --last;
    m_map.erase(last->key);
    m_lruList.erase(last);
}
