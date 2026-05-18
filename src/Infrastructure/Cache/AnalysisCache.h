#pragma once
// AnalysisCache.h — 분석 결과 인메모리 캐시
// std::unordered_map 기반, TTL 30분, LRU 제거 (최대 100항목)

#include "../../Common/Types.h"
#include <unordered_map>
#include <list>
#include <ctime>

// CString 해시 펑터 — namespace std 특수화 대신 커스텀 펑터 사용
// (MSVC PCH 환경에서 namespace std 재열기가 금지되므로)
struct CStringHasher {
    size_t operator()(const CString& str) const noexcept {
        size_t h = 0;
        for (int i = 0; i < str.GetLength(); i++) {
            h = h * 31 + static_cast<size_t>(str[i]);
        }
        return h;
    }
};
struct CStringEqual {
    bool operator()(const CString& a, const CString& b) const noexcept {
        return a == b;
    }
};

// ============================================================
// AnalysisCache — TTL + LRU 인메모리 캐시
// ============================================================
class AnalysisCache {
public:
    // 기본값: TTL 30분, 최대 100항목
    explicit AnalysisCache(int ttlSeconds = 1800, int maxSize = 100);
    ~AnalysisCache();

    // 캐시에서 항목 조회
    // 반환: TRUE=캐시 히트, FALSE=미스 또는 만료
    BOOL Get(const CString& key, CachedResult& outResult);

    // 캐시에 항목 저장 (최대 크기 초과 시 LRU 제거)
    void Set(const CString& key, const CachedResult& result);

    // 특정 키 제거
    void Remove(const CString& key);

    // 전체 캐시 비움
    void Clear();

    // 만료된 항목 일괄 제거
    void Evict();

    // 현재 캐시 항목 수
    int Size() const;

    // TTL/최대크기 설정 변경
    void SetTTL(int ttlSeconds)  { m_ttlSeconds = ttlSeconds; }
    void SetMaxSize(int maxSize) { m_maxSize    = maxSize;    }

private:
    // LRU 노드: 접근 순서 관리용 리스트 항목
    struct LRUNode {
        CString      key;
        CachedResult result;
    };

    // 키가 만료됐는지 확인
    BOOL IsExpired(const CachedResult& result) const;

    // 최대 크기 초과 시 가장 오래된 항목 제거
    void EvictLRU();

    int m_ttlSeconds;   // TTL (초)
    int m_maxSize;      // 최대 항목 수

    // LRU 접근 순서 리스트 (앞=가장 최근, 뒤=가장 오래됨)
    std::list<LRUNode> m_lruList;

    // 빠른 조회용 맵: key → 리스트 이터레이터 (CStringHasher 사용)
    std::unordered_map<CString, std::list<LRUNode>::iterator, CStringHasher, CStringEqual> m_map;
};
