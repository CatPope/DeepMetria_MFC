// test_file_parser.cpp — CSVParser / ExcelParser 단위 테스트
// Google Test 기반. CMakeLists.txt의 deepmetria_unit_tests 타깃에 자동 포함됨.
//
// 실행 전제:
//   - tests/data/ 폴더에 샘플 파일 존재 (sample_sales.csv 등)
//   - UNICODE / WIN32_LEAN_AND_MEAN 매크로는 CMakeLists.txt에서 정의됨
//   - CString은 atlstr.h (ATL) 제공 — MFC PCH 불필요

// MFC 헤더를 Google Test보다 먼저 포함 (PCH 순서 보장)
#include <windows.h>
#include <atlstr.h>   // CString (ATL — MFC 없이 사용 가능)

#include <gtest/gtest.h>
#include <fstream>
#include <string>

// 테스트 대상 헤더
#include "Infrastructure/Parser/CSVParser.h"
#include "Infrastructure/Parser/ExcelParser.h"

// ============================================================
// 헬퍼: 테스트 데이터 디렉터리 경로 반환
// CMake 빌드 디렉터리 기준 상대 경로 사용
// ============================================================
namespace {

// 테스트 바이너리 위치와 무관하게 tests/data 폴더를 찾기 위해
// CMake가 정의하는 소스 디렉터리 매크로를 사용한다.
// CMakeLists.txt에 target_compile_definitions으로 TEST_DATA_DIR을 추가하거나
// 아래 fallback 경로를 사용한다.
#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "../../tests/data"
#endif

CString DataPath(const char* filename)
{
    std::string path = std::string(TEST_DATA_DIR) + "/" + filename;
    // std::string(UTF-8) → CString(UTF-16)
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    CString result;
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1,
                        result.GetBufferSetLength(wlen), wlen);
    result.ReleaseBuffer();
    return result;
}

// 임시 파일 생성 헬퍼 — 테스트 종료 시 자동 삭제
struct TempFile {
    std::wstring path;

    explicit TempFile(const std::wstring& suffix = L".csv")
    {
        wchar_t tmpDir[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, tmpDir);
        wchar_t tmpFile[MAX_PATH] = {};
        GetTempFileNameW(tmpDir, L"dmt", 0, tmpFile);
        // GetTempFileName이 생성한 파일을 원하는 확장자로 교체
        path = std::wstring(tmpFile) + suffix;
        // 원본 임시 파일 삭제 (확장자 붙인 새 파일로 대체)
        DeleteFileW(tmpFile);
    }

    CString AsCString() const { return CString(path.c_str()); }

    void Write(const std::string& content) const
    {
        std::ofstream f(path, std::ios::binary);
        f.write(content.data(), content.size());
    }

    ~TempFile()
    {
        DeleteFileW(path.c_str());
    }
};

} // anonymous namespace

// ============================================================
// CSVParser 테스트 스위트
// ============================================================
class CSVParserTest : public ::testing::Test {
protected:
    AppError err;

    void SetUp() override { err.Clear(); }
};

// ------------------------------------------------------------
// 1. 표준 CSV 파싱 — sample_sales.csv
//    10개 데이터 행, 5개 컬럼 (날짜/카테고리/매출/수량/지역)
// ------------------------------------------------------------
TEST_F(CSVParserTest, ParseStandardCSV_Returns10RowsAnd5Columns)
{
    DataTable table = CSVParser::Parse(DataPath("sample_sales.csv"), err);

    // 오류 없음 (헤더만 파일은 severity=1 warning이지만 여기는 데이터 있음)
    EXPECT_TRUE(err.code.IsEmpty())
        << "오류 코드: " << (LPCWSTR)err.code;

    // 컬럼 수 검증
    EXPECT_EQ(table.colCount, 5);

    // 행 수 검증
    EXPECT_EQ(table.rowCount, 10);
    EXPECT_EQ((int)table.rows.size(), 10);

    // 헤더 이름 검증
    ASSERT_EQ((int)table.headers.size(), 5);
    EXPECT_EQ(table.headers[0], _T("날짜"));
    EXPECT_EQ(table.headers[1], _T("카테고리"));
    EXPECT_EQ(table.headers[2], _T("매출"));
    EXPECT_EQ(table.headers[3], _T("수량"));
    EXPECT_EQ(table.headers[4], _T("지역"));

    // 첫 번째 행 값 검증
    ASSERT_EQ((int)table.rows[0].size(), 5);
    EXPECT_EQ(table.rows[0][0], _T("2024-01"));
    EXPECT_EQ(table.rows[0][1], _T("전자제품"));
    EXPECT_EQ(table.rows[0][2], _T("1500000"));
    EXPECT_EQ(table.rows[0][3], _T("120"));
    EXPECT_EQ(table.rows[0][4], _T("서울"));
}

// ------------------------------------------------------------
// 2. 넓은 CSV 파싱 — sample_wide.csv
//    5개 행, 10개 컬럼
// ------------------------------------------------------------
TEST_F(CSVParserTest, ParseWideCSV_Returns5RowsAnd10Columns)
{
    DataTable table = CSVParser::Parse(DataPath("sample_wide.csv"), err);

    EXPECT_TRUE(err.code.IsEmpty())
        << "오류 코드: " << (LPCWSTR)err.code;
    EXPECT_EQ(table.colCount, 10);
    EXPECT_EQ(table.rowCount, 5);
}

// ------------------------------------------------------------
// 3. 좁은 CSV 파싱 — sample_narrow.csv
//    3개 행, 3개 컬럼
// ------------------------------------------------------------
TEST_F(CSVParserTest, ParseNarrowCSV_Returns3RowsAnd3Columns)
{
    DataTable table = CSVParser::Parse(DataPath("sample_narrow.csv"), err);

    EXPECT_TRUE(err.code.IsEmpty())
        << "오류 코드: " << (LPCWSTR)err.code;
    EXPECT_EQ(table.colCount, 3);
    EXPECT_EQ(table.rowCount, 3);

    ASSERT_EQ((int)table.headers.size(), 3);
    EXPECT_EQ(table.headers[0], _T("항목"));
    EXPECT_EQ(table.headers[1], _T("값"));
    EXPECT_EQ(table.headers[2], _T("비고"));
}

// ------------------------------------------------------------
// 4. 결측값 처리 — sample_missing.csv
//    빈 셀은 빈 CString("")으로 파싱되어야 한다
// ------------------------------------------------------------
TEST_F(CSVParserTest, ParseMissingValues_EmptyCellsReturnEmptyString)
{
    DataTable table = CSVParser::Parse(DataPath("sample_missing.csv"), err);

    // 파싱 자체는 성공해야 함 (오류 코드 없음 또는 severity < 2)
    EXPECT_FALSE(err.IsError())
        << "예상치 못한 오류: " << (LPCWSTR)err.code;

    // sample_missing.csv: 이름/나이/도시/직업/연봉 — 8개 데이터 행
    EXPECT_EQ(table.colCount, 5);
    EXPECT_GT(table.rowCount, 0);

    // 이영희 행(index 1): 나이 빈값, 직업 빈값
    ASSERT_GT((int)table.rows.size(), 1);
    EXPECT_EQ(table.rows[1][1], _T(""))  << "나이 결측값은 빈 문자열이어야 한다";
    EXPECT_EQ(table.rows[1][3], _T(""))  << "직업 결측값은 빈 문자열이어야 한다";

    // 오세훈 행(index 6): 모든 값 빈값
    ASSERT_GT((int)table.rows.size(), 6);
    for (int col = 1; col < table.colCount; ++col) {
        EXPECT_EQ(table.rows[6][col], _T(""))
            << "컬럼 " << col << " 결측값은 빈 문자열이어야 한다";
    }
}

// ------------------------------------------------------------
// 5. 헤더만 있는 파일 — sample_header_only.csv
//    데이터 행 0개, 헤더 존재, NO_DATA_ROWS warning
// ------------------------------------------------------------
TEST_F(CSVParserTest, ParseHeaderOnly_ReturnsZeroRowsWithWarning)
{
    DataTable table = CSVParser::Parse(DataPath("sample_header_only.csv"), err);

    // 헤더만 있는 경우 severity=1(warning) — IsError()는 FALSE
    EXPECT_FALSE(err.IsError())
        << "헤더만 있는 파일은 치명적 오류가 아니어야 한다";
    EXPECT_EQ(err.code, _T("NO_DATA_ROWS"))
        << "NO_DATA_ROWS warning이 설정되어야 한다";

    // 헤더는 파싱되어야 함
    EXPECT_EQ(table.colCount, 3);
    ASSERT_EQ((int)table.headers.size(), 3);
    EXPECT_EQ(table.headers[0], _T("컬럼A"));
    EXPECT_EQ(table.headers[1], _T("컬럼B"));
    EXPECT_EQ(table.headers[2], _T("컬럼C"));

    // 데이터 행 없음
    EXPECT_EQ(table.rowCount, 0);
    EXPECT_TRUE(table.rows.empty());
}

// ------------------------------------------------------------
// 6. 잘못된 파일 확장자 — sample_invalid.txt
//    CSVParser는 확장자를 검사하지 않지만 일반 텍스트는
//    헤더/행 파싱을 시도한다. 오류 없이 파싱되거나
//    EMPTY_FILE 오류가 발생할 수 있다.
//    핵심: 크래시가 없어야 한다.
// ------------------------------------------------------------
TEST_F(CSVParserTest, ParseInvalidFile_DoesNotCrash)
{
    // CSVParser는 확장자 검사를 하지 않으므로 텍스트를 CSV로 파싱 시도
    // 크래시 없이 결과를 반환해야 한다
    DataTable table = CSVParser::Parse(DataPath("sample_invalid.txt"), err);

    // 파싱 결과는 구현에 따르지만 크래시는 없어야 함
    // (이 테스트의 핵심 assertion은 실행이 완료되는 것 자체)
    SUCCEED() << "sample_invalid.txt 파싱 시 크래시가 발생하지 않았다";
}

// ------------------------------------------------------------
// 7. 존재하지 않는 파일 — 가짜 경로
//    FILE_OPEN_FAILED 오류가 설정되어야 한다
// ------------------------------------------------------------
TEST_F(CSVParserTest, ParseNonExistentFile_ReturnsFileOpenFailedError)
{
    CString fakePath = _T("C:\\does\\not\\exist\\fake_file.csv");
    DataTable table = CSVParser::Parse(fakePath, err);

    EXPECT_TRUE(err.IsError())
        << "존재하지 않는 파일은 오류를 반환해야 한다";
    EXPECT_EQ(err.code, _T("FILE_OPEN_FAILED"))
        << "오류 코드는 FILE_OPEN_FAILED이어야 한다";
    EXPECT_TRUE(table.headers.empty())
        << "실패 시 빈 DataTable이 반환되어야 한다";
    EXPECT_EQ(table.rowCount, 0);
}

// ------------------------------------------------------------
// 8. 빈 경로 — 빈 CString
//    FILE_OPEN_FAILED 오류가 설정되어야 한다
// ------------------------------------------------------------
TEST_F(CSVParserTest, ParseEmptyPath_ReturnsFileOpenFailedError)
{
    CString emptyPath = _T("");
    DataTable table = CSVParser::Parse(emptyPath, err);

    EXPECT_TRUE(err.IsError())
        << "빈 경로는 오류를 반환해야 한다";
    EXPECT_EQ(err.code, _T("FILE_OPEN_FAILED"));
    EXPECT_EQ(table.rowCount, 0);
}

// ------------------------------------------------------------
// 9. 탭 구분자 자동 감지
//    탭으로 구분된 임시 파일을 생성하여 올바르게 파싱하는지 검증
// ------------------------------------------------------------
TEST_F(CSVParserTest, DelimiterAutoDetection_TabDelimitedFileIsCorrectlyParsed)
{
    TempFile tmp(L".csv");
    // UTF-8 탭 구분 CSV
    tmp.Write("이름\t나이\t도시\r\n"
              "홍길동\t30\t서울\r\n"
              "김영수\t25\t부산\r\n");

    DataTable table = CSVParser::Parse(tmp.AsCString(), err);

    EXPECT_FALSE(err.IsError())
        << "탭 구분 파일 파싱 오류: " << (LPCWSTR)err.code;
    EXPECT_EQ(table.colCount, 3)
        << "탭 구분자를 올바르게 감지하여 3개 컬럼이어야 한다";
    EXPECT_EQ(table.rowCount, 2);
    EXPECT_EQ(table.headers[0], _T("이름"));
    EXPECT_EQ(table.headers[1], _T("나이"));
    EXPECT_EQ(table.headers[2], _T("도시"));
}

// ------------------------------------------------------------
// 10. 대용량 파일 처리 — 10,000행 임시 파일
//     크래시 없이 정확한 행 수가 파싱되어야 한다
// ------------------------------------------------------------
TEST_F(CSVParserTest, LargeFileHandling_10000RowsNoCrashAndCorrectCount)
{
    TempFile tmp(L".csv");

    std::string content = "ID,값,설명\r\n";
    content.reserve(content.size() + 10000 * 30);
    for (int i = 1; i <= 10000; ++i) {
        content += std::to_string(i) + ","
                 + std::to_string(i * 100) + ","
                 + "테스트데이터\r\n";
    }
    tmp.Write(content);

    DataTable table = CSVParser::Parse(tmp.AsCString(), err);

    EXPECT_FALSE(err.IsError())
        << "대용량 파일 파싱 오류: " << (LPCWSTR)err.code;
    EXPECT_EQ(table.colCount, 3);
    EXPECT_EQ(table.rowCount, 10000)
        << "10,000행이 잘리지 않고 모두 파싱되어야 한다";
}

// ------------------------------------------------------------
// 11. UTF-8 BOM + 한국어 텍스트
//     sample_korean_path_test.csv: 한국어 컬럼명/값 검증
// ------------------------------------------------------------
TEST_F(CSVParserTest, EncodingUTF8BOM_KoreanTextIsCorrectlyDecoded)
{
    DataTable table = CSVParser::Parse(
        DataPath("sample_korean_path_test.csv"), err);

    EXPECT_FALSE(err.IsError())
        << "한국어 CSV 파싱 오류: " << (LPCWSTR)err.code;
    EXPECT_GT(table.colCount, 0)
        << "컬럼이 1개 이상 파싱되어야 한다";
    EXPECT_GT((int)table.headers.size(), 0);

    // 헤더가 깨지지 않고 한국어 문자를 포함하는지 검증
    // (빈 문자열이나 물음표 대체 문자가 아닌 실제 문자)
    bool hasKoreanHeader = false;
    for (const auto& h : table.headers) {
        for (int i = 0; i < h.GetLength(); ++i) {
            WCHAR ch = h[i];
            // 한글 유니코드 범위: AC00–D7A3 (완성형), 1100–11FF (자모)
            if ((ch >= 0xAC00 && ch <= 0xD7A3) ||
                (ch >= 0x1100 && ch <= 0x11FF)) {
                hasKoreanHeader = true;
                break;
            }
        }
        if (hasKoreanHeader) break;
    }
    EXPECT_TRUE(hasKoreanHeader)
        << "헤더에 한국어 문자가 포함되어야 한다 (인코딩 깨짐 없음)";
}

// ------------------------------------------------------------
// 12. 따옴표로 묶인 셀 — 셀 내부에 쉼표가 포함된 경우
//     "cell,with,commas" → 단일 셀로 파싱되어야 한다
// ------------------------------------------------------------
TEST_F(CSVParserTest, QuotedCells_CommaInsideQuotesIsNotTreatedAsDelimiter)
{
    TempFile tmp(L".csv");
    // RFC 4180 준수: 셀 내부 쉼표는 따옴표로 묶어야 함
    tmp.Write("제목,설명,태그\r\n"
              "항목1,\"쉼표,포함,설명\",태그A\r\n"
              "항목2,일반설명,태그B\r\n");

    DataTable table = CSVParser::Parse(tmp.AsCString(), err);

    EXPECT_FALSE(err.IsError())
        << "따옴표 셀 파싱 오류: " << (LPCWSTR)err.code;
    EXPECT_EQ(table.colCount, 3)
        << "따옴표 내 쉼표를 구분자로 처리하지 않아야 한다";
    EXPECT_EQ(table.rowCount, 2);

    // 따옴표가 제거되고 내용이 보존되어야 함
    ASSERT_GE((int)table.rows[0].size(), 2);
    EXPECT_EQ(table.rows[0][1], _T("쉼표,포함,설명"))
        << "따옴표 제거 후 쉼표 포함 내용이 그대로 보존되어야 한다";
}

// ============================================================
// ExcelParser 테스트 스위트
// ============================================================
class ExcelParserTest : public ::testing::Test {
protected:
    AppError err;

    void SetUp() override { err.Clear(); }
};

// ------------------------------------------------------------
// E1. 존재하지 않는 xlsx 파일 — FILE_NOT_FOUND 오류
// ------------------------------------------------------------
TEST_F(ExcelParserTest, ParseNonExistentXlsx_ReturnsFileNotFoundError)
{
    CString fakePath = _T("C:\\does\\not\\exist\\fake.xlsx");
    DataTable table = ExcelParser::Parse(fakePath, err);

    EXPECT_TRUE(err.IsError())
        << "존재하지 않는 xlsx는 오류를 반환해야 한다";
    EXPECT_EQ(err.code, _T("FILE_NOT_FOUND"))
        << "오류 코드는 FILE_NOT_FOUND이어야 한다";
    EXPECT_TRUE(table.headers.empty());
    EXPECT_EQ(table.rowCount, 0);
}

// ------------------------------------------------------------
// E2. 잘못된 확장자 (.txt) — 파일이 존재하더라도 xlsx 구조 검증 실패
//     또는 라이브러리 없는 경우 EXCEL_LIB_MISSING 오류
// ------------------------------------------------------------
TEST_F(ExcelParserTest, ParseInvalidExtension_ReturnsErrorForNonXlsxFile)
{
    // sample_invalid.txt는 실제 존재하는 파일 (텍스트 내용)
    CString txtPath = DataPath("sample_invalid.txt");
    DataTable table = ExcelParser::Parse(txtPath, err);

    // ExcelParser는 ValidateXlsx에서 파일 존재 여부만 확인하고
    // HAVE_OPENXLSX 없이는 EXCEL_LIB_MISSING을 반환한다.
    // 두 가지 결과 모두 허용: IsError()가 TRUE이어야 함
    EXPECT_TRUE(err.IsError())
        << "txt 파일의 xlsx 파싱은 오류를 반환해야 한다. "
           "오류 코드: " << (LPCWSTR)err.code;
    EXPECT_TRUE(table.headers.empty())
        << "실패 시 빈 DataTable이 반환되어야 한다";
}

// JsonParser TDD 테스트는 tests/unit/test_json_parser.cpp 에 통합됨 (ODR 위반 방지)
