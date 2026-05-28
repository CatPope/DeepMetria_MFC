#pragma once

// ColumnTypeInference.h — 컬럼 타입 추론 헬퍼 (헤더 전용 / inline)
// DeepMetriaDoc.cpp의 LoadFile / Serialize 역직렬화 두 곳에서 중복되던
// 타입 추론 로직을 단일 구현으로 통합한다. 동작은 기존과 바이트 단위로 동일하다.
//
// 규칙:
//   date    : YYYY-MM-DD / YYYY/MM/DD / YYYY.MM.DD (길이>=10, idx4==idx7 구분자,
//             0-3,5,6,8,9 위치가 모두 숫자)
//   numeric : _tcstod가 문자열 전체를 소비
//   text    : 그 외
//   값이 모두 비어 있는(hasNonEmpty == false) 컬럼 => "text"
//
// CString / TCHAR / _T / _tcstod / _istdigit 는 stdafx.h(afxwin.h, tchar.h)에서
// 이미 로드된 상태로 이 헤더를 include하는 .cpp에서 사용 가능하다.

#include <vector>

// ------------------------------------------------------------
// 단일 값이 날짜 패턴인지 검사 (LoadFile 원본 로직과 동일)
// ------------------------------------------------------------
inline bool IsDateValue(const CString& v)
{
    if (v.GetLength() >= 10)
    {
        TCHAR sep = v[4];
        if ((sep == _T('-') || sep == _T('/') || sep == _T('.')) &&
            v[7] == sep &&
            _istdigit(v[0]) && _istdigit(v[1]) &&
            _istdigit(v[2]) && _istdigit(v[3]) &&
            _istdigit(v[5]) && _istdigit(v[6]) &&
            _istdigit(v[8]) && _istdigit(v[9]))
        {
            return true;
        }
    }
    return false;
}

// ------------------------------------------------------------
// 단일 값이 숫자(numeric)인지 검사 (LoadFile 원본 로직과 동일)
// _tcstod가 문자열 시작부터 끝까지 전부 소비해야 numeric으로 본다.
// ------------------------------------------------------------
inline bool IsNumericValue(const CString& v)
{
    TCHAR* endPtr = nullptr;
    _tcstod(v, &endPtr);
    // endPtr이 시작 위치 그대로이거나 끝까지 가지 못하면 숫자가 아님
    return !(endPtr == (LPCTSTR)v || *endPtr != _T('\0'));
}

// ------------------------------------------------------------
// 컬럼 값 목록으로부터 타입("date"/"numeric"/"text")을 추론
// 기존 LoadFile / Serialize 두 블록의 allDate / allNumeric / hasNonEmpty
// 처리 의미를 그대로 보존한다.
// ------------------------------------------------------------
inline CString InferColumnType(const std::vector<CString>& values)
{
    bool allNumeric  = true;
    bool allDate     = true;
    bool hasNonEmpty = false;

    for (const auto& v : values)
    {
        if (v.IsEmpty()) continue;
        hasNonEmpty = true;

        if (allDate)
        {
            if (!IsDateValue(v)) allDate = false;
        }

        if (allNumeric)
        {
            if (!IsNumericValue(v)) allNumeric = false;
        }
    }

    if (!hasNonEmpty)     return _T("text");
    else if (allDate)     return _T("date");
    else if (allNumeric)  return _T("numeric");
    else                  return _T("text");
}
