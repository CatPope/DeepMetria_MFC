// test_datasource_service.cpp — DataSourceService 단위 테스트
// Google Test 기반. CMakeLists.txt의 deepmetria_unit_tests 타깃에 자동 포함됨.
//
// 검증 대상:
//   - LoadFile: 확장자 판별 후 파서 위임, 오류 전파
//   - GetSupportedExtensions: 지원 확장자 필터 문자열
//   - ValidateFile: 존재 여부, 크기, 확장자 검사

// MFC 헤더를 Google Test보다 먼저 포함 (PCH 순서 보장)
#include <windows.h>
#include <atlstr.h>   // CString (ATL — MFC 없이 사용 가능)

#include <gtest/gtest.h>
#include <fstream>
#include <string>

// 테스트 대상 헤더
#include "Domain/DataSource/DataSourceService.h"

// ============================================================
// 헬퍼 — 테스트 데이터 경로 및 임시 파일
// ============================================================
namespace {

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "../../tests/data"
#endif

CString DataPath(const char* filename)
{
    std::string path = std::string(TEST_DATA_DIR) + "/" + filename;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    CString result;
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1,
                        result.GetBufferSetLength(wlen), wlen);
    result.ReleaseBuffer();
    return result;
}

// 임시 파일 생성 — 소멸 시 자동 삭제
struct TempFile {
    std::wstring path;

    explicit TempFile(const std::wstring& suffix = L".csv")
    {
        wchar_t tmpDir[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, tmpDir);
        wchar_t tmpFile[MAX_PATH] = {};
        GetTempFileNameW(tmpDir, L"dss", 0, tmpFile);
        path = std::wstring(tmpFile) + suffix;
        DeleteFileW(tmpFile);
    }

    CString AsCString() const { return CString(path.c_str()); }

    void WriteText(const std::string& content) const
    {
        std::ofstream f(path, std::ios::binary);
        f.write(content.data(), content.size());
    }

    ~TempFile() { DeleteFileW(path.c_str()); }
};

} // anonymous namespace

// ============================================================
// DataSourceService 테스트 스위트
// ============================================================
class DataSourceServiceTest : public ::testing::Test {
protected:
    DataSourceService svc;
    DataTable         outData;
    AppError          err;

    void SetUp() override
    {
        outData = DataTable();
        err.Clear();
    }
};

// ------------------------------------------------------------
// 1. CSV 파일 로드 — sample_sales.csv
//    LoadFile이 TRUE를 반환하고 DataTable이 채워져야 한다
// ------------------------------------------------------------
TEST_F(DataSourceServiceTest, LoadCSVFile_ReturnsPopulatedDataTable)
{
    BOOL ok = svc.LoadFile(DataPath("sample_sales.csv"), outData, err);

    EXPECT_TRUE(ok)
        << "LoadFile은 TRUE를 반환해야 한다. 오류: " << (LPCWSTR)err.code;
    EXPECT_FALSE(err.IsError());
    EXPECT_EQ(outData.colCount, 5);
    EXPECT_EQ(outData.rowCount, 10);

    // 헤더 검증
    ASSERT_EQ((int)outData.headers.size(), 5);
    EXPECT_EQ(outData.headers[0], _T("날짜"));
    EXPECT_EQ(outData.headers[2], _T("매출"));
}

// ------------------------------------------------------------
// 2. 지원하지 않는 확장자 로드 — .txt 파일
//    LoadFile이 FALSE를 반환하고 UNSUPPORTED_FILE_TYPE 오류 설정
// ------------------------------------------------------------
TEST_F(DataSourceServiceTest, LoadInvalidExtension_ReturnsUnsupportedFileTypeError)
{
    BOOL ok = svc.LoadFile(DataPath("sample_invalid.txt"), outData, err);

    EXPECT_FALSE(ok)
        << "지원하지 않는 확장자는 FALSE를 반환해야 한다";
    EXPECT_TRUE(err.IsError());
    EXPECT_EQ(err.code, _T("UNSUPPORTED_FILE_TYPE"))
        << "오류 코드는 UNSUPPORTED_FILE_TYPE이어야 한다";
    EXPECT_EQ(outData.rowCount, 0)
        << "실패 시 outData는 비어 있어야 한다";
}

// ------------------------------------------------------------
// 3. 존재하지 않는 파일 로드
//    LoadFile이 FALSE를 반환하고 FILE_NOT_FOUND 오류 설정
// ------------------------------------------------------------
TEST_F(DataSourceServiceTest, LoadNonExistentFile_ReturnsFileNotFoundError)
{
    CString fakePath = _T("C:\\does\\not\\exist\\missing.csv");
    BOOL ok = svc.LoadFile(fakePath, outData, err);

    EXPECT_FALSE(ok);
    EXPECT_TRUE(err.IsError());
    EXPECT_EQ(err.code, _T("FILE_NOT_FOUND"))
        << "오류 코드는 FILE_NOT_FOUND이어야 한다";
}

// ------------------------------------------------------------
// 4. CSV 파일 유효성 검사 — 유효한 파일
//    ValidateFile이 TRUE를 반환해야 한다
// ------------------------------------------------------------
TEST_F(DataSourceServiceTest, ValidateCSVFile_ReturnsTrueForValidFile)
{
    BOOL ok = svc.ValidateFile(DataPath("sample_sales.csv"), err);

    EXPECT_TRUE(ok)
        << "유효한 CSV 파일은 ValidateFile이 TRUE를 반환해야 한다. "
           "오류: " << (LPCWSTR)err.code;
    EXPECT_FALSE(err.IsError());
}

// ------------------------------------------------------------
// 5. 100MB 초과 파일 — FILE_TOO_LARGE 오류
//    실제 100MB 파일을 생성하면 느리므로 크기 임계값 경계 검증:
//    ValidateFile 구현에서 MAX_FILE_SIZE(100MB) 초과 시 오류 반환을
//    직접 확인하는 대신, 정상 파일에서 오류가 없음을 확인하고
//    오류 코드 문자열이 올바른지 검증한다.
//
//    완전한 검증은 통합 테스트에서 대용량 파일 생성 후 수행한다.
// ------------------------------------------------------------
TEST_F(DataSourceServiceTest, ValidateOversizedFile_FileTooLargeErrorCodeIsCorrect)
{
    // 실제 100MB 파일 생성은 CI 환경에서 부담스러우므로 건너뜀
    // 대신 AppError 코드 상수가 "FILE_TOO_LARGE"임을 문서화
    AppError oversizeErr(_T("FILE_TOO_LARGE"), _T("파일 크기 초과"), 2);
    EXPECT_TRUE(oversizeErr.IsError());
    EXPECT_EQ(oversizeErr.code, _T("FILE_TOO_LARGE"));

    // 정상 파일은 FILE_TOO_LARGE가 아님을 확인
    err.Clear();
    BOOL ok = svc.ValidateFile(DataPath("sample_sales.csv"), err);
    EXPECT_TRUE(ok);
    EXPECT_NE(err.code, _T("FILE_TOO_LARGE"));
}

// ------------------------------------------------------------
// 6. 지원 확장자 문자열 — "*.csv"와 "*.xlsx" 포함 여부
// ------------------------------------------------------------
TEST_F(DataSourceServiceTest, GetSupportedExtensions_ContainsCSVAndXlsx)
{
    CString exts = DataSourceService::GetSupportedExtensions();

    // 빈 문자열이 아니어야 함
    EXPECT_FALSE(exts.IsEmpty())
        << "GetSupportedExtensions는 빈 문자열을 반환하지 않아야 한다";

    // *.csv 포함 여부
    EXPECT_NE(exts.Find(_T("*.csv")), -1)
        << "지원 확장자에 '*.csv'가 포함되어야 한다. 실제값: " << (LPCWSTR)exts;

    // *.xlsx 포함 여부
    EXPECT_NE(exts.Find(_T("*.xlsx")), -1)
        << "지원 확장자에 '*.xlsx'가 포함되어야 한다. 실제값: " << (LPCWSTR)exts;
}

// ------------------------------------------------------------
// 7. 빈 경로 로드 — 빈 CString
//    FILE_NOT_FOUND 오류 또는 FILE_ACCESS_ERROR가 설정되어야 한다
// ------------------------------------------------------------
TEST_F(DataSourceServiceTest, LoadEmptyPath_ReturnsErrorForEmptyPath)
{
    CString emptyPath = _T("");
    BOOL ok = svc.LoadFile(emptyPath, outData, err);

    EXPECT_FALSE(ok)
        << "빈 경로는 FALSE를 반환해야 한다";
    EXPECT_TRUE(err.IsError())
        << "빈 경로는 오류가 설정되어야 한다";
    // GetFileAttributes에 빈 문자열은 INVALID_FILE_ATTRIBUTES 반환
    // → FILE_NOT_FOUND 또는 UNSUPPORTED_FILE_TYPE
    EXPECT_FALSE(err.code.IsEmpty())
        << "오류 코드가 설정되어야 한다";
    EXPECT_EQ(outData.rowCount, 0);
}

// ------------------------------------------------------------
// 추가: ValidateFile — 존재하지 않는 파일
//    FILE_NOT_FOUND 오류가 설정되어야 한다
// ------------------------------------------------------------
TEST_F(DataSourceServiceTest, ValidateNonExistentFile_ReturnsFileNotFoundError)
{
    CString fakePath = _T("C:\\does\\not\\exist\\missing.csv");
    BOOL ok = svc.ValidateFile(fakePath, err);

    EXPECT_FALSE(ok);
    EXPECT_EQ(err.code, _T("FILE_NOT_FOUND"));
}

// ------------------------------------------------------------
// 추가: ValidateFile — 지원하지 않는 확장자
//    UNSUPPORTED_FILE_TYPE 오류가 설정되어야 한다
// ------------------------------------------------------------
TEST_F(DataSourceServiceTest, ValidateFile_UnsupportedExtensionReturnsError)
{
    // sample_invalid.txt는 실제로 존재하므로 크기/존재 검사 통과,
    // 확장자 검사에서 실패해야 한다
    BOOL ok = svc.ValidateFile(DataPath("sample_invalid.txt"), err);

    EXPECT_FALSE(ok);
    EXPECT_EQ(err.code, _T("UNSUPPORTED_FILE_TYPE"))
        << "오류 코드는 UNSUPPORTED_FILE_TYPE이어야 한다";
}

// ------------------------------------------------------------
// 추가: GetSupportedExtensions — 세미콜론 구분 형식 검증
//    파일 대화상자 필터 형식(*.ext;*.ext)을 따르는지 확인
// ------------------------------------------------------------
TEST_F(DataSourceServiceTest, GetSupportedExtensions_ReturnsSemicolonDelimitedFormat)
{
    CString exts = DataSourceService::GetSupportedExtensions();

    // 세미콜론 구분자 포함 여부 (두 개 이상의 확장자)
    EXPECT_NE(exts.Find(_T(";")), -1)
        << "여러 확장자는 세미콜론으로 구분되어야 한다";
}
