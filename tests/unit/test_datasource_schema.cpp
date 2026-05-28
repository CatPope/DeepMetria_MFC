// test_datasource_schema.cpp
// DataSourceService::GetSchema 단위 테스트
// TC-02-03: 스키마 추출 검증

// MFC 헤더를 Google Test보다 먼저 포함 (PCH 순서 보장)
#include <windows.h>
#include <atlstr.h>

#include <gtest/gtest.h>

#include "Common/Types.h"
#include "Domain/DataSource/DataSourceService.h"
#include "fixtures/TestDataFixture.h"

// ============================================================
// DataSourceSchemaTest 픽스처
// TestDataFixture에서 5컬럼 10행의 한국어 샘플 테이블을 제공받는다.
// ============================================================
class DataSourceSchemaTest : public TestDataFixture {};

// ============================================================
// TC-02-03: GetSchema — 컬럼 수 검증
// ============================================================

// 반환된 스키마의 컬럼 수가 DataTable.columns 수와 일치해야 함
TEST_F(DataSourceSchemaTest, GetSchema_ReturnsCorrectColumnCount) {
    auto schema = DataSourceService::GetSchema(m_sampleTable);
    EXPECT_EQ(schema.size(), 5u)
        << "샘플 테이블의 스키마는 5개 컬럼을 포함해야 한다";
}

// ============================================================
// 컬럼 이름 보존 검증
// ============================================================

// 첫 번째 컬럼명이 "날짜"이어야 함
TEST_F(DataSourceSchemaTest, GetSchema_PreservesFirstColumnName) {
    auto schema = DataSourceService::GetSchema(m_sampleTable);
    ASSERT_GE(schema.size(), 1u);
    EXPECT_EQ(schema[0].name, CString(_T("날짜")))
        << "첫 번째 컬럼명은 '날짜'이어야 한다";
}

// 모든 컬럼명이 DataTable의 원본 컬럼명과 일치해야 함
TEST_F(DataSourceSchemaTest, GetSchema_PreservesAllColumnNames) {
    auto schema = DataSourceService::GetSchema(m_sampleTable);
    ASSERT_EQ(schema.size(), 5u);

    EXPECT_EQ(schema[0].name, CString(_T("날짜")));
    EXPECT_EQ(schema[1].name, CString(_T("카테고리")));
    EXPECT_EQ(schema[2].name, CString(_T("매출")));
    EXPECT_EQ(schema[3].name, CString(_T("수량")));
    EXPECT_EQ(schema[4].name, CString(_T("지역")));
}

// ============================================================
// 컬럼 타입 검출 검증
// ============================================================

// numeric 타입 컬럼이 올바르게 감지되어야 함
TEST_F(DataSourceSchemaTest, GetSchema_DetectsNumericColumnType) {
    auto schema = DataSourceService::GetSchema(m_sampleTable);
    ASSERT_GE(schema.size(), 3u);
    // 매출(index 2)은 numeric
    EXPECT_EQ(schema[2].type, CString(_T("numeric")))
        << "매출 컬럼의 타입은 'numeric'이어야 한다";
}

// text 타입 컬럼이 올바르게 감지되어야 함
TEST_F(DataSourceSchemaTest, GetSchema_DetectsTextColumnType) {
    auto schema = DataSourceService::GetSchema(m_sampleTable);
    ASSERT_GE(schema.size(), 2u);
    // 카테고리(index 1)는 text
    EXPECT_EQ(schema[1].type, CString(_T("text")))
        << "카테고리 컬럼의 타입은 'text'이어야 한다";
}

// date 타입 컬럼이 올바르게 감지되어야 함
TEST_F(DataSourceSchemaTest, GetSchema_DetectsDateColumnType) {
    auto schema = DataSourceService::GetSchema(m_sampleTable);
    ASSERT_GE(schema.size(), 1u);
    // 날짜(index 0)는 date
    EXPECT_EQ(schema[0].type, CString(_T("date")))
        << "날짜 컬럼의 타입은 'date'이어야 한다";
}

// ============================================================
// 컬럼 인덱스 검증
// ============================================================

// 각 컬럼의 index 값이 순서와 일치해야 함
TEST_F(DataSourceSchemaTest, GetSchema_SetsCorrectColumnIndices) {
    auto schema = DataSourceService::GetSchema(m_sampleTable);
    ASSERT_EQ(schema.size(), 5u);
    for (size_t i = 0; i < schema.size(); ++i) {
        EXPECT_EQ(schema[i].index, static_cast<int>(i))
            << "컬럼 " << i << "의 index가 올바르지 않다";
    }
}

// ============================================================
// 빈 테이블 처리
// ============================================================

// 컬럼이 없는 빈 테이블은 빈 스키마를 반환해야 함
TEST_F(DataSourceSchemaTest, GetSchema_EmptyTable_ReturnsEmptySchema) {
    DataTable empty;
    empty.rowCount = 0;
    empty.colCount = 0;
    // columns 없음

    auto schema = DataSourceService::GetSchema(empty);
    EXPECT_TRUE(schema.empty())
        << "빈 테이블의 스키마는 비어 있어야 한다";
}

// ============================================================
// 단일 컬럼 테이블
// ============================================================

// 컬럼이 하나뿐인 테이블도 올바르게 처리되어야 함
TEST_F(DataSourceSchemaTest, GetSchema_SingleColumnTable_ReturnsOneSchema) {
    DataTable single;
    single.rowCount = 3;
    single.colCount = 1;

    DataColumn col;
    col.name   = _T("점수");
    col.type   = _T("numeric");
    col.values = { _T("90"), _T("85"), _T("92") };
    single.columns = { col };

    auto schema = DataSourceService::GetSchema(single);
    ASSERT_EQ(schema.size(), 1u);
    EXPECT_EQ(schema[0].name, CString(_T("점수")));
    EXPECT_EQ(schema[0].type, CString(_T("numeric")));
    EXPECT_EQ(schema[0].index, 0);
}
