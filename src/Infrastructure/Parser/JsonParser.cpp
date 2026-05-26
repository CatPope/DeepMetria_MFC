#include "stdafx.h"
#include "JsonParser.h"
#include <fstream>
#include <sstream>

// ============================================================
// Parse — 진입점
// ============================================================

DataTable JsonParser::Parse(const CString& filePath, AppError& outError) {
    outError.Clear();
    DataTable result;

    // 파일 열기
    std::wstring wPath(filePath);
    std::ifstream fin(wPath, std::ios::binary);
    if (!fin.is_open()) {
        outError.Set(_T("FILE_OPEN_FAILED"), _T("JSON 파일을 열 수 없습니다."));
        return result;
    }

    // 전체 파일 읽기
    std::string raw((std::istreambuf_iterator<char>(fin)),
                     std::istreambuf_iterator<char>());
    fin.close();

    if (raw.empty()) {
        outError.Set(_T("EMPTY_FILE"), _T("파일에 데이터가 없습니다."));
        return result;
    }

    // BOM 제거 후 UTF-8 변환
    std::string content = StripBOM(ConvertToUTF8(raw));

    // 앞뒤 공백 제거 후 최소 유효성 검사
    size_t start = SkipWhitespace(content, 0);
    if (start >= content.size() || content[start] != '[') {
        outError.Set(_T("PARSE_ERROR"), _T("JSON 파일은 배열([...])로 시작해야 합니다."));
        return result;
    }

    // 배열에서 객체 추출
    std::vector<std::string> objects = ExtractObjects(content, outError);
    if (outError.IsError()) return result;

    // 빈 배열
    if (objects.empty()) {
        outError.Set(_T("NO_DATA_ROWS"), _T("데이터 행이 없습니다 (빈 배열)."), 1); // warning
        result.rowCount = 0;
        return result;
    }

    // 첫 번째 객체로 헤더 결정
    std::vector<std::pair<std::string, std::string>> firstPairs;
    if (!ParseObject(objects[0], firstPairs, outError)) return result;

    for (auto& p : firstPairs) {
        result.headers.push_back(UTF8ToCString(p.first));
    }
    result.colCount = (int)result.headers.size();

    // 각 객체를 행으로 변환
    for (size_t i = 0; i < objects.size(); ++i) {
        std::vector<std::pair<std::string, std::string>> pairs;
        AppError rowErr;
        if (!ParseObject(objects[i], pairs, rowErr)) {
            outError = rowErr;
            return result;
        }

        // 헤더 순서에 맞춰 값 배치
        DataRow row;
        row.resize(result.colCount, _T(""));
        for (auto& p : pairs) {
            CString key = UTF8ToCString(p.first);
            for (int c = 0; c < result.colCount; ++c) {
                if (result.headers[c] == key) {
                    row[c] = UTF8ToCString(p.second);
                    break;
                }
            }
        }
        result.rows.push_back(row);
    }

    result.rowCount = (int)result.rows.size();
    return result;
}

// ============================================================
// 인코딩 감지 및 UTF-8 변환 (CSVParser와 동일)
// ============================================================

std::string JsonParser::ConvertToUTF8(const std::string& raw) {
    if (raw.size() >= 3 &&
        (unsigned char)raw[0] == 0xEF &&
        (unsigned char)raw[1] == 0xBB &&
        (unsigned char)raw[2] == 0xBF) {
        return raw; // 이미 UTF-8 BOM
    }

    bool validUTF8 = true;
    for (size_t i = 0; i < raw.size(); ) {
        unsigned char c = (unsigned char)raw[i];
        int extra = 0;
        if (c < 0x80)      { extra = 0; }
        else if (c < 0xC0) { validUTF8 = false; break; }
        else if (c < 0xE0) { extra = 1; }
        else if (c < 0xF0) { extra = 2; }
        else               { extra = 3; }
        for (int j = 1; j <= extra; ++j) {
            if (i + j >= raw.size() || ((unsigned char)raw[i+j] & 0xC0) != 0x80) {
                validUTF8 = false; break;
            }
        }
        if (!validUTF8) break;
        i += extra + 1;
    }

    if (validUTF8) return raw;
    return CP949ToUTF8(raw);
}

std::string JsonParser::CP949ToUTF8(const std::string& cp949) {
    int wlen = MultiByteToWideChar(949, 0, cp949.c_str(), (int)cp949.size(), nullptr, 0);
    if (wlen <= 0) return cp949;

    std::wstring wstr(wlen, L'\0');
    MultiByteToWideChar(949, 0, cp949.c_str(), (int)cp949.size(), &wstr[0], wlen);

    int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, nullptr, 0, nullptr, nullptr);
    if (u8len <= 0) return cp949;

    std::string utf8(u8len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, &utf8[0], u8len, nullptr, nullptr);
    return utf8;
}

std::string JsonParser::StripBOM(const std::string& s) {
    if (s.size() >= 3 &&
        (unsigned char)s[0] == 0xEF &&
        (unsigned char)s[1] == 0xBB &&
        (unsigned char)s[2] == 0xBF) {
        return s.substr(3);
    }
    return s;
}

// ============================================================
// 공백 건너뜀
// ============================================================

size_t JsonParser::SkipWhitespace(const std::string& s, size_t pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' ||
                               s[pos] == '\n' || s[pos] == '\r')) {
        ++pos;
    }
    return pos;
}

// ============================================================
// 배열에서 최상위 객체 추출
// ============================================================

std::vector<std::string> JsonParser::ExtractObjects(const std::string& json, AppError& outError) {
    std::vector<std::string> objects;

    size_t pos = SkipWhitespace(json, 0);
    if (pos >= json.size() || json[pos] != '[') {
        outError.Set(_T("PARSE_ERROR"), _T("JSON 배열을 찾을 수 없습니다."));
        return objects;
    }
    ++pos; // '[' 건너뜀

    pos = SkipWhitespace(json, pos);
    if (pos < json.size() && json[pos] == ']') {
        return objects; // 빈 배열
    }

    while (pos < json.size()) {
        pos = SkipWhitespace(json, pos);
        if (pos >= json.size()) break;

        if (json[pos] == ']') break; // 배열 끝

        if (json[pos] != '{') {
            outError.Set(_T("PARSE_ERROR"), _T("배열 원소가 객체({})가 아닙니다."));
            return objects;
        }

        // 중괄호 깊이 추적으로 객체 전체 추출
        size_t objStart = pos;
        int depth = 0;
        bool inStr = false;
        bool escape = false;

        for (; pos < json.size(); ++pos) {
            char c = json[pos];
            if (escape) { escape = false; continue; }
            if (c == '\\' && inStr) { escape = true; continue; }
            if (c == '"') { inStr = !inStr; continue; }
            if (inStr) continue;
            if (c == '{') ++depth;
            else if (c == '}') {
                --depth;
                if (depth == 0) { ++pos; break; }
            }
        }

        if (depth != 0) {
            outError.Set(_T("PARSE_ERROR"), _T("JSON 객체가 닫히지 않았습니다."));
            return objects;
        }

        objects.push_back(json.substr(objStart, pos - objStart));

        // 객체 뒤 ',' 또는 ']' 처리
        pos = SkipWhitespace(json, pos);
        if (pos < json.size()) {
            if (json[pos] == ',') ++pos;
            else if (json[pos] == ']') break;
        }
    }

    return objects;
}

// ============================================================
// 단일 객체 파싱 → key/value 쌍 벡터
// ============================================================

bool JsonParser::ParseObject(const std::string& obj,
                              std::vector<std::pair<std::string, std::string>>& outPairs,
                              AppError& outError) {
    size_t pos = SkipWhitespace(obj, 0);
    if (pos >= obj.size() || obj[pos] != '{') {
        outError.Set(_T("PARSE_ERROR"), _T("객체가 '{'로 시작하지 않습니다."));
        return false;
    }
    ++pos; // '{' 건너뜀

    pos = SkipWhitespace(obj, pos);
    if (pos < obj.size() && obj[pos] == '}') return true; // 빈 객체

    while (pos < obj.size()) {
        pos = SkipWhitespace(obj, pos);
        if (pos >= obj.size()) break;
        if (obj[pos] == '}') break;

        // 키: 반드시 문자열
        if (obj[pos] != '"') {
            outError.Set(_T("PARSE_ERROR"), _T("JSON 키는 문자열이어야 합니다."));
            return false;
        }
        ++pos; // 여는 '"' 건너뜀
        std::string key = ParseStringValue(obj, pos, outError);
        if (outError.IsError()) return false;

        // ':' 찾기
        pos = SkipWhitespace(obj, pos);
        if (pos >= obj.size() || obj[pos] != ':') {
            outError.Set(_T("PARSE_ERROR"), _T("JSON 키-값 구분자 ':'가 없습니다."));
            return false;
        }
        ++pos; // ':' 건너뜀
        pos = SkipWhitespace(obj, pos);

        // 값 파싱
        std::string value;
        if (pos >= obj.size()) {
            outError.Set(_T("PARSE_ERROR"), _T("JSON 값이 없습니다."));
            return false;
        }

        char ch = obj[pos];
        if (ch == '"') {
            ++pos; // 여는 '"' 건너뜀
            value = ParseStringValue(obj, pos, outError);
            if (outError.IsError()) return false;
        } else if (ch == '{' || ch == '[') {
            value = ParseNestedValue(obj, pos, outError);
            if (outError.IsError()) return false;
        } else {
            value = ParseLiteralValue(obj, pos);
        }

        outPairs.push_back({ key, value });

        // ',' 또는 '}' 처리
        pos = SkipWhitespace(obj, pos);
        if (pos >= obj.size()) break;
        if (obj[pos] == ',') { ++pos; continue; }
        if (obj[pos] == '}') break;
    }

    return true;
}

// ============================================================
// JSON 문자열 값 파싱 (여는 '"' 다음 위치부터 시작)
// ============================================================

std::string JsonParser::ParseStringValue(const std::string& s, size_t& pos, AppError& outError) {
    std::string result;
    while (pos < s.size()) {
        char c = s[pos];
        if (c == '"') {
            ++pos; // 닫는 '"' 건너뜀
            return result;
        }
        if (c == '\\') {
            ++pos;
            if (pos >= s.size()) break;
            char esc = s[pos];
            switch (esc) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'u': {
                    // \uXXXX — 4자리 16진수를 UTF-8로 변환
                    if (pos + 4 >= s.size()) { result += '?'; break; }
                    std::string hex = s.substr(pos + 1, 4);
                    pos += 4;
                    unsigned int cp = (unsigned int)strtol(hex.c_str(), nullptr, 16);
                    if (cp < 0x80) {
                        result += (char)cp;
                    } else if (cp < 0x800) {
                        result += (char)(0xC0 | (cp >> 6));
                        result += (char)(0x80 | (cp & 0x3F));
                    } else {
                        result += (char)(0xE0 | (cp >> 12));
                        result += (char)(0x80 | ((cp >> 6) & 0x3F));
                        result += (char)(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default: result += esc; break;
            }
        } else {
            result += c;
        }
        ++pos;
    }
    outError.Set(_T("PARSE_ERROR"), _T("JSON 문자열이 닫히지 않았습니다."));
    return result;
}

// ============================================================
// 숫자/bool/null 리터럴 파싱
// ============================================================

std::string JsonParser::ParseLiteralValue(const std::string& s, size_t& pos) {
    size_t start = pos;
    // 리터럴 끝: 공백, ',', '}', ']' 중 하나
    while (pos < s.size()) {
        char c = s[pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
            c == ',' || c == '}' || c == ']') break;
        ++pos;
    }
    std::string lit = s.substr(start, pos - start);

    if (lit == "true")  return "TRUE";
    if (lit == "false") return "FALSE";
    if (lit == "null")  return "";
    return lit; // 숫자는 그대로
}

// ============================================================
// 중첩 객체/배열 통째로 읽기 (문자열로 직렬화)
// ============================================================

std::string JsonParser::ParseNestedValue(const std::string& s, size_t& pos, AppError& outError) {
    char open  = s[pos];
    char close = (open == '{') ? '}' : ']';
    int depth  = 0;
    size_t start = pos;
    bool inStr = false;
    bool escape = false;

    for (; pos < s.size(); ++pos) {
        char c = s[pos];
        if (escape) { escape = false; continue; }
        if (c == '\\' && inStr) { escape = true; continue; }
        if (c == '"') { inStr = !inStr; continue; }
        if (inStr) continue;
        if (c == open)  { ++depth; }
        else if (c == close) {
            --depth;
            if (depth == 0) { ++pos; break; }
        }
    }

    if (depth != 0) {
        outError.Set(_T("PARSE_ERROR"), _T("중첩 JSON 구조가 닫히지 않았습니다."));
        return "";
    }

    return s.substr(start, pos - start);
}

// ============================================================
// UTF-8 std::string → CString
// ============================================================

CString JsonParser::UTF8ToCString(const std::string& utf8) {
    if (utf8.empty()) return _T("");
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    CString result;
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.GetBufferSetLength(wlen), wlen);
    result.ReleaseBuffer();
    return result;
}
