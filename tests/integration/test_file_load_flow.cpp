// test_file_load_flow.cpp
// 통합 테스트: 파일 로드 엔드투엔드 흐름
// 파일 경로 → 파서 선택 → DataTable 생성 전 과정을 검증한다.
//
// 테스트 환경:
//   - 실제 CSVParser / ExcelParser 호출 (mock 미사용)
//   - 테스트 데이터: tests/data/sample_sales.csv
//   - MFC 없이 빌드되므로 CString 대신 리터럴 _T() 매크로 사용

#include <gtest/gtest.h>
#include "Domain/DataSource/DataSourceService.h"
#include "Infrastructure/Parser/CSVParser.h"
#include "Infrastructure/Parser/ExcelParser.h"

// ============================================================
// 테스트 픽스처 — DataSourceService 공유 인스턴스
// ============================================================
class FileLoadFlowTest : public ::testing::Test {
protected:
    DataSourceService service;

    // 테스트 데이터 디렉터리 경로 (CMake 빌드 루트 기준)
    // CMakeLists.txt에서 TEST_DATA_DIR 정의를 기대하지 않으므로
    // 상대 경로를 하드코딩하지 않고 절대 경로 매크로를 사용한다.
    // 빌드 시 -DTEST_DATA_DIR="..." 를 넘기거나, 기본값 사용.
#ifndef TEST_DATA_DIR
    static constexpr const TCHAR* kDataDir = _T("../tests/data/");
#else
    static constexpr const TCHAR* kDataDir = _T(TEST_DATA_DIR);
#endif

    CString DataPath(const TCHAR* filename) const {
        CString path(kDataDir);
        path += filename;
        return path;
    }
};

// ============================================================
// 1. CSVLoadToDataTable
// CSV 파일을 DataSourceService로 로드했을 때
// DataTable 행/열 수가 실제 파일 내용과 일치해야 한다.
// ============================================================
TEST_F(FileLoadFlowTest, CSVLoadToDataTable) {
    DataTable  table;
    AppError   err;

    CString csvPath = DataPath(_T("sample_sales.csv"));
    BOOL ok = service.LoadFile(csvPath, table, err);

    ASSERT_TRUE(ok) << "LoadFile 실패: " << (LPCWSTR)err.message;

    // sample_sales.csv 는 헤더 제외 실제 데이터 행이 1개 이상 존재해야 한다.
    EXPECT_GT(table.rowCount, 0)
        << "DataTable 행 수가 0 — 파싱 결과 확인 필요";

    // 헤더가 1개 이상 파싱되어야 한다.
    EXPECT_GT((int)table.headers.size(), 0)
        << "DataTable 헤더가 비어 있음 — CSVParser 헤더 파싱 확인 필요";

    // colCount 와 headers.size() 가 일치해야 한다.
    EXPECT_EQ(table.colCount, (int)table.headers.size())
        << "colCount 와 headers.size() 불일치";

    // fileName 필드가 원본 경로로 설정되어야 한다.
    EXPECT_FALSE(table.fileName.IsEmpty())
        << "DataTable.fileName 이 비어 있음";
}

// ============================================================
// 2. FileExtensionRouting_CSV
// .csv 확장자 파일은 CSVParser 경로로 라우팅되어야 한다.
// DataSourceService.GetSupportedExtensions() 에 "csv" 포함 여부로 검증.
// ============================================================
TEST_F(FileLoadFlowTest, FileExtensionRouting_CSV) {
    CString supported = DataSourceService::GetSupportedExtensions();

    // 지원 확장자 문자열에 "csv" 가 포함되어야 한다.
    EXPECT_TRUE(supported.Find(_T("csv")) != -1)
        << "GetSupportedExtensions() 에 'csv' 미포함: " << (LPCWSTR)supported;

    // 실제 로드도 성공해야 한다 (라우팅 경로 확인).
    DataTable table;
    AppError  err;
    BOOL ok = service.LoadFile(DataPath(_T("sample_sales.csv")), table, err);
    EXPECT_TRUE(ok) << "CSV 라우팅 로드 실패: " << (LPCWSTR)err.message;
}

// ============================================================
// 3. FileExtensionRouting_XLSX
// .xlsx 확장자 파일은 ExcelParser 경로로 라우팅되어야 한다.
// 테스트 데이터 디렉터리에 sample.xlsx 가 없는 경우 ValidateFile 단계에서
// FILE_NOT_FOUND 오류가 발생하므로, 지원 목록 포함 여부만 검증한다.
// ============================================================
TEST_F(FileLoadFlowTest, FileExtensionRouting_XLSX) {
    CString supported = DataSourceService::GetSupportedExtensions();

    // 지원 확장자 문자열에 "xlsx" 가 포함되어야 한다.
    EXPECT_TRUE(supported.Find(_T("xlsx")) != -1)
        << "GetSupportedExtensions() 에 'xlsx' 미포함: " << (LPCWSTR)supported;
}

// ============================================================
// 4. FileExtensionRouting_JSON  [TDD — JSON 파서 미구현]
// .json 확장자 파일이 지원 목록에 포함되어야 한다.
// JsonParser 구현 후 이 테스트가 GREEN 이 되도록 한다.
// ============================================================
TEST_F(FileLoadFlowTest, DISABLED_FileExtensionRouting_JSON) {
    // TDD: JsonParser 미구현 — JsonParser 연결 후 활성화
    // GetSupportedExtensions() 반환값에 "json" 이 포함되어야 한다.
    CString supported = DataSourceService::GetSupportedExtensions();
    EXPECT_TRUE(supported.Find(_T("json")) != -1)
        << "GetSupportedExtensions() 에 'json' 미포함: " << (LPCWSTR)supported;

    // 실제 JSON 파일 로드도 성공해야 한다.
    DataTable table;
    AppError  err;
    BOOL ok = service.LoadFile(DataPath(_T("sample.json")), table, err);
    EXPECT_TRUE(ok) << "JSON 라우팅 로드 실패: " << (LPCWSTR)err.message;
    EXPECT_GT(table.rowCount, 0);
}

// ============================================================
// 5. UnsupportedExtension
// 지원하지 않는 확장자(.pdf)는 오류를 반환해야 한다.
// ============================================================
TEST_F(FileLoadFlowTest, UnsupportedExtension) {
    DataTable table;
    AppError  err;

    // 존재하지 않아도 확장자 검사가 먼저 실행되어야 한다.
    BOOL ok = service.LoadFile(_T("C:\\fake\\report.pdf"), table, err);

    EXPECT_FALSE(ok)
        << ".pdf 확장자에 대해 TRUE 반환 — 확장자 필터링 로직 확인 필요";
    EXPECT_TRUE(err.IsError())
        << ".pdf 오류 시 AppError 미설정";
}

// ============================================================
// 6. LoadAndVerifyColumnTypes
// CSV 로드 후 DataTable 컬럼 타입(numeric / text / date)이
// 올바르게 감지되어야 한다.
// ============================================================
TEST_F(FileLoadFlowTest, LoadAndVerifyColumnTypes) {
    DataTable table;
    AppError  err;

    CString csvPath = DataPath(_T("sample_sales.csv"));
    BOOL ok = service.LoadFile(csvPath, table, err);
    ASSERT_TRUE(ok) << "LoadFile 실패: " << (LPCWSTR)err.message;

    // columns 벡터가 채워져 있어야 한다 (타입 감지 경로).
    // CSVParser 가 columns 필드를 채우지 않은 경우 이 단계에서 실패한다.
    ASSERT_FALSE(table.columns.empty())
        << "DataTable.columns 가 비어 있음 — 타입 감지 로직 미연결";

    bool foundNumeric = false;
    bool foundText    = false;

    for (const auto& col : table.columns) {
        // 타입 값은 "numeric", "text", "date" 중 하나여야 한다.
        EXPECT_TRUE(
            col.type == _T("numeric") ||
            col.type == _T("text")    ||
            col.type == _T("date"))
            << "컬럼 '" << (LPCWSTR)col.name
            << "' 의 type 값이 예상 범위를 벗어남: " << (LPCWSTR)col.type;

        if (col.type == _T("numeric")) foundNumeric = true;
        if (col.type == _T("text"))    foundText    = true;
    }

    // sample_sales.csv 는 숫자 컬럼과 문자열 컬럼을 모두 포함해야 한다.
    EXPECT_TRUE(foundNumeric) << "수치형 컬럼이 감지되지 않음";
    EXPECT_TRUE(foundText)    << "문자열 컬럼이 감지되지 않음";
}
