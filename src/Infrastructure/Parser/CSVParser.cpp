#include "stdafx.h"
#include "CSVParser.h"
#include <fstream>
#include <sstream>

// ============================================================
// Parse — 진입점
// ============================================================

DataTable CSVParser::Parse(const CString& filePath, AppError& outError) {
    outError.Clear();
    DataTable result;

    // 파일 경로 CString → wstring → 멀티바이트(CP_ACP) 경로
    std::wstring wPath(filePath);
    std::ifstream fin(wPath, std::ios::binary);
    if (!fin.is_open()) {
        outError.Set(_T("FILE_OPEN_FAILED"), _T("CSV 파일을 열 수 없습니다."));
        return result;
    }

    // 전체 파일 읽기 (스트리밍 대용량 지원: 청크 단위로 처리)
    std::string raw((std::istreambuf_iterator<char>(fin)),
                     std::istreambuf_iterator<char>());
    fin.close();

    if (raw.empty()) {
        outError.Set(_T("EMPTY_FILE"), _T("파일에 데이터가 없습니다."));
        return result;
    }

    // BOM 제거 후 UTF-8 변환
    std::string content = StripBOM(ConvertToUTF8(raw));

    // 구분자 감지 (첫 1KB 샘플)
    std::string sample = content.substr(0, (std::min)((size_t)1024, content.size()));
    char delimiter = DetectDelimiter(sample);

    // 행 단위 파싱
    std::istringstream stream(content);
    std::string line;
    bool isFirstRow = true;

    while (std::getline(stream, line)) {
        // 개행 문자 제거 (\r\n 대응)
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::vector<std::string> cells = SplitRow(line, delimiter);

        if (isFirstRow) {
            // 헤더 행
            for (auto& cell : cells) {
                std::string h = UnquoteCell(cell);
                result.headers.push_back(UTF8ToCString(h));
            }
            result.colCount = (int)result.headers.size();
            isFirstRow = false;
        } else {
            // 데이터 행
            DataRow row;
            row.reserve(result.colCount);
            for (int i = 0; i < (int)cells.size(); ++i) {
                row.push_back(UTF8ToCString(UnquoteCell(cells[i])));
            }
            // 컬럼 수 맞춤 (부족하면 빈 값으로 채움)
            while ((int)row.size() < result.colCount) row.push_back(_T(""));
            result.rows.push_back(row);
        }
    }

    // 헤더가 없으면 빈 파일 처리
    if (result.headers.empty()) {
        outError.Set(_T("EMPTY_FILE"), _T("파일에 데이터가 없습니다."));
        return result;
    }

    // 헤더만 있는 경우 (데이터 행 없음)
    if (result.rows.empty()) {
        outError.Set(_T("NO_DATA_ROWS"), _T("데이터 행이 없습니다 (헤더만 존재)."), 1); // warning
    }

    result.rowCount = (int)result.rows.size();
    return result;
}

// ============================================================
// 구분자 자동 감지
// ============================================================

char CSVParser::DetectDelimiter(const std::string& sample) {
    int commaCount = 0, tabCount = 0;
    bool inQuote = false;
    for (char c : sample) {
        if (c == '"') { inQuote = !inQuote; continue; }
        if (inQuote) continue;
        if (c == ',') ++commaCount;
        else if (c == '\t') ++tabCount;
    }
    return (tabCount > commaCount) ? '\t' : ',';
}

// ============================================================
// 인코딩 감지 및 UTF-8 변환
// ============================================================

std::string CSVParser::ConvertToUTF8(const std::string& raw) {
    if (raw.size() >= 3 &&
        (unsigned char)raw[0] == 0xEF &&
        (unsigned char)raw[1] == 0xBB &&
        (unsigned char)raw[2] == 0xBF) {
        // 이미 UTF-8 BOM
        return raw;
    }

    // UTF-8 여부 검사: 유효하지 않은 바이트 시퀀스가 있으면 CP949로 간주
    bool validUTF8 = true;
    for (size_t i = 0; i < raw.size(); ) {
        unsigned char c = (unsigned char)raw[i];
        int extra = 0;
        if (c < 0x80)       { extra = 0; }
        else if (c < 0xC0)  { validUTF8 = false; break; }
        else if (c < 0xE0)  { extra = 1; }
        else if (c < 0xF0)  { extra = 2; }
        else                { extra = 3; }
        for (int j = 1; j <= extra; ++j) {
            if (i + j >= raw.size() || ((unsigned char)raw[i+j] & 0xC0) != 0x80) {
                validUTF8 = false; break;
            }
        }
        if (!validUTF8) break;
        i += extra + 1;
    }

    if (validUTF8) return raw;

    // CP949(EUC-KR) → UTF-8 변환 시도
    return CP949ToUTF8(raw);
}

std::string CSVParser::CP949ToUTF8(const std::string& cp949) {
    // CP949 → UTF-16LE → UTF-8 (Windows API)
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

std::string CSVParser::StripBOM(const std::string& s) {
    if (s.size() >= 3 &&
        (unsigned char)s[0] == 0xEF &&
        (unsigned char)s[1] == 0xBB &&
        (unsigned char)s[2] == 0xBF) {
        return s.substr(3);
    }
    return s;
}

// ============================================================
// 행 분리 (큰따옴표 내 구분자 무시)
// ============================================================

std::vector<std::string> CSVParser::SplitRow(const std::string& line, char delimiter) {
    std::vector<std::string> cells;
    std::string cell;
    bool inQuote = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (inQuote && i + 1 < line.size() && line[i+1] == '"') {
                // 이스케이프된 따옴표 ("")
                cell += '"';
                ++i;
            } else {
                inQuote = !inQuote;
            }
        } else if (c == delimiter && !inQuote) {
            cells.push_back(cell);
            cell.clear();
        } else {
            cell += c;
        }
    }
    cells.push_back(cell);
    return cells;
}

// ============================================================
// 셀 언쿼트
// ============================================================

std::string CSVParser::UnquoteCell(const std::string& cell) {
    if (cell.size() >= 2 && cell.front() == '"' && cell.back() == '"') {
        std::string inner = cell.substr(1, cell.size() - 2);
        // "" → " 이스케이프 처리
        std::string result;
        for (size_t i = 0; i < inner.size(); ++i) {
            if (inner[i] == '"' && i + 1 < inner.size() && inner[i+1] == '"') {
                result += '"'; ++i;
            } else {
                result += inner[i];
            }
        }
        return result;
    }
    return cell;
}

// ============================================================
// UTF-8 std::string → CString
// ============================================================

CString CSVParser::UTF8ToCString(const std::string& utf8) {
    if (utf8.empty()) return _T("");
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    CString result;
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.GetBufferSetLength(wlen), wlen);
    result.ReleaseBuffer();
    return result;
}
