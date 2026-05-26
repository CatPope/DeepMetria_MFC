#pragma once
// JsonParser.h — JSON 파일 파서
// 객체 배열 형식 [{ ... }, { ... }] 파싱, 외부 라이브러리 미사용
// UTF-8 / CP949 인코딩 처리, 유니코드 빌드 대응

#include "../../Common/Types.h"

// ============================================================
// JsonParser — 정적 유틸리티 클래스
// ============================================================
class JsonParser {
public:
    // JSON 파일 파싱
    // filePath : 전체 파일 경로
    // outError : 실패 시 오류 정보
    // 반환     : 파싱된 DataTable (실패 시 빈 DataTable + outError 설정)
    static DataTable Parse(const CString& filePath, AppError& outError);

private:
    // 인코딩 감지 및 UTF-8 변환 (CSVParser와 동일 로직)
    static std::string ConvertToUTF8(const std::string& raw);

    // CP949 → UTF-8 변환 (Windows API 사용)
    static std::string CP949ToUTF8(const std::string& cp949);

    // UTF-8 BOM 제거
    static std::string StripBOM(const std::string& s);

    // JSON 문자열에서 공백(탭, 개행 등) 건너뜀
    static size_t SkipWhitespace(const std::string& s, size_t pos);

    // 최상위 배열에서 객체 문자열 목록 추출 ( [ {...}, {...} ] )
    // 중괄호 깊이를 추적하여 각 객체를 통째로 반환
    static std::vector<std::string> ExtractObjects(const std::string& json, AppError& outError);

    // 단일 객체 { "key": value, ... } 파싱 → key/value 쌍 벡터 반환
    // 순서 보장을 위해 map 대신 vector<pair> 사용
    static bool ParseObject(const std::string& obj,
                            std::vector<std::pair<std::string, std::string>>& outPairs,
                            AppError& outError);

    // JSON 문자열 값 파싱 ("..." 구간, 이스케이프 처리)
    // pos: 여는 '"' 다음 위치에서 시작, 반환 시 닫는 '"' 다음 위치
    static std::string ParseStringValue(const std::string& s, size_t& pos, AppError& outError);

    // 숫자/bool/null 리터럴 파싱
    // pos: 리터럴 첫 글자 위치, 반환 시 리터럴 끝 다음 위치
    static std::string ParseLiteralValue(const std::string& s, size_t& pos);

    // 중첩 값 (객체/배열) 전체를 원본 문자열로 읽어 건너뜀
    // pos: '{' 또는 '[' 위치, 반환 시 닫는 괄호 다음 위치
    static std::string ParseNestedValue(const std::string& s, size_t& pos, AppError& outError);

    // CString 변환 헬퍼 (UTF-8 std::string → CString)
    static CString UTF8ToCString(const std::string& utf8);
};
