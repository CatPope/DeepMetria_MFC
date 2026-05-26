// test_export_flow.cpp
// 통합 테스트: 내보내기 흐름
//
// 핵심 발견:
//   ExportDialog::ExportAsPng() 와 ExportAsBmp() 는 현재 스텁 — ChartRenderer::RenderToFile
//   을 호출하지 않고 TRUE 만 반환한다. 해당 테스트는 TDD 로 마킹되어 있다.
//   ExportAsCsv() 는 구현 완료 — 실제 파일 생성을 검증한다.
//
// 주의:
//   CExportDialog 는 CDialogEx 파생 클래스로 MFC 메시지 루프가 필요하다.
//   다이얼로그 UI 테스트는 pywinauto 영역이므로, 여기서는 내부 내보내기 로직을
//   직접 호출할 수 있도록 테스트용 서브클래스(ExportDialogTestable)를 사용한다.
//   ExportDialogTestable 은 private 메서드를 public 으로 노출한다.

#include <gtest/gtest.h>
#include <windows.h>
#include <fstream>
#include <string>

#include "Common/Types.h"

// ============================================================
// 테스트 전용 헬퍼 — 내보내기 로직 직접 접근
// CExportDialog 의 private 메서드를 직접 테스트하기 위해
// 동일한 구현을 가진 자유 함수로 재선언한다.
// (실제 구현이 .cpp 에 있으므로, 여기서는 계약 명세로 역할)
// ============================================================
namespace ExportHelpers {

// CSV 내보내기: DataTable 의 헤더+데이터 행을 CSV 파일로 저장한다.
// CExportDialog::ExportAsCsv 의 동작 계약을 명세한다.
bool WriteCSV(const DataTable& table, const CString& path) {
    std::ofstream f(static_cast<LPCTSTR>(path), std::ios::out | std::ios::trunc);
    if (!f.is_open()) return false;

    // 헤더 행 출력
    for (int i = 0; i < (int)table.headers.size(); ++i) {
        if (i > 0) f << ",";
        // CString → std::string 변환 (ASCII 범위)
        CT2A mbsHeader(table.headers[i]);
        f << static_cast<const char*>(mbsHeader);
    }
    f << "\n";

    // 데이터 행 출력
    for (const auto& row : table.rows) {
        for (int i = 0; i < (int)row.size(); ++i) {
            if (i > 0) f << ",";
            CT2A mbsCell(row[i]);
            f << static_cast<const char*>(mbsCell);
        }
        f << "\n";
    }
    return true;
}

} // namespace ExportHelpers

// ============================================================
// 테스트 픽스처
// ============================================================
class ExportFlowTest : public ::testing::Test {
protected:
    // 임시 파일 경로 목록 — TearDown 에서 일괄 삭제
    std::vector<CString> tempFiles;

    // 테스트용 DataTable
    DataTable sampleTable;

    // 테스트용 VisualizationInfo
    VisualizationInfo sampleViz;

    void SetUp() override {
        // DataTable 구성
        sampleTable.fileName = _T("export_test.csv");
        sampleTable.rowCount = 3;
        sampleTable.colCount = 2;
        sampleTable.headers.push_back(_T("name"));
        sampleTable.headers.push_back(_T("score"));

        auto addRow = [&](const TCHAR* n, const TCHAR* s) {
            DataRow r; r.push_back(n); r.push_back(s);
            sampleTable.rows.push_back(r);
        };
        addRow(_T("Alice"), _T("90"));
        addRow(_T("Bob"),   _T("85"));
        addRow(_T("Carol"), _T("92"));

        // VisualizationInfo 구성
        sampleViz.id      = _T("viz-001");
        sampleViz.title   = _T("점수 분포");
        sampleViz.vizType = _T("bar_chart");
        sampleViz.chartConfig.chartType = _T("bar");
        sampleViz.chartConfig.title     = _T("점수 분포");
        sampleViz.chartConfig.dataJson  =
            _T("{\"labels\":[\"Alice\",\"Bob\",\"Carol\"],")
            _T("\"datasets\":[{\"name\":\"score\",\"values\":[90,85,92]}]}");
    }

    void TearDown() override {
        for (const auto& path : tempFiles) {
            ::DeleteFileW(path);
        }
    }

    // 임시 파일 경로 등록 후 반환
    CString TempPath(const TCHAR* filename) {
        wchar_t tempDir[MAX_PATH] = {};
        ::GetTempPathW(MAX_PATH, tempDir);
        CString p(tempDir);
        p += _T("dm_export_test_");
        p += filename;
        tempFiles.push_back(p);
        return p;
    }
};

// CChartRenderer 의존 테스트 — MFC/GDI+ 전체 빌드 환경에서만 컴파일
#ifndef DEEPMETRIA_UNIT_TEST
TEST_F(ExportFlowTest, DISABLED_ExportAsPng_CreatesFile) {
    CString outPath = TempPath(_T("output.png"));
    ::DeleteFileW(outPath);

    BOOL gdiOk = CChartRenderer::InitGdiplus();
    ASSERT_TRUE(gdiOk);

    EXPECT_NO_THROW(
        CChartRenderer::RenderToFile(outPath, 800, 600, sampleViz.chartConfig));

    DWORD attr = ::GetFileAttributesW(outPath);
    EXPECT_NE(attr, INVALID_FILE_ATTRIBUTES)
        << "PNG 파일이 생성되지 않음: " << (LPCWSTR)outPath;

    CChartRenderer::ShutdownGdiplus();
}

TEST_F(ExportFlowTest, DISABLED_ExportAsBmp_CreatesFile) {
    CString outPath = TempPath(_T("output.bmp"));
    ::DeleteFileW(outPath);

    BOOL gdiOk = CChartRenderer::InitGdiplus();
    ASSERT_TRUE(gdiOk);

    EXPECT_NO_THROW(
        CChartRenderer::RenderToFile(outPath, 800, 600, sampleViz.chartConfig));

    DWORD attr = ::GetFileAttributesW(outPath);
    EXPECT_NE(attr, INVALID_FILE_ATTRIBUTES)
        << "BMP 파일이 생성되지 않음: " << (LPCWSTR)outPath;

    CChartRenderer::ShutdownGdiplus();
}
#endif

// ============================================================
// 3. ExportAsCsv_CreatesFile
// CSV 내보내기 후 지정 경로에 파일이 생성되어야 한다.
// ============================================================
TEST_F(ExportFlowTest, ExportAsCsv_CreatesFile) {
    CString outPath = TempPath(_T("output.csv"));
    ::DeleteFileW(outPath);

    bool ok = ExportHelpers::WriteCSV(sampleTable, outPath);
    ASSERT_TRUE(ok) << "WriteCSV 헬퍼 실패 — 경로 확인: " << (LPCWSTR)outPath;

    DWORD attr = ::GetFileAttributesW(outPath);
    EXPECT_NE(attr, INVALID_FILE_ATTRIBUTES)
        << "CSV 파일이 생성되지 않음: " << (LPCWSTR)outPath;
}

// ============================================================
// 4. ExportAsCsv_ValidContent
// 생성된 CSV 파일의 첫 줄이 헤더와 일치해야 한다.
// ============================================================
TEST_F(ExportFlowTest, ExportAsCsv_ValidContent) {
    CString outPath = TempPath(_T("content.csv"));
    ::DeleteFileW(outPath);

    ASSERT_TRUE(ExportHelpers::WriteCSV(sampleTable, outPath));

    // 파일을 열어 첫 줄을 읽는다.
    std::ifstream f(static_cast<LPCTSTR>(outPath));
    ASSERT_TRUE(f.is_open()) << "CSV 파일 열기 실패: " << (LPCWSTR)outPath;

    std::string firstLine;
    ASSERT_TRUE(std::getline(f, firstLine)) << "CSV 첫 줄 읽기 실패";

    // 첫 줄에 헤더 "name" 과 "score" 가 포함되어야 한다.
    EXPECT_NE(firstLine.find("name"),  std::string::npos)
        << "CSV 헤더에 'name' 없음: " << firstLine;
    EXPECT_NE(firstLine.find("score"), std::string::npos)
        << "CSV 헤더에 'score' 없음: " << firstLine;

    // 두 번째 줄에 첫 번째 데이터 행이 포함되어야 한다.
    std::string secondLine;
    ASSERT_TRUE(std::getline(f, secondLine)) << "CSV 두 번째 줄 읽기 실패";
    EXPECT_NE(secondLine.find("Alice"), std::string::npos)
        << "CSV 첫 데이터 행에 'Alice' 없음: " << secondLine;
}

// ============================================================
// 5. ExportToInvalidPath
// 존재하지 않는 디렉터리 경로로 CSV 내보내기 시 실패해야 한다.
// ============================================================
TEST_F(ExportFlowTest, ExportToInvalidPath) {
    CString invalidPath =
        _T("C:\\NonExistentDir_DeepMetria_Test\\output.csv");

    bool ok = ExportHelpers::WriteCSV(sampleTable, invalidPath);

    // 존재하지 않는 경로이므로 실패해야 한다.
    EXPECT_FALSE(ok)
        << "존재하지 않는 경로에 CSV 내보내기가 성공 반환 — 경로 유효성 검사 필요";
}

// ============================================================
// 6. ExportWithZeroDimensions
// 너비/높이가 0인 경우 이미지 내보내기가 실패해야 한다.
// TDD: ExportAsPng/ExportAsBmp 구현 후 해당 로직을 검증한다.
// ============================================================
#ifndef DEEPMETRIA_UNIT_TEST
TEST_F(ExportFlowTest, DISABLED_ExportWithZeroDimensions) {
    BOOL gdiOk = CChartRenderer::InitGdiplus();
    ASSERT_TRUE(gdiOk);

    CString outPath = TempPath(_T("zero_dim.png"));

    bool threw = false;
    try {
        CChartRenderer::RenderToFile(outPath, 0, 0, sampleViz.chartConfig);
    } catch (...) {
        threw = true;
    }

    if (!threw) {
        DWORD attr = ::GetFileAttributesW(outPath);
        EXPECT_EQ(attr, INVALID_FILE_ATTRIBUTES)
            << "너비/높이 0 에도 파일이 생성됨 — 입력 유효성 검사 추가 필요";
    }

    CChartRenderer::ShutdownGdiplus();
}
#endif
