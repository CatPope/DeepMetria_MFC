// test_export_formatter.cpp — ExportFormatter 단위 테스트
//
// CSV 수식 인젝션 방지(보안 리뷰 M1), RFC 4180 따옴표 처리, dataJson→CSV/HTML
// 포맷팅을 LLM/UI 없이 결정론적으로 검증한다.
//
// ExportFormatter.h는 헤더 전용(ATL CString + std)이라 MFC/GDI+ 없이 컴파일된다.

#include <gtest/gtest.h>

// MFC 없이 ATL CString만 사용 (LLM 테스트와 동일 패턴)
#include <windows.h>
#include <atlstr.h>

// 테스트 대상 (헤더 전용)
#include "Infrastructure/Export/ExportFormatter.h"

using namespace ExportFormatter;

// ============================================================
// CsvSafeField — 수식 인젝션 방지 + RFC 4180 이스케이프
// ============================================================
TEST(CsvSafeField, NeutralizesFormulaInjectionPayload)
{
    // 고전적 DDE/수식 인젝션 페이로드는 작은따옴표로 무력화되어야 한다.
    CString out = CsvSafeField(_T("=cmd|'/c calc'!A1"));
    ASSERT_GT(out.GetLength(), 0);
    EXPECT_EQ(out[0], _T('\''));
}

TEST(CsvSafeField, NeutralizesLeadingPlusMinusAt)
{
    EXPECT_EQ(CsvSafeField(_T("+1"))[0], _T('\''));
    EXPECT_EQ(CsvSafeField(_T("-1"))[0], _T('\''));
    EXPECT_EQ(CsvSafeField(_T("@x"))[0], _T('\''));
}

TEST(CsvSafeField, WrapsFieldContainingComma)
{
    CString out = CsvSafeField(_T("a,b"));
    EXPECT_EQ(out, _T("\"a,b\""));
}

TEST(CsvSafeField, DoublesAndWrapsFieldContainingQuote)
{
    // RFC 4180: 내부 따옴표는 두 개로 escape하고 전체를 따옴표로 감싼다.
    CString out = CsvSafeField(_T("a\"b"));
    EXPECT_EQ(out, _T("\"a\"\"b\""));
}

TEST(CsvSafeField, LeavesPlainTextUnchanged)
{
    EXPECT_EQ(CsvSafeField(_T("Alice")), _T("Alice"));
}

// ============================================================
// BuildCsvFromDataJson — table 형식
// ============================================================
TEST(BuildCsvFromDataJson, TableFormatHeaderContainsColumnNames)
{
    CString json = _T("{\"columns\":[\"Name\",\"Score\"],\"rows\":[[\"Alice\",\"10\"]]}");
    CString csv = BuildCsvFromDataJson(json);

    // 헤더 줄(첫 줄)에 컬럼 이름이 포함되어야 한다.
    EXPECT_GE(csv.Find(_T("Name")), 0);
    EXPECT_GE(csv.Find(_T("Score")), 0);
}

TEST(BuildCsvFromDataJson, TableFormatNeutralizesFormulaInRowCell)
{
    // 행 셀에 수식 페이로드가 있으면 작은따옴표 접두로 무력화되어야 한다.
    CString json = _T("{\"columns\":[\"Expr\"],\"rows\":[[\"=SUM(A1)\"]]}");
    CString csv = BuildCsvFromDataJson(json);

    // 원본 =SUM 은 그대로 노출되지 않고 '=SUM 으로 무력화된다.
    EXPECT_GE(csv.Find(_T("'=SUM(A1)")), 0);
    EXPECT_LT(csv.Find(_T("\n=SUM(A1)")), 0);  // 줄 시작이 raw '='가 아님
}

// ============================================================
// BuildHtmlTableFromDataJson — XSS 이스케이프 + 표 구조
// ============================================================
TEST(BuildHtmlTableFromDataJson, EscapesScriptTagInCell)
{
    CString json = _T("{\"columns\":[\"Note\"],\"rows\":[[\"<script>alert(1)</script>\"]]}");
    CString html = BuildHtmlTableFromDataJson(json);

    // 원본 <script> 는 그대로 남으면 안 되고 엔티티로 escape되어야 한다.
    EXPECT_LT(html.Find(_T("<script>")), 0);
    EXPECT_GE(html.Find(_T("&lt;script&gt;")), 0);
}

TEST(BuildHtmlTableFromDataJson, ContainsTableStructure)
{
    CString json = _T("{\"columns\":[\"Name\"],\"rows\":[[\"Alice\"]]}");
    CString html = BuildHtmlTableFromDataJson(json);

    EXPECT_GE(html.Find(_T("<table>")), 0);
    EXPECT_GE(html.Find(_T("<tr>")), 0);
    EXPECT_GE(html.Find(_T("<td>")), 0);
}
