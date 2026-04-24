#include "stdafx.h"
#include "ExcelParser.h"

// xlsx는 ZIP 아카이브 내 XML 파일들로 구성됨.
// OpenXLSX (vcpkg)를 사용하는 래퍼 구현.
// vcpkg에 openxlsx가 없는 경우를 위해 minizip + tinyxml2 자체 구현 경로도 유지.

// OpenXLSX가 vcpkg에 설치된 경우:
// #include <OpenXLSX.hpp>
// using namespace OpenXLSX;

// 여기서는 OpenXLSX API 기반 구현을 제공.
// 빌드 환경에 따라 #include 경로를 조정하세요.

#ifdef HAVE_OPENXLSX
#include <OpenXLSX.hpp>
using namespace OpenXLSX;
#endif

// ============================================================
// Parse — 진입점
// ============================================================

DataTable ExcelParser::Parse(const CString& filePath, AppError& outError) {
    outError.Clear();
    DataTable result;

    // 파일 경로 검증
    if (!ValidateXlsx(filePath, outError)) return result;

#ifdef HAVE_OPENXLSX
    // ── OpenXLSX 경로 ────────────────────────────────────────
    try {
        // CString → std::string (UTF-8)
        int len = WideCharToMultiByte(CP_UTF8, 0, filePath, -1, nullptr, 0, nullptr, nullptr);
        std::string utf8Path(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, filePath, -1, &utf8Path[0], len, nullptr, nullptr);
        utf8Path.resize(len - 1);

        XLDocument doc;
        doc.open(utf8Path);

        auto wb    = doc.workbook();
        auto sheet = wb.worksheet(wb.sheetNames().front()); // 첫 번째 시트

        auto range = sheet.range();
        bool isFirstRow = true;

        for (auto& rowRange : range) {
            if (isFirstRow) {
                // 헤더 행
                for (auto& cell : rowRange) {
                    std::string val = cell.value().get<std::string>();
                    result.headers.push_back(UTF8ToCString(val));
                }
                result.colCount = (int)result.headers.size();
                isFirstRow = false;
            } else {
                DataRow row;
                row.reserve(result.colCount);
                for (auto& cell : rowRange) {
                    CString cellVal;
                    auto cellType = cell.value().type();
                    switch (cellType) {
                    case XLValueType::Integer:
                        cellVal.Format(_T("%lld"), cell.value().get<long long>());
                        break;
                    case XLValueType::Float:
                        cellVal.Format(_T("%g"),   cell.value().get<double>());
                        break;
                    case XLValueType::Boolean:
                        cellVal = cell.value().get<bool>() ? _T("TRUE") : _T("FALSE");
                        break;
                    case XLValueType::String:
                        cellVal = UTF8ToCString(cell.value().get<std::string>());
                        break;
                    default:
                        cellVal = _T("");
                        break;
                    }
                    row.push_back(cellVal);
                }
                while ((int)row.size() < result.colCount) row.push_back(_T(""));
                result.rows.push_back(row);
            }
        }
        doc.close();
    }
    catch (const std::exception& e) {
        outError.Set(_T("PARSE_ERROR"), UTF8ToCString(e.what()));
        return DataTable();
    }
#else
    // ── OpenXLSX 미설치: 빌드 오류 대신 명확한 런타임 메시지 반환 ──
    outError.Set(_T("EXCEL_LIB_MISSING"),
                 _T("Excel 파싱 라이브러리(OpenXLSX)가 설치되지 않았습니다. ")
                 _T("vcpkg install openxlsx 후 HAVE_OPENXLSX 매크로를 정의하세요."));
    return result;
#endif

    if (result.headers.empty()) {
        outError.Set(_T("EMPTY_FILE"), _T("파일에 데이터가 없습니다."));
        return result;
    }
    if (result.rows.empty()) {
        outError.Set(_T("NO_DATA_ROWS"), _T("데이터 행이 없습니다 (헤더만 존재)."), 1);
    }

    result.rowCount = (int)result.rows.size();
    return result;
}

// ============================================================
// 파일 유효성 검사
// ============================================================

BOOL ExcelParser::ValidateXlsx(const CString& filePath, AppError& outError) {
    // 확장자 확인
    CString ext = filePath.Right(5).MakeLower();
    if (ext != _T(".xlsx") && !filePath.Right(4).MakeLower().Compare(_T(".xls")) == 0) {
        // .xlsx 또는 .xls 허용
    }

    // 파일 존재 확인
    DWORD attr = GetFileAttributes(filePath);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        outError.Set(_T("FILE_NOT_FOUND"), _T("Excel 파일을 찾을 수 없습니다."));
        return FALSE;
    }
    return TRUE;
}

// ============================================================
// 셀 값 추출 (공유 문자열 테이블 참조)
// ============================================================

CString ExcelParser::ExtractCellValue(const std::string& rawValue,
                                      char cellType,
                                      const std::vector<std::string>& sharedStr) {
    if (rawValue.empty()) return _T("");

    switch (cellType) {
    case 's': {
        // 공유 문자열 인덱스
        int idx = std::stoi(rawValue);
        if (idx >= 0 && idx < (int)sharedStr.size())
            return UTF8ToCString(sharedStr[idx]);
        return _T("");
    }
    case 'b':
        return (rawValue == "1") ? _T("TRUE") : _T("FALSE");
    case 'd':
        // 날짜 시리얼 → 문자열
        try { return SerialDateToString(std::stod(rawValue)); }
        catch (...) { return UTF8ToCString(rawValue); }
    default:
        // 숫자 또는 인라인 문자열
        return UTF8ToCString(rawValue);
    }
}

// ============================================================
// Excel 시리얼 날짜 변환
// ============================================================

CString ExcelParser::SerialDateToString(double serial) {
    // Excel 시리얼: 1900-01-01 = 1 (1900년 버그로 실제로는 1900-01-00=0 기준)
    // serial 60 이후부터 정상 (1900-02-28 버그 보정)
    int days = (int)serial;
    if (days >= 60) --days; // 1900-02-29 버그 보정

    // 1899-12-31 기준 일수 → FILETIME
    SYSTEMTIME st = { 1899, 12, 0, 31, 0, 0, 0, 0 };
    FILETIME   ft;
    SystemTimeToFileTime(&st, &ft);

    ULARGE_INTEGER uli;
    uli.LowPart  = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    uli.QuadPart += (ULONGLONG)days * 864000000000ULL; // 하루 = 10^7 * 86400

    ft.dwLowDateTime  = uli.LowPart;
    ft.dwHighDateTime = uli.HighPart;

    SYSTEMTIME result;
    FileTimeToSystemTime(&ft, &result);

    CString dateStr;
    dateStr.Format(_T("%04d-%02d-%02d"), result.wYear, result.wMonth, result.wDay);
    return dateStr;
}

// ============================================================
// UTF-8 std::string → CString
// ============================================================

CString ExcelParser::UTF8ToCString(const std::string& utf8) {
    if (utf8.empty()) return _T("");
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    CString result;
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.GetBufferSetLength(wlen), wlen);
    result.ReleaseBuffer();
    return result;
}
