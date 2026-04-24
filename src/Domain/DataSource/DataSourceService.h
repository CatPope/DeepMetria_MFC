#pragma once

// 파일 로드 및 파싱 조율 서비스
// Architecture §3, §4.1 / DetailedSpec §5 참조

#include "../../Common/Types.h"

// 전방 선언 (Infrastructure 레이어 — .cpp에서 include)
class CSVParser;
class ExcelParser;

// ============================================================
// DataSourceService — 파일 확장자 판별 후 파서 위임
// ============================================================
class DataSourceService {
public:
    DataSourceService();
    ~DataSourceService();

    // 파일 경로의 파일을 읽어 DataTable로 반환
    // 확장자 판별 → CSVParser / ExcelParser 호출
    BOOL LoadFile(const CString& filePath,
                  DataTable&     outData,
                  AppError&      outError);

    // 지원 확장자 필터 문자열 반환
    // 반환값: "*.csv;*.xlsx;*.json"
    static CString GetSupportedExtensions();

    // 파일 유효성 검사 (존재 여부, 크기 제한 100MB)
    BOOL ValidateFile(const CString& filePath, AppError& outError);

private:
    // 확장자 추출 (소문자)
    static CString GetExtension(const CString& filePath);

    // 파일 크기 조회 (바이트)
    static LONGLONG GetFileSize(const CString& filePath);

    static const LONGLONG MAX_FILE_SIZE = 100LL * 1024 * 1024; // 100MB
};
