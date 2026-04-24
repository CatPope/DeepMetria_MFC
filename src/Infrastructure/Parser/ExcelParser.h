#pragma once
// ExcelParser.h — Excel (xlsx) 파일 파서
// 첫 시트만 파싱 (MVP), 숫자/문자열/날짜 셀 타입 처리
// OpenXLSX 또는 자체 ZIP+XML 구현 (vcpkg openxlsx 의존)

#include "../../Common/Types.h"

// ============================================================
// ExcelParser — 정적 유틸리티 클래스
// ============================================================
class ExcelParser {
public:
    // xlsx 파일 파싱
    // filePath : 전체 파일 경로 (.xlsx)
    // outError : 실패 시 오류 정보
    // 반환     : 파싱된 DataTable (실패 시 빈 DataTable + outError 설정)
    static DataTable Parse(const CString& filePath, AppError& outError);

private:
    // xlsx 파일 유효성 검사 (ZIP 구조, 필수 XML 존재 여부)
    static BOOL ValidateXlsx(const CString& filePath, AppError& outError);

    // 셀 값 문자열 추출 (공유 문자열 테이블 참조 처리)
    // rawValue  : XML에서 읽은 원시 값
    // cellType  : 'n'=숫자, 's'=공유문자열, 'd'=날짜, 'b'=불리언, ''=인라인문자열
    // sharedStr : 공유 문자열 테이블
    static CString ExtractCellValue(const std::string&              rawValue,
                                    char                            cellType,
                                    const std::vector<std::string>& sharedStr);

    // Excel 시리얼 날짜 → "YYYY-MM-DD" 문자열 변환
    static CString SerialDateToString(double serial);

    // UTF-8 std::string → CString 변환
    static CString UTF8ToCString(const std::string& utf8);
};
