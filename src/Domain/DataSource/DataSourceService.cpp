#include "stdafx.h"
#include "DataSourceService.h"

// Infrastructure 레이어 — .cpp에서만 include
#include "../../Infrastructure/Parser/CSVParser.h"
#include "../../Infrastructure/Parser/ExcelParser.h"
#include "../../Infrastructure/Parser/JsonParser.h"

// ============================================================
// DataSourceService 구현
// Architecture §4.1 / DetailedSpec §5 참조
// ============================================================

DataSourceService::DataSourceService()
{
}

DataSourceService::~DataSourceService()
{
}

BOOL DataSourceService::LoadFile(const CString& filePath,
                                  DataTable&     outData,
                                  AppError&      outError)
{
    // 1. 파일 유효성 검사
    if (!ValidateFile(filePath, outError))
        return FALSE;

    // 2. 확장자 판별 후 파서 위임
    CString ext = GetExtension(filePath);

    if (ext == _T("csv")) {
        outData = CSVParser::Parse(filePath, outError);
        return outError.code.IsEmpty();
    }
    else if (ext == _T("xlsx") || ext == _T("xls")) {
        outData = ExcelParser::Parse(filePath, outError);
        return outError.code.IsEmpty();
    }
    else if (ext == _T("json")) {
        outData = JsonParser::Parse(filePath, outError);
        return outError.code.IsEmpty();
    }
    else {
        outError = AppError(
            _T("UNSUPPORTED_FILE_TYPE"),
            _T("지원하지 않는 파일 형식입니다. CSV 또는 Excel 파일을 선택하세요."),
            2
        );
        return FALSE;
    }
}

CString DataSourceService::GetSupportedExtensions()
{
    return _T("*.csv;*.xlsx;*.json");
}

BOOL DataSourceService::ValidateFile(const CString& filePath, AppError& outError)
{
    // 1. 파일 존재 여부 확인
    if (GetFileAttributes(filePath) == INVALID_FILE_ATTRIBUTES) {
        outError = AppError(
            _T("FILE_NOT_FOUND"),
            _T("파일을 찾을 수 없습니다: ") + filePath,
            2
        );
        return FALSE;
    }

    // 2. 파일 크기 확인 (100MB 제한)
    LONGLONG fileSize = GetFileSize(filePath);
    if (fileSize < 0) {
        outError = AppError(
            _T("FILE_ACCESS_ERROR"),
            _T("파일 크기를 확인할 수 없습니다."),
            2
        );
        return FALSE;
    }

    if (fileSize > MAX_FILE_SIZE) {
        CString msg;
        msg.Format(_T("파일 크기가 제한(100MB)을 초과합니다. 현재 크기: %lldMB"),
                   fileSize / (1024 * 1024));
        outError = AppError(_T("FILE_TOO_LARGE"), msg, 2);
        return FALSE;
    }

    // 3. 빈 파일 확인
    if (fileSize == 0) {
        outError = AppError(
            _T("EMPTY_FILE"),
            _T("파일에 데이터가 없습니다."),
            2
        );
        return FALSE;
    }

    // 4. 확장자 확인
    CString ext = GetExtension(filePath);
    if (ext != _T("csv") && ext != _T("xlsx") && ext != _T("xls") && ext != _T("json")) {
        outError = AppError(
            _T("UNSUPPORTED_FILE_TYPE"),
            _T("지원하지 않는 파일 형식입니다."),
            2
        );
        return FALSE;
    }

    return TRUE;
}

CString DataSourceService::GetExtension(const CString& filePath)
{
    int dot = filePath.ReverseFind(_T('.'));
    if (dot < 0) return _T("");
    CString ext = filePath.Mid(dot + 1);
    ext.MakeLower();
    return ext;
}

LONGLONG DataSourceService::GetFileSize(const CString& filePath)
{
    WIN32_FILE_ATTRIBUTE_DATA fad = {};
    if (!GetFileAttributesEx(filePath, GetFileExInfoStandard, &fad))
        return -1LL;

    LARGE_INTEGER li;
    li.HighPart = fad.nFileSizeHigh;
    li.LowPart  = fad.nFileSizeLow;
    return li.QuadPart;
}

// ============================================================
// GetSchema — DataTable에서 ColumnSchema 벡터 추출
// ============================================================

std::vector<ColumnSchema> DataSourceService::GetSchema(const DataTable& data)
{
    std::vector<ColumnSchema> schema;
    schema.reserve(data.columns.size());

    for (int i = 0; i < (int)data.columns.size(); ++i) {
        const DataColumn& col = data.columns[i];

        ColumnSchema cs;
        cs.name  = col.name;
        cs.type  = col.type.IsEmpty() ? _T("text") : col.type;
        cs.index = i;

        // null(빈 문자열) 개수 집계 및 샘플 값 수집
        int nullCount = 0;
        int sampleCount = 0;
        CString samples;

        for (const CString& val : col.values) {
            if (val.IsEmpty()) {
                ++nullCount;
            } else if (sampleCount < 3) {
                if (sampleCount > 0)
                    samples += _T(",");
                samples += val;
                ++sampleCount;
            }
        }

        cs.nullCount    = nullCount;
        cs.sampleValues = samples;

        schema.push_back(cs);
    }

    return schema;
}
