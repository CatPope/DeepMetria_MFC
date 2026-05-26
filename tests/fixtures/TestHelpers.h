#pragma once
// TestHelpers.h — 테스트 공통 유틸리티
//
// 제공 기능:
//   1. CString 비교 매크로 (EXPECT_CSTRING_EQ, ASSERT_CSTRING_EQ)
//   2. JSON 필드 추출 헬퍼
//   3. 임시 파일 RAII 래퍼
//   4. DataTable 플루언트 빌더

#include <gtest/gtest.h>
#include "Common/Types.h"
#include <string>
#include <fstream>
#include <sstream>
#include <windows.h>

// ============================================================
// 1. CString 비교 매크로
//
// Google Test 는 CString 을 직접 비교할 수 없다.
// 아래 매크로는 CString 을 std::wstring 으로 변환하여 비교한다.
// ============================================================

// CString -> std::wstring 변환 헬퍼 (내부 사용)
inline std::wstring CStringToWString(const CString& cs) {
    return std::wstring(static_cast<LPCTSTR>(cs));
}

// CString 내용이 같은지 검증 (EXPECT 방식 — 실패 시 계속 진행)
#define EXPECT_CSTRING_EQ(expected, actual) \
    EXPECT_EQ(CStringToWString(expected), CStringToWString(actual)) \
        << "CString mismatch: expected=[" \
        << CW2A(static_cast<LPCTSTR>(expected)) \
        << "] actual=[" \
        << CW2A(static_cast<LPCTSTR>(actual)) << "]"

// CString 내용이 같은지 검증 (ASSERT 방식 — 실패 시 테스트 중단)
#define ASSERT_CSTRING_EQ(expected, actual) \
    ASSERT_EQ(CStringToWString(expected), CStringToWString(actual)) \
        << "CString mismatch: expected=[" \
        << CW2A(static_cast<LPCTSTR>(expected)) \
        << "] actual=[" \
        << CW2A(static_cast<LPCTSTR>(actual)) << "]"

// CString 이 비어 있지 않은지 검증
#define EXPECT_CSTRING_NONEMPTY(cs) \
    EXPECT_FALSE((cs).IsEmpty()) << "Expected non-empty CString"

// CString 이 비어 있는지 검증
#define EXPECT_CSTRING_EMPTY(cs) \
    EXPECT_TRUE((cs).IsEmpty()) << "Expected empty CString, got: " \
        << CW2A(static_cast<LPCTSTR>(cs))

// ============================================================
// 2. JSON 파싱 헬퍼
//
// 정규식이나 외부 라이브러리 없이 단순 문자열 탐색으로
// JSON 문자열 필드를 추출한다. 테스트 목적의 단순 구현이다.
// ============================================================
namespace TestJson {

    // JSON 문자열에서 "key": "value" 형태의 값을 추출한다.
    // 반환: 추출된 값(CString). 키가 없으면 빈 문자열 반환.
    inline CString ExtractStringField(const CString& json, const CString& key) {
        // 검색 패턴: "key":"
        CString searchKey = _T("\"") + key + _T("\":");
        int keyPos = json.Find(searchKey);
        if (keyPos == -1) return _T("");

        int valueStart = keyPos + searchKey.GetLength();

        // 공백 건너뜀
        while (valueStart < json.GetLength() && json[valueStart] == _T(' ')) {
            ++valueStart;
        }
        if (valueStart >= json.GetLength()) return _T("");

        // 문자열 값 ("로 시작)
        if (json[valueStart] == _T('"')) {
            ++valueStart;
            int valueEnd = json.Find(_T('"'), valueStart);
            if (valueEnd == -1) return _T("");
            return json.Mid(valueStart, valueEnd - valueStart);
        }

        // 숫자/불리언 값 (,나 }까지)
        int valueEnd = valueStart;
        while (valueEnd < json.GetLength() &&
               json[valueEnd] != _T(',') &&
               json[valueEnd] != _T('}') &&
               json[valueEnd] != _T(']')) {
            ++valueEnd;
        }
        return json.Mid(valueStart, valueEnd - valueStart).Trim();
    }

    // JSON 문자열에서 "key": number 형태의 정수 값을 추출한다.
    // 반환: 추출된 정수. 키가 없거나 변환 실패 시 defaultValue 반환.
    inline int ExtractIntField(const CString& json,
                               const CString& key,
                               int defaultValue = 0) {
        CString val = ExtractStringField(json, key);
        if (val.IsEmpty()) return defaultValue;
        return _ttoi(val);
    }

    // JSON 문자열이 특정 키를 포함하는지 확인한다.
    inline bool HasField(const CString& json, const CString& key) {
        CString searchKey = _T("\"") + key + _T("\":");
        return json.Find(searchKey) != -1;
    }

} // namespace TestJson

// ============================================================
// 3. 임시 파일 RAII 래퍼
//
// 생성 시 임시 파일을 만들고, 소멸 시 자동으로 삭제한다.
// 테스트에서 파일 입출력 로직을 검증할 때 사용한다.
// ============================================================
class TempFile {
public:
    // 임시 파일 생성. content 가 주어지면 파일에 기록한다.
    explicit TempFile(const std::string& content = "") {
        // Windows 임시 디렉터리에 파일 생성
        char tempDir[MAX_PATH] = {};
        char tempPath[MAX_PATH] = {};
        ::GetTempPathA(MAX_PATH, tempDir);
        ::GetTempFileNameA(tempDir, "dmt", 0, tempPath);

        m_path = tempPath;

        if (!content.empty()) {
            std::ofstream ofs(m_path, std::ios::binary);
            ofs << content;
        }
    }

    // 유니코드 경로로 임시 파일 생성
    explicit TempFile(const std::wstring& content) {
        wchar_t tempDir[MAX_PATH] = {};
        wchar_t tempPathW[MAX_PATH] = {};
        ::GetTempPathW(MAX_PATH, tempDir);
        ::GetTempFileNameW(tempDir, L"dmt", 0, tempPathW);

        char tempPathA[MAX_PATH] = {};
        ::WideCharToMultiByte(CP_ACP, 0, tempPathW, -1, tempPathA, MAX_PATH, nullptr, nullptr);
        m_path = tempPathA;

        std::wofstream ofs(tempPathW, std::ios::binary);
        ofs << content;
    }

    ~TempFile() {
        if (!m_path.empty()) {
            ::DeleteFileA(m_path.c_str());
        }
    }

    // 파일 경로 반환 (std::string)
    const std::string& Path() const { return m_path; }

    // 파일 경로를 CString 으로 반환
    CString PathAsCString() const {
        return CString(m_path.c_str());
    }

    // 파일 존재 여부 확인
    bool Exists() const {
        return ::GetFileAttributesA(m_path.c_str()) != INVALID_FILE_ATTRIBUTES;
    }

    // 파일 내용 읽기
    std::string ReadContent() const {
        std::ifstream ifs(m_path, std::ios::binary);
        std::ostringstream oss;
        oss << ifs.rdbuf();
        return oss.str();
    }

    // 복사/이동 금지 (단일 소유권)
    TempFile(const TempFile&)            = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile(TempFile&&)                 = delete;
    TempFile& operator=(TempFile&&)      = delete;

private:
    std::string m_path;
};

// ============================================================
// 4. DataTable 플루언트 빌더
//
// 테스트에서 다양한 DataTable 을 간결하게 구성하기 위한
// 빌더 패턴 구현이다.
//
// 사용 예:
//   DataTable t = DataTableBuilder()
//       .WithFileName(_T("test.csv"))
//       .AddColumn(_T("이름"), _T("text"),  {_T("김철수"), _T("이영희")})
//       .AddColumn(_T("나이"), _T("numeric"), {_T("30"), _T("25")})
//       .Build();
// ============================================================
class DataTableBuilder {
public:
    DataTableBuilder() {
        m_table.rowCount = 0;
        m_table.colCount = 0;
    }

    // 파일 이름 설정
    DataTableBuilder& WithFileName(const CString& fileName) {
        m_table.fileName = fileName;
        return *this;
    }

    // 컬럼 추가 (헤더, 컬럼 배열, 행 배열을 동기화하여 추가)
    DataTableBuilder& AddColumn(const CString&              name,
                                const CString&              type,
                                const std::vector<CString>& values) {
        DataColumn col;
        col.name   = name;
        col.type   = type;
        col.values = values;
        m_table.columns.push_back(col);
        m_table.headers.push_back(name);
        m_table.colCount++;

        // 행 수 갱신 (첫 컬럼 기준)
        if (m_table.rowCount == 0) {
            m_table.rowCount = static_cast<int>(values.size());
            // 행 기반 rows 초기화
            m_table.rows.resize(m_table.rowCount);
        }

        // rows 에 이 컬럼 값을 추가
        int colIdx = m_table.colCount - 1;
        for (int r = 0; r < m_table.rowCount; ++r) {
            if (r < static_cast<int>(values.size())) {
                if (static_cast<int>(m_table.rows[r].size()) <= colIdx) {
                    m_table.rows[r].push_back(values[r]);
                } else {
                    m_table.rows[r][colIdx] = values[r];
                }
            }
        }

        return *this;
    }

    // 빈 DataTable 반환 (rowCount=0, colCount=0)
    static DataTable Empty() {
        return DataTable{};
    }

    // 단일 컬럼 numeric DataTable 빠른 생성
    static DataTable SingleNumericColumn(const CString&              colName,
                                         const std::vector<CString>& values) {
        return DataTableBuilder()
            .WithFileName(_T("test.csv"))
            .AddColumn(colName, _T("numeric"), values)
            .Build();
    }

    // 완성된 DataTable 반환
    DataTable Build() { return m_table; }

private:
    DataTable m_table;
};
