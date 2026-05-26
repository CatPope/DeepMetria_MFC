// test_json_parser.cpp — JsonParser TDD 테스트
// Google Test 기반. CMakeLists.txt의 deepmetria_unit_tests 타깃에 자동 포함됨.
//
// [TDD — RED 단계]
// JsonParser는 아직 구현되지 않았다.
// 이 파일의 테스트는 DISABLED_ 접두사로 비활성화되어 있다.
//
// 구현 절차 (Red-Green-Refactor):
//   1. RED  : 이 파일의 DISABLED_ 접두사를 제거하고 빌드 — 컴파일 오류 확인
//   2. GREEN: src/Infrastructure/Parser/JsonParser.h 와 .cpp 최소 구현
//             → 모든 테스트 통과
//   3. REFACTOR: 코드 정리 후 테스트 재실행 — 녹색 유지 확인
//
// 활성화 방법:
//   1. src/Infrastructure/Parser/JsonParser.h / JsonParser.cpp 구현
//   2. 이 파일의 모든 "DISABLED_" 접두사 제거
//   3. CMakeLists.txt에 JsonParser.cpp 소스 추가 (UNIT_SOURCES에 자동 포함됨)
//   4. #include 주석 해제
//
// 예상 인터페이스 (CSVParser/ExcelParser 패턴 일치):
//   class JsonParser {
//   public:
//       static DataTable Parse(const CString& filePath, AppError& outError);
//   };

// MFC 헤더를 Google Test보다 먼저 포함 (PCH 순서 보장)
#include <windows.h>
#include <atlstr.h>   // CString (ATL — MFC 없이 사용 가능)

#include <gtest/gtest.h>
#include <fstream>
#include <string>

// TDD: JsonParser 미구현 — 구현 후 아래 주석 해제
// #include "Infrastructure/Parser/JsonParser.h"

// 공용 타입 (CString, AppError, DataTable 등)
#include "Common/Types.h"

// ============================================================
// 헬퍼 — 임시 파일 생성
// ============================================================
namespace {

struct TempFile {
    std::wstring path;

    explicit TempFile(const std::wstring& suffix = L".json")
    {
        wchar_t tmpDir[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, tmpDir);
        wchar_t tmpFile[MAX_PATH] = {};
        GetTempFileNameW(tmpDir, L"jsn", 0, tmpFile);
        path = std::wstring(tmpFile) + suffix;
        DeleteFileW(tmpFile);
    }

    CString AsCString() const { return CString(path.c_str()); }

    void Write(const std::string& content) const
    {
        std::ofstream f(path, std::ios::binary);
        f.write(content.data(), content.size());
    }

    ~TempFile() { DeleteFileW(path.c_str()); }
};

} // anonymous namespace

// ============================================================
// JsonParser TDD 테스트 스위트
//
// 모든 테스트는 DISABLED_ 접두사로 비활성화되어 있다.
// JsonParser 구현 완료 후 접두사를 제거하여 활성화한다.
// ============================================================
class JsonParserTest : public ::testing::Test {
protected:
    AppError err;
    void SetUp() override { err.Clear(); }
};

// ------------------------------------------------------------
// 1. 유효한 JSON 배열 파싱
//    일관된 키를 가진 객체 배열 → DataTable 변환
//    - headers: JSON 객체의 키 목록
//    - rows: 각 객체의 값 목록 (키 순서 일치)
//    - rowCount: 배열 원소 수
// ------------------------------------------------------------
// TDD: JsonParser 미구현 — 구현 후 활성화
TEST_F(JsonParserTest, DISABLED_ParseValidJsonArray_ReturnsPopulatedDataTable)
{
    // TDD: JsonParser 미구현 — 구현 후 활성화
    TempFile tmp;
    tmp.Write(R"([
        {"이름": "홍길동", "나이": "30", "도시": "서울"},
        {"이름": "김영수", "나이": "25", "도시": "부산"},
        {"이름": "이민아", "나이": "28", "도시": "대구"}
    ])");

    // DataTable table = JsonParser::Parse(tmp.AsCString(), err);

    // EXPECT_FALSE(err.IsError())
    //     << "유효한 JSON 배열 파싱 오류: " << (LPCWSTR)err.code;
    // EXPECT_EQ(table.colCount, 3)
    //     << "JSON 객체 키 3개 → 컬럼 3개";
    // EXPECT_EQ(table.rowCount, 3)
    //     << "JSON 배열 원소 3개 → 행 3개";
    // ASSERT_EQ((int)table.headers.size(), 3);
    // EXPECT_EQ(table.headers[0], _T("이름"));
    // EXPECT_EQ(table.headers[1], _T("나이"));
    // EXPECT_EQ(table.headers[2], _T("도시"));
    // ASSERT_EQ((int)table.rows[0].size(), 3);
    // EXPECT_EQ(table.rows[0][0], _T("홍길동"));
    // EXPECT_EQ(table.rows[0][1], _T("30"));
    // EXPECT_EQ(table.rows[0][2], _T("서울"));
    GTEST_SKIP() << "JsonParser 미구현 — 구현 후 활성화";
}

// ------------------------------------------------------------
// 2. 중첩 JSON 객체 파싱
//    중첩 객체는 플래트닝(점 표기법 키)하거나
//    지원하지 않는 경우 PARSE_ERROR를 반환해야 한다.
//    구현 선택에 따라 동작이 달라질 수 있다.
// ------------------------------------------------------------
// TDD: JsonParser 미구현 — 구현 후 활성화
TEST_F(JsonParserTest, DISABLED_ParseNestedJson_FlattensOrReturnsError)
{
    // TDD: JsonParser 미구현 — 구현 후 활성화
    TempFile tmp;
    tmp.Write(R"([
        {"이름": "홍길동", "주소": {"도시": "서울", "구": "강남구"}},
        {"이름": "김영수", "주소": {"도시": "부산", "구": "해운대구"}}
    ])");

    // DataTable table = JsonParser::Parse(tmp.AsCString(), err);

    // 구현 선택 A: 플래트닝 — 점 표기법 키 사용
    //   EXPECT_FALSE(err.IsError());
    //   EXPECT_EQ(table.colCount, 3); // 이름, 주소.도시, 주소.구
    //
    // 구현 선택 B: 중첩 미지원 — 오류 반환
    //   EXPECT_TRUE(err.IsError());
    //   EXPECT_EQ(err.code, _T("PARSE_ERROR"));
    //
    // 두 선택 중 하나를 구현에 맞게 활성화한다.
    GTEST_SKIP() << "JsonParser 미구현 — 구현 후 활성화";
}

// ------------------------------------------------------------
// 3. 빈 JSON 배열 파싱
//    빈 배열 [] → 0개 행, 헤더 없음
//    NO_DATA_ROWS warning (severity=1) 또는 오류 없음
// ------------------------------------------------------------
// TDD: JsonParser 미구현 — 구현 후 활성화
TEST_F(JsonParserTest, DISABLED_ParseEmptyArray_ReturnsZeroRows)
{
    // TDD: JsonParser 미구현 — 구현 후 활성화
    TempFile tmp;
    tmp.Write("[]");

    // DataTable table = JsonParser::Parse(tmp.AsCString(), err);

    // EXPECT_EQ(table.rowCount, 0)
    //     << "빈 배열은 0개 행을 반환해야 한다";
    // EXPECT_TRUE(table.rows.empty());
    //
    // 빈 배열 처리 정책에 따라 두 가지 중 하나:
    // 정책 A: 오류 없음 (단순히 0개 행)
    //   EXPECT_FALSE(err.IsError());
    // 정책 B: NO_DATA_ROWS warning
    //   EXPECT_EQ(err.code, _T("NO_DATA_ROWS"));
    //   EXPECT_EQ(err.severity, 1); // warning, not error
    //   EXPECT_FALSE(err.IsError());
    GTEST_SKIP() << "JsonParser 미구현 — 구현 후 활성화";
}

// ------------------------------------------------------------
// 4. 잘못된 JSON 형식 파싱
//    JSON 파싱 오류 → PARSE_ERROR 오류 코드 반환
// ------------------------------------------------------------
// TDD: JsonParser 미구현 — 구현 후 활성화
TEST_F(JsonParserTest, DISABLED_ParseInvalidJson_ReturnsParseError)
{
    // TDD: JsonParser 미구현 — 구현 후 활성화
    TempFile tmp;
    tmp.Write("{잘못된 JSON: 따옴표 없는 키, 닫는 괄호 없음");

    // DataTable table = JsonParser::Parse(tmp.AsCString(), err);

    // EXPECT_TRUE(err.IsError())
    //     << "잘못된 JSON은 오류를 반환해야 한다";
    // EXPECT_EQ(err.code, _T("PARSE_ERROR"))
    //     << "오류 코드는 PARSE_ERROR이어야 한다";
    // EXPECT_EQ(table.rowCount, 0);
    // EXPECT_TRUE(table.headers.empty());
    GTEST_SKIP() << "JsonParser 미구현 — 구현 후 활성화";
}

// ------------------------------------------------------------
// 5. 존재하지 않는 파일 파싱
//    FILE_OPEN_FAILED 오류가 설정되어야 한다 (CSVParser와 동일 코드)
// ------------------------------------------------------------
// TDD: JsonParser 미구현 — 구현 후 활성화
TEST_F(JsonParserTest, DISABLED_ParseNonExistentFile_ReturnsFileOpenFailedError)
{
    // TDD: JsonParser 미구현 — 구현 후 활성화
    // CString fakePath = _T("C:\\does\\not\\exist\\fake.json");
    // DataTable table = JsonParser::Parse(fakePath, err);

    // EXPECT_TRUE(err.IsError())
    //     << "존재하지 않는 파일은 오류를 반환해야 한다";
    // EXPECT_EQ(err.code, _T("FILE_OPEN_FAILED"));
    // EXPECT_EQ(table.rowCount, 0);
    // EXPECT_TRUE(table.headers.empty());
    GTEST_SKIP() << "JsonParser 미구현 — 구현 후 활성화";
}

// ------------------------------------------------------------
// 6. 혼합 타입 JSON (string/number/bool/null)
//    모든 값이 CString으로 변환되어야 한다:
//    - string → 그대로
//    - number → 숫자 문자열 ("30", "3.14")
//    - bool   → "TRUE" 또는 "FALSE" (CSVParser 규칙 준수)
//    - null   → "" (빈 문자열)
// ------------------------------------------------------------
// TDD: JsonParser 미구현 — 구현 후 활성화
TEST_F(JsonParserTest, DISABLED_ParseJsonWithMixedTypes_AllValuesConvertedToCString)
{
    // TDD: JsonParser 미구현 — 구현 후 활성화
    TempFile tmp;
    tmp.Write(R"([
        {"이름": "홍길동", "나이": 30, "활성": true,  "비고": null},
        {"이름": "김영수", "나이": 25, "활성": false, "비고": "메모 있음"}
    ])");

    // DataTable table = JsonParser::Parse(tmp.AsCString(), err);

    // EXPECT_FALSE(err.IsError())
    //     << "혼합 타입 JSON 파싱 오류: " << (LPCWSTR)err.code;
    // EXPECT_EQ(table.colCount, 4);
    // EXPECT_EQ(table.rowCount, 2);

    // // 첫 번째 행: 홍길동
    // ASSERT_GE((int)table.rows[0].size(), 4);
    // EXPECT_EQ(table.rows[0][0], _T("홍길동"))   << "string 타입";
    // EXPECT_EQ(table.rows[0][1], _T("30"))       << "number → 문자열";
    // // bool true → "TRUE" (대문자 정책, ExcelParser와 일치)
    // EXPECT_EQ(table.rows[0][2], _T("TRUE"))     << "bool true → TRUE";
    // // null → 빈 문자열 (결측값과 동일 처리)
    // EXPECT_EQ(table.rows[0][3], _T(""))         << "null → 빈 문자열";

    // // 두 번째 행: 김영수
    // ASSERT_GE((int)table.rows[1].size(), 4);
    // EXPECT_EQ(table.rows[1][2], _T("FALSE"))    << "bool false → FALSE";
    // EXPECT_EQ(table.rows[1][3], _T("메모 있음")) << "string 타입";
    GTEST_SKIP() << "JsonParser 미구현 — 구현 후 활성화";
}
