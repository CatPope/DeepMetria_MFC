#pragma once
// CSVParser.h — CSV 파일 파서
// 첫 행 헤더, 쉼표/탭 구분자 자동 감지, UTF-8/CP949 인코딩 처리
// 대용량 파일 스트리밍 파싱 지원 (100만 행 이상)

#include "../../Common/Types.h"

// ============================================================
// CSVParser — 정적 유틸리티 클래스
// ============================================================
class CSVParser {
public:
    // CSV 파일 파싱
    // filePath : 전체 파일 경로
    // outError : 실패 시 오류 정보
    // 반환     : 파싱된 DataTable (실패 시 빈 DataTable + outError 설정)
    static DataTable Parse(const CString& filePath, AppError& outError);

private:
    // 구분자 자동 감지 (첫 1KB 샘플 기반)
    // 반환: ',' 또는 '\t'
    static char DetectDelimiter(const std::string& sample);

    // 인코딩 감지 및 UTF-8 변환
    // 반환: UTF-8 변환된 문자열. 감지 실패 시 원본 반환
    static std::string ConvertToUTF8(const std::string& raw);

    // CP949 → UTF-8 변환 (Windows API 사용)
    static std::string CP949ToUTF8(const std::string& cp949);

    // UTF-8 BOM 제거
    static std::string StripBOM(const std::string& s);

    // 한 행을 구분자로 분리 (큰따옴표 내 구분자 무시)
    static std::vector<std::string> SplitRow(const std::string& line, char delimiter);

    // 셀 값에서 큰따옴표 제거 및 이스케이프 처리
    static std::string UnquoteCell(const std::string& cell);

    // CString 변환 헬퍼 (UTF-8 std::string → CString)
    static CString UTF8ToCString(const std::string& utf8);
};
