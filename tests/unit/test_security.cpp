// test_security.cpp
// NFR 보안 요건 단위 테스트
// TC-S-02: SQL Injection 방어
// TC-S-03: XSS 방어
// TC-S-04: Path Traversal 방어
// TC-S-05: API Key 마스킹
// TC-S-06: 대용량 입력 처리 (버퍼 오버플로 방지)
// TC-S-07: NULL / 특수문자 입력 처리

// MFC 헤더를 Google Test보다 먼저 포함 (PCH 순서 보장)
#include <windows.h>
#include <atlstr.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Common/Types.h"
#include "Domain/Analysis/AnalysisTools.h"
#include "fixtures/TestDataFixture.h"

// ============================================================
// SecurityTest 픽스처
// ============================================================
class SecurityTest : public TestDataFixture {};

// ============================================================
// TC-S-02: SQL Injection 방어
// ============================================================

TEST_F(SecurityTest, AnalysisTools_SqlInjectionInColumnName_NoExecution) {
    DataTable malicious = m_sampleTable;
    malicious.columns[0].name = _T("'; DROP TABLE users; --");

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::BasicStats(malicious);
    }) << "SQL 주입 컬럼명으로 크래시가 발생했다";

    EXPECT_FALSE(result.IsEmpty()) << "SQL 주입 시도 후에도 결과를 반환해야 한다";
}

TEST_F(SecurityTest, AnalysisTools_SqlInjectionInCellValue_NoExecution) {
    DataTable malicious = m_sampleTable;
    malicious.columns[1].values[0] = _T("전자'; DELETE FROM data; --");

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::GroupByAggregate(
            malicious, _T("카테고리"), _T("매출"), _T("sum"));
    }) << "셀 값 SQL 주입으로 크래시가 발생했다";
}

TEST_F(SecurityTest, AnalysisTools_UnionSqlInjection_HandledSafely) {
    DataTable malicious = m_sampleTable;
    malicious.columns[1].values[0] = _T("A' UNION SELECT * FROM passwords--");

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::FrequencyDistribution(malicious, _T("카테고리"));
    });
}

// ============================================================
// TC-S-03: XSS 방어
// ============================================================

TEST_F(SecurityTest, AnalysisTools_XssInColumnName_NotExecuted) {
    DataTable xss = m_sampleTable;
    xss.columns[1].name = _T("<script>alert('xss')</script>");

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::BasicStats(xss);
    }) << "XSS 컬럼명으로 크래시가 발생했다";

    EXPECT_FALSE(result.IsEmpty());
}

TEST_F(SecurityTest, AnalysisTools_HtmlInjectionInCellValue_HandledSafely) {
    DataTable xss = m_sampleTable;
    xss.columns[1].values[0] = _T("<img src=x onerror=alert(1)>");

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::FrequencyDistribution(xss, _T("카테고리"));
    });
    EXPECT_FALSE(result.IsEmpty());
}

// ============================================================
// TC-S-04: Path Traversal 방어
// ============================================================

TEST_F(SecurityTest, DataTable_PathTraversalInFileName_NoCrash) {
    DataTable pt = m_sampleTable;
    pt.fileName = _T("../../../etc/passwd");

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::BasicStats(pt);
    }) << "Path Traversal 파일명으로 크래시가 발생했다";
}

TEST_F(SecurityTest, DataTable_WindowsPathTraversal_NoCrash) {
    DataTable pt = m_sampleTable;
    pt.fileName = _T("..\\..\\Windows\\System32\\cmd.exe");

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::BasicStats(pt);
    });
}

// ============================================================
// TC-S-05: API Key 마스킹
// ============================================================

TEST(SecurityStandaloneTest, AppError_DoesNotExposeApiKey) {
    const CString fakeApiKey = _T("sk-ant-api03-abc123def456");

    AppError err(_T("AUTH_FAILED"), _T("인증 실패"), 2);
    EXPECT_TRUE(err.IsError());

    EXPECT_EQ(err.message.Find(fakeApiKey), -1)
        << "오류 메시지에 API 키가 노출되었다";
}

TEST_F(SecurityTest, AnalysisTools_Output_DoesNotContainApiKey) {
    const CString fakeApiKey = _T("sk-openai-test-key-xyz789");

    DataTable tbl = m_sampleTable;
    tbl.columns[1].values[0] = fakeApiKey;

    CString result = AnalysisTools::BasicStats(tbl);
    EXPECT_EQ(result.Find(fakeApiKey), -1)
        << "BasicStats 결과에 API 키 유사 문자열이 포함되었다";
}

// ============================================================
// TC-S-06: 대용량 입력 처리 (버퍼 오버플로 방지)
// ============================================================

TEST_F(SecurityTest, AnalysisTools_VeryLongColumnName_NoCrash) {
    DataTable large = m_sampleTable;
    std::wstring longName(10000, L'A');
    large.columns[0].name = CString(longName.c_str());

    EXPECT_NO_FATAL_FAILURE({
        CString result = AnalysisTools::BasicStats(large);
    }) << "매우 긴 컬럼명으로 크래시가 발생했다";
}

TEST_F(SecurityTest, AnalysisTools_VeryLongCellValue_NoCrash) {
    DataTable large = m_sampleTable;
    std::wstring longVal(10000, L'B');
    large.columns[1].values[0] = CString(longVal.c_str());

    EXPECT_NO_FATAL_FAILURE({
        CString result = AnalysisTools::FrequencyDistribution(large, _T("카테고리"));
    }) << "매우 긴 셀 값으로 크래시가 발생했다";
}

TEST(SecurityStandaloneTest, BasicStats_1000Rows_NoBufferOverflow) {
    DataTable large;
    large.rowCount = 1000;
    large.colCount = 1;

    DataColumn col;
    col.name = _T("매출");
    col.type = _T("numeric");
    for (int i = 0; i < 1000; ++i) {
        col.values.push_back(CString(std::to_wstring(i * 1000).c_str()));
    }
    large.columns = { col };

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::BasicStats(large);
    }) << "1000행 처리 중 버퍼 오버플로가 발생했다";

    EXPECT_FALSE(result.IsEmpty());
}

// ============================================================
// TC-S-07: NULL / 특수문자 입력 처리
// ============================================================

TEST_F(SecurityTest, AnalysisTools_EmptyColumnName_HandledGracefully) {
    DataTable tbl = m_sampleTable;
    tbl.columns[2].name = _T("");

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::BasicStats(tbl);
    }) << "빈 컬럼명으로 크래시가 발생했다";
}

TEST_F(SecurityTest, AnalysisTools_SpecialCharsInColumnName_NoCrash) {
    DataTable tbl = m_sampleTable;
    tbl.columns[2].name = _T("매출\t금액\n(원)");

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::BasicStats(tbl);
    }) << "탭/개행 포함 컬럼명으로 크래시가 발생했다";
}

TEST_F(SecurityTest, AnalysisTools_ControlChars_NoCrash) {
    DataTable tbl = m_sampleTable;
    tbl.columns[1].values[0] = _T("\x01\x02\x1F");

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::FrequencyDistribution(tbl, _T("카테고리"));
    }) << "제어문자 입력으로 크래시가 발생했다";
}

TEST_F(SecurityTest, AnalysisTools_NegativeNumbers_HandledCorrectly) {
    DataTable tbl;
    tbl.rowCount = 5;
    tbl.colCount = 1;

    DataColumn col;
    col.name = _T("잔액");
    col.type = _T("numeric");
    col.values = {
        _T("-1000"), _T("-500"), _T("0"), _T("500"), _T("1000")
    };
    tbl.columns = { col };

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::BasicStats(tbl);
    }) << "음수 값 처리 중 크래시가 발생했다";

    EXPECT_FALSE(result.IsEmpty());
    EXPECT_NE(result.Find(_T("\"min\"")), -1);
}

TEST_F(SecurityTest, AnalysisTools_VeryLargeNumbers_NoCrash) {
    DataTable tbl;
    tbl.rowCount = 3;
    tbl.colCount = 1;

    DataColumn col;
    col.name = _T("극값");
    col.type = _T("numeric");
    col.values = {
        _T("999999999999999"),
        _T("999999999999998"),
        _T("999999999999997")
    };
    tbl.columns = { col };

    CString result;
    EXPECT_NO_FATAL_FAILURE({
        result = AnalysisTools::BasicStats(tbl);
    }) << "매우 큰 숫자 처리 중 크래시가 발생했다";
}
