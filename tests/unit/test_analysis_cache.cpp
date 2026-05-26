// test_analysis_cache.cpp
// AnalysisCache 단위 테스트
// Google Test 기반, TTL/LRU/크기 관리 동작 검증

#include <windows.h>
#include <atlstr.h>

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "Infrastructure/Cache/AnalysisCache.h"

// ============================================================
// 공통 헬퍼 — CachedResult 생성
// ============================================================
static CachedResult MakeResult(const CString& key,
                                const CString& data,
                                const CString& vizType = _T("bar")) {
    CachedResult r;
    r.key     = key;
    r.data    = data;
    r.vizType = vizType;
    // createdAt은 Set() 호출 시 캐시가 갱신하므로 0으로 둠
    r.createdAt = 0;
    return r;
}

// ============================================================
// 픽스처 — 기본 TTL 60초, 최대 크기 10개 (테스트 전용 소형 캐시)
// ============================================================
class AnalysisCacheTest : public ::testing::Test {
protected:
    AnalysisCache cache{ 60, 10 };  // TTL 60초, 최대 10항목

    void SetUp() override {
        cache.Clear();
    }
};

// ============================================================
// 1. SetAndGet — 저장 후 동일 값 조회
// ============================================================
TEST_F(AnalysisCacheTest, SetAndGet_RetrievesStoredValue) {
    CachedResult in  = MakeResult(_T("key1"), _T("{\"result\":\"ok\"}"), _T("bar"));
    cache.Set(_T("key1"), in);

    CachedResult out;
    BOOL hit = cache.Get(_T("key1"), out);

    EXPECT_TRUE(hit);
    EXPECT_EQ(out.key,     CString(_T("key1")));
    EXPECT_EQ(out.data,    CString(_T("{\"result\":\"ok\"}")));
    EXPECT_EQ(out.vizType, CString(_T("bar")));
}

TEST_F(AnalysisCacheTest, SetAndGet_CreatedAtIsSetOnStore) {
    CachedResult in = MakeResult(_T("key_ts"), _T("data"));
    cache.Set(_T("key_ts"), in);

    CachedResult out;
    cache.Get(_T("key_ts"), out);

    // Set() 시각이 기록되어야 함 (0보다 커야 함)
    EXPECT_GT(out.createdAt, (time_t)0);
}

// ============================================================
// 2. GetMissing — 없는 키는 FALSE 반환
// ============================================================
TEST_F(AnalysisCacheTest, GetMissing_ReturnsFalseForNonExistentKey) {
    CachedResult out;
    BOOL hit = cache.Get(_T("존재하지않는키"), out);

    EXPECT_FALSE(hit);
}

// ============================================================
// 3. Remove — 특정 키 제거
// ============================================================
TEST_F(AnalysisCacheTest, Remove_ExistingKey_CannotBeRetrievedAfterRemoval) {
    cache.Set(_T("delme"), MakeResult(_T("delme"), _T("data")));
    EXPECT_EQ(cache.Size(), 1);

    cache.Remove(_T("delme"));

    CachedResult out;
    EXPECT_FALSE(cache.Get(_T("delme"), out));
    EXPECT_EQ(cache.Size(), 0);
}

TEST_F(AnalysisCacheTest, Remove_NonExistentKey_DoesNotThrow) {
    // 없는 키를 제거해도 예외 없이 처리되어야 함
    EXPECT_NO_THROW(cache.Remove(_T("ghost")));
    EXPECT_EQ(cache.Size(), 0);
}

// ============================================================
// 4. Clear — 전체 항목 삭제
// ============================================================
TEST_F(AnalysisCacheTest, Clear_RemovesAllEntries) {
    cache.Set(_T("a"), MakeResult(_T("a"), _T("data_a")));
    cache.Set(_T("b"), MakeResult(_T("b"), _T("data_b")));
    cache.Set(_T("c"), MakeResult(_T("c"), _T("data_c")));
    EXPECT_EQ(cache.Size(), 3);

    cache.Clear();

    EXPECT_EQ(cache.Size(), 0);
}

TEST_F(AnalysisCacheTest, Clear_EmptyCache_NoError) {
    EXPECT_NO_THROW(cache.Clear());
    EXPECT_EQ(cache.Size(), 0);
}

// ============================================================
// 5. SizeTracking — 추가/제거 후 Size() 정확성
// ============================================================
TEST_F(AnalysisCacheTest, SizeTracking_IncreasesAfterSet) {
    EXPECT_EQ(cache.Size(), 0);

    cache.Set(_T("s1"), MakeResult(_T("s1"), _T("d1")));
    EXPECT_EQ(cache.Size(), 1);

    cache.Set(_T("s2"), MakeResult(_T("s2"), _T("d2")));
    EXPECT_EQ(cache.Size(), 2);
}

TEST_F(AnalysisCacheTest, SizeTracking_DecreasesAfterRemove) {
    cache.Set(_T("s1"), MakeResult(_T("s1"), _T("d1")));
    cache.Set(_T("s2"), MakeResult(_T("s2"), _T("d2")));

    cache.Remove(_T("s1"));
    EXPECT_EQ(cache.Size(), 1);
}

TEST_F(AnalysisCacheTest, SizeTracking_ZeroAfterClear) {
    cache.Set(_T("s1"), MakeResult(_T("s1"), _T("d1")));
    cache.Set(_T("s2"), MakeResult(_T("s2"), _T("d2")));

    cache.Clear();
    EXPECT_EQ(cache.Size(), 0);
}

// ============================================================
// 6. MaxSizeEviction — 최대 크기 초과 시 LRU 항목 제거
// ============================================================
TEST_F(AnalysisCacheTest, MaxSizeEviction_OldestEntryEvictedWhenFull) {
    // 최대 크기 3인 캐시로 재설정
    AnalysisCache smallCache(60, 3);

    smallCache.Set(_T("oldest"), MakeResult(_T("oldest"), _T("d0")));
    smallCache.Set(_T("middle"), MakeResult(_T("middle"), _T("d1")));
    smallCache.Set(_T("newest"), MakeResult(_T("newest"), _T("d2")));
    EXPECT_EQ(smallCache.Size(), 3);

    // 4번째 항목 추가 → oldest 제거
    smallCache.Set(_T("extra"), MakeResult(_T("extra"), _T("d3")));

    EXPECT_EQ(smallCache.Size(), 3);

    CachedResult out;
    // "oldest"가 제거되었어야 함
    EXPECT_FALSE(smallCache.Get(_T("oldest"), out));
    // 나머지는 존재해야 함
    EXPECT_TRUE(smallCache.Get(_T("middle"), out));
    EXPECT_TRUE(smallCache.Get(_T("newest"), out));
    EXPECT_TRUE(smallCache.Get(_T("extra"),  out));
}

TEST_F(AnalysisCacheTest, MaxSizeEviction_SizeNeverExceedsMaxSize) {
    AnalysisCache boundedCache(60, 5);

    for (int i = 0; i < 10; ++i) {
        CString key;
        key.Format(_T("key%d"), i);
        boundedCache.Set(key, MakeResult(key, _T("data")));
    }

    // 최대 크기 5를 넘지 않아야 함
    EXPECT_LE(boundedCache.Size(), 5);
}

// ============================================================
// 7. TTLExpiry — TTL 만료 후 캐시 미스
// ============================================================
TEST_F(AnalysisCacheTest, TTLExpiry_ItemExpiredAfterTTL) {
    // TTL 1초 캐시
    AnalysisCache shortTtlCache(1, 10);
    shortTtlCache.Set(_T("expire_me"),
                      MakeResult(_T("expire_me"), _T("will_expire")));

    // 즉시 조회 → 히트
    CachedResult out;
    EXPECT_TRUE(shortTtlCache.Get(_T("expire_me"), out));

    // 2초 대기 → TTL 초과
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 만료 후 조회 → 미스
    EXPECT_FALSE(shortTtlCache.Get(_T("expire_me"), out));
}

TEST_F(AnalysisCacheTest, TTLExpiry_NonExpiredItemStillAccessible) {
    // TTL 60초 캐시에서 즉시 조회 → 만료되지 않아야 함
    cache.Set(_T("fresh"), MakeResult(_T("fresh"), _T("valid_data")));

    CachedResult out;
    EXPECT_TRUE(cache.Get(_T("fresh"), out));
    EXPECT_EQ(out.data, CString(_T("valid_data")));
}

// ============================================================
// 8. UpdateExisting — 동일 키 재저장 시 값 갱신
// ============================================================
TEST_F(AnalysisCacheTest, UpdateExisting_OverwritesOldValue) {
    cache.Set(_T("upd"), MakeResult(_T("upd"), _T("old_data")));
    cache.Set(_T("upd"), MakeResult(_T("upd"), _T("new_data")));

    CachedResult out;
    BOOL hit = cache.Get(_T("upd"), out);

    EXPECT_TRUE(hit);
    EXPECT_EQ(out.data, CString(_T("new_data")));
}

TEST_F(AnalysisCacheTest, UpdateExisting_SizeRemainsOne) {
    cache.Set(_T("dup"), MakeResult(_T("dup"), _T("v1")));
    cache.Set(_T("dup"), MakeResult(_T("dup"), _T("v2")));

    // 동일 키 재저장이므로 크기는 1
    EXPECT_EQ(cache.Size(), 1);
}

// ============================================================
// 9. EvictManual — Evict() 호출 시 만료 항목만 제거
// ============================================================
TEST_F(AnalysisCacheTest, EvictManual_RemovesExpiredKeepsValid) {
    // TTL 1초 캐시에 두 항목 저장
    AnalysisCache mixedCache(1, 10);
    mixedCache.Set(_T("expire1"), MakeResult(_T("expire1"), _T("d1")));
    mixedCache.Set(_T("expire2"), MakeResult(_T("expire2"), _T("d2")));

    // 2초 대기 → 두 항목 만료
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // TTL이 긴 항목 추가 (Evict 후에도 남아야 함)
    AnalysisCache longCache(3600, 10);
    longCache.Set(_T("keep1"), MakeResult(_T("keep1"), _T("d3")));
    longCache.Set(_T("expired1"), MakeResult(_T("expired1"), _T("d4")));

    // expired1의 createdAt을 과거로 강제 설정하는 것은 불가하므로
    // 만료된 캐시 객체에서 Evict 동작 검증
    mixedCache.Evict();

    // 만료 항목은 Evict 후 Size가 0이어야 함
    EXPECT_EQ(mixedCache.Size(), 0);
}

TEST_F(AnalysisCacheTest, EvictManual_ValidItemsNotRemoved) {
    // TTL 60초 캐시 — 즉시 Evict해도 항목 유지
    cache.Set(_T("v1"), MakeResult(_T("v1"), _T("d1")));
    cache.Set(_T("v2"), MakeResult(_T("v2"), _T("d2")));

    cache.Evict();

    // 만료되지 않은 항목은 그대로 유지
    EXPECT_EQ(cache.Size(), 2);
}

// ============================================================
// 10. SetTTLAndMaxSize — 설정 변경이 이후 동작에 반영됨
// ============================================================
TEST_F(AnalysisCacheTest, SetTTL_ShorterTTLExpiresItemsFaster) {
    cache.Set(_T("cfg_test"), MakeResult(_T("cfg_test"), _T("data")));

    // TTL을 1초로 변경
    cache.SetTTL(1);

    // 즉시 조회 → 기존 항목은 createdAt 기준이므로 아직 유효
    // (Set 직후라면 만료 안 됨)
    CachedResult out;
    BOOL hitBefore = cache.Get(_T("cfg_test"), out);

    // 2초 후 → TTL 1초 적용으로 만료
    std::this_thread::sleep_for(std::chrono::seconds(2));
    BOOL hitAfter = cache.Get(_T("cfg_test"), out);

    // 처음엔 히트, 나중엔 미스
    EXPECT_TRUE(hitBefore);
    EXPECT_FALSE(hitAfter);
}

TEST_F(AnalysisCacheTest, SetMaxSize_NewLimitEnforcedOnNextSet) {
    // 먼저 3개 저장
    cache.Set(_T("m1"), MakeResult(_T("m1"), _T("d1")));
    cache.Set(_T("m2"), MakeResult(_T("m2"), _T("d2")));
    cache.Set(_T("m3"), MakeResult(_T("m3"), _T("d3")));
    EXPECT_EQ(cache.Size(), 3);

    // 최대 크기를 2로 줄임
    cache.SetMaxSize(2);

    // 새 항목 추가 → 최대 크기 초과로 LRU 제거
    cache.Set(_T("m4"), MakeResult(_T("m4"), _T("d4")));

    EXPECT_LE(cache.Size(), 2);
}

// ============================================================
// 11. LRU 순서 검증 — 가장 최근 접근 항목이 제거되지 않음
// ============================================================
TEST_F(AnalysisCacheTest, LRU_MostRecentlyAccessedNotEvicted) {
    AnalysisCache lruCache(60, 3);

    lruCache.Set(_T("a"), MakeResult(_T("a"), _T("da")));
    lruCache.Set(_T("b"), MakeResult(_T("b"), _T("db")));
    lruCache.Set(_T("c"), MakeResult(_T("c"), _T("dc")));

    // "a"에 접근하여 최근 접근 갱신
    CachedResult out;
    lruCache.Get(_T("a"), out);

    // "d" 추가 → LRU인 "b"가 제거되어야 함
    lruCache.Set(_T("d"), MakeResult(_T("d"), _T("dd")));

    // "a"는 최근 접근했으므로 남아있어야 함
    EXPECT_TRUE(lruCache.Get(_T("a"), out));
    // "c", "d"도 남아야 함
    EXPECT_TRUE(lruCache.Get(_T("c"), out));
    EXPECT_TRUE(lruCache.Get(_T("d"), out));
    // "b"는 가장 오래된 항목이므로 제거됨
    EXPECT_FALSE(lruCache.Get(_T("b"), out));
}

// ============================================================
// 12. 멀티키 독립성 — 서로 다른 키가 독립적으로 저장/조회
// ============================================================
TEST_F(AnalysisCacheTest, MultipleKeys_EachRetrievesOwnValue) {
    cache.Set(_T("group_by"),   MakeResult(_T("group_by"),   _T("{\"group\":true}"),  _T("bar")));
    cache.Set(_T("time_series"),MakeResult(_T("time_series"),_T("{\"time\":true}"),   _T("line")));
    cache.Set(_T("correlation"),MakeResult(_T("correlation"),_T("{\"corr\":true}"),   _T("scatter")));

    CachedResult out;

    cache.Get(_T("group_by"), out);
    EXPECT_EQ(out.vizType, CString(_T("bar")));

    cache.Get(_T("time_series"), out);
    EXPECT_EQ(out.vizType, CString(_T("line")));

    cache.Get(_T("correlation"), out);
    EXPECT_EQ(out.vizType, CString(_T("scatter")));
}
