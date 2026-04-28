#pragma once
#include <string>

// StringUtils — CString/std::string 변환 및 JSON 이스케이프 유틸리티
// 중복 변환 로직을 한 곳에서 관리한다.

namespace StringUtils {

    // CString (유니코드) → std::string (UTF-8)
    inline std::string ToUTF8(const CString& str) {
        if (str.IsEmpty()) return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, str, str.GetLength(), NULL, 0, NULL, NULL);
        std::string result(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, str, str.GetLength(), &result[0], len, NULL, NULL);
        return result;
    }

    // std::string (UTF-8) → CString (유니코드)
    inline CString FromUTF8(const std::string& str) {
        if (str.empty()) return _T("");
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
        CString result;
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), result.GetBuffer(len), len);
        result.ReleaseBuffer(len);
        return result;
    }

    // JSON 문자열 이스케이프 (특수문자 처리)
    inline std::string JsonEscape(const std::string& str) {
        std::string result;
        result.reserve(str.size() + 16);
        for (char c : str) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n";  break;
                case '\r': result += "\\r";  break;
                case '\t': result += "\\t";  break;
                default:   result += c;      break;
            }
        }
        return result;
    }

} // namespace StringUtils
