#pragma once

// ============================================================
// ExportFormatter.h — 내보내기 포맷 순수 문자열 빌더 (헤더 전용)
//
// 목적: Export(CSV/HTML) 포맷팅 로직을 MFC/GDI+/파일 I/O에서 분리하여
//       ATL 전용 환경(Google Test)에서 결정론적으로 단위 테스트 가능하게 한다.
//
// 제약: ATL CString(atlstr.h) + std 만 사용. MFC(afxwin.h), CStdioFile,
//       GDI+ 의존성 없음. base64 이미지 임베드/파일 I/O는 다이얼로그에 남는다.
//
// 보안: dataJson은 LLM/분석 출력(신뢰 불가). CsvSafeField로 수식 인젝션을
//       방지하고(보안 리뷰 M1), HtmlEscape로 XSS를 방지한다.
// ============================================================

#include <atlstr.h>   // CString (ATL 경량 버전, MFC afxwin.h 대신 사용)
#include <windows.h>  // MultiByteToWideChar, CP_UTF8
#include <string>
#include <vector>
#include <sstream>

namespace ExportFormatter
{
    // ============================================================
    // UTF-8 std::string → CString 변환 (내부 헬퍼)
    // ============================================================
    inline CString Utf8ToCString(const std::string& s)
    {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        CString cs;
        if (wlen > 0)
        {
            MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1,
                                cs.GetBufferSetLength(wlen), wlen);
            cs.ReleaseBuffer();
        }
        return cs;
    }

    // ============================================================
    // CSV 필드 보안 처리 (보안 리뷰 M1: 수식 인젝션 방지 + 이스케이프)
    // dataJson은 LLM/분석 출력(신뢰 불가). =,+,-,@,TAB,CR로 시작하는 값은
    // 스프레드시트에서 수식으로 실행될 수 있으므로 작은따옴표로 무력화하고,
    // 구분자/따옴표/개행이 포함된 필드는 RFC 4180 방식으로 따옴표 감싼다.
    // ============================================================
    inline CString CsvSafeField(const CString& in)
    {
        CString s = in;
        if (!s.IsEmpty())
        {
            TCHAR c0 = s[0];
            if (c0 == _T('=') || c0 == _T('+') || c0 == _T('-') ||
                c0 == _T('@') || c0 == _T('\t') || c0 == _T('\r'))
                s = _T("'") + s;
        }
        if (s.Find(_T(',')) >= 0 || s.Find(_T('"')) >= 0 ||
            s.Find(_T('\n')) >= 0 || s.Find(_T('\r')) >= 0)
        {
            s.Replace(_T("\""), _T("\"\""));
            s = _T("\"") + s + _T("\"");
        }
        return s;
    }

    // ============================================================
    // dataJson → CSV 텍스트 변환
    // columns/rows 형식과 labels/datasets 형식을 모두 지원한다.
    // 줄 끝은 다이얼로그와 동일하게 \n 을 사용한다.
    // 각 필드에 CsvSafeField를 적용한다.
    // ============================================================
    inline CString BuildCsvFromDataJson(const CString& dataJson)
    {
        if (dataJson.IsEmpty())
            return CString();

        CT2A utf8Json(dataJson, CP_UTF8);
        std::string json(static_cast<const char*>(utf8Json));

        bool isTableFormat = (json.find("\"columns\"") != std::string::npos &&
                              json.find("\"rows\"") != std::string::npos);
        bool isChartFormat = (json.find("\"labels\"") != std::string::npos &&
                              json.find("\"datasets\"") != std::string::npos);

        CString csv;

        if (isTableFormat)
        {
            // columns/rows 형식 → CSV
            size_t colStart = json.find("\"columns\"");
            size_t arrStart = json.find('[', colStart);
            size_t arrEnd   = json.find(']', arrStart);
            std::string colArr = json.substr(arrStart + 1, arrEnd - arrStart - 1);

            std::vector<CString> columns;
            size_t pos = 0;
            while ((pos = colArr.find('"', pos)) != std::string::npos)
            {
                size_t end = colArr.find('"', pos + 1);
                if (end == std::string::npos) break;
                columns.push_back(Utf8ToCString(colArr.substr(pos + 1, end - pos - 1)));
                pos = end + 1;
            }

            CString headerLine;
            for (size_t i = 0; i < columns.size(); ++i)
            {
                if (i > 0) headerLine += _T(",");
                headerLine += CsvSafeField(columns[i]);
            }
            csv += headerLine + _T("\n");

            size_t rowsStart    = json.find("\"rows\"");
            size_t rowsArrStart = json.find('[', rowsStart);
            size_t searchPos    = rowsArrStart + 1;
            while ((searchPos = json.find('[', searchPos)) != std::string::npos)
            {
                size_t rowEnd = json.find(']', searchPos);
                if (rowEnd == std::string::npos) break;
                std::string rowStr = json.substr(searchPos + 1, rowEnd - searchPos - 1);

                CString rowLine;
                size_t vpos = 0;
                bool first = true;
                while ((vpos = rowStr.find('"', vpos)) != std::string::npos)
                {
                    size_t vend = rowStr.find('"', vpos + 1);
                    if (vend == std::string::npos) break;
                    CString cs = Utf8ToCString(rowStr.substr(vpos + 1, vend - vpos - 1));
                    if (!first) rowLine += _T(",");
                    rowLine += CsvSafeField(cs);
                    first = false;
                    vpos = vend + 1;
                }
                csv += rowLine + _T("\n");
                searchPos = rowEnd + 1;
            }
        }
        else if (isChartFormat)
        {
            // labels/datasets 형식 → CSV
            size_t labStart    = json.find("\"labels\"");
            size_t labArrStart = json.find('[', labStart);
            size_t labArrEnd   = json.find(']', labArrStart);
            std::string labArr = json.substr(labArrStart + 1, labArrEnd - labArrStart - 1);

            std::vector<std::string> labels;
            size_t lp = 0;
            while ((lp = labArr.find('"', lp)) != std::string::npos)
            {
                size_t le = labArr.find('"', lp + 1);
                if (le == std::string::npos) break;
                labels.push_back(labArr.substr(lp + 1, le - lp - 1));
                lp = le + 1;
            }

            std::vector<std::string> dsNames;
            std::vector<std::vector<std::string>> dsValues;

            size_t dsStart     = json.find("\"datasets\"");
            size_t dsArrStart  = json.find('[', dsStart);
            size_t dsSearchPos = dsArrStart + 1;

            while (true)
            {
                size_t namePos = json.find("\"name\"", dsSearchPos);
                if (namePos == std::string::npos) break;

                size_t ns = json.find('"', namePos + 6);
                size_t ne = json.find('"', ns + 1);
                dsNames.push_back(json.substr(ns + 1, ne - ns - 1));

                size_t valPos  = json.find("\"values\"", ne);
                size_t valArrS = json.find('[', valPos);
                size_t valArrE = json.find(']', valArrS);
                std::string valStr = json.substr(valArrS + 1, valArrE - valArrS - 1);

                std::vector<std::string> vals;
                std::istringstream vss(valStr);
                std::string token;
                while (std::getline(vss, token, ','))
                {
                    size_t s  = token.find_first_not_of(" \t\r\n");
                    size_t e2 = token.find_last_not_of(" \t\r\n");
                    if (s != std::string::npos)
                        vals.push_back(token.substr(s, e2 - s + 1));
                }
                dsValues.push_back(vals);
                dsSearchPos = valArrE + 1;
            }

            // 헤더 행
            CString headerLine;
            for (size_t i = 0; i < dsNames.size(); ++i)
            {
                headerLine += _T(",");
                headerLine += CsvSafeField(Utf8ToCString(dsNames[i]));
            }
            csv += headerLine + _T("\n");

            // 데이터 행
            for (size_t r = 0; r < labels.size(); ++r)
            {
                CString line = CsvSafeField(Utf8ToCString(labels[r]));
                for (size_t d = 0; d < dsValues.size(); ++d)
                {
                    line += _T(",");
                    if (r < dsValues[d].size())
                        line += CsvSafeField(Utf8ToCString(dsValues[d][r]));
                }
                csv += line + _T("\n");
            }
        }
        else
        {
            // 알 수 없는 형식 — raw data 출력 (제목은 다이얼로그가 별도로 기록)
            csv += dataJson + _T("\n");
        }

        return csv;
    }

    // ============================================================
    // HTML 특수문자 이스케이프 (XSS 방지)
    // & < > " 를 엔티티로 치환한다.
    // ============================================================
    inline CString HtmlEscape(const CString& in)
    {
        CString out;
        for (int i = 0; i < in.GetLength(); ++i)
        {
            TCHAR c = in[i];
            switch (c)
            {
            case _T('&'): out += _T("&amp;");  break;
            case _T('<'): out += _T("&lt;");   break;
            case _T('>'): out += _T("&gt;");   break;
            case _T('"'): out += _T("&quot;"); break;
            default:      out += c;            break;
            }
        }
        return out;
    }

    // ============================================================
    // dataJson → HTML <table> 변환
    // columns/rows 형식과 labels/datasets 형식을 모두 지원한다.
    // 모든 셀 값에 HtmlEscape를 적용한다. base64 이미지는 포함하지 않는다.
    // ============================================================
    inline CString BuildHtmlTableFromDataJson(const CString& dataJson)
    {
        if (dataJson.IsEmpty())
            return CString();

        CT2A utf8Json(dataJson, CP_UTF8);
        std::string json(static_cast<const char*>(utf8Json));

        bool isTableFormat = (json.find("\"columns\"") != std::string::npos &&
                              json.find("\"rows\"") != std::string::npos);
        bool isChartFormat = (json.find("\"labels\"") != std::string::npos &&
                              json.find("\"datasets\"") != std::string::npos);

        CString html;
        html += _T("    <table>\n");

        if (isTableFormat)
        {
            // columns 헤더
            size_t colStart = json.find("\"columns\"");
            size_t arrStart = json.find('[', colStart);
            size_t arrEnd   = json.find(']', arrStart);
            std::string colArr = json.substr(arrStart + 1, arrEnd - arrStart - 1);

            html += _T("      <thead><tr>");
            size_t pos = 0;
            while ((pos = colArr.find('"', pos)) != std::string::npos)
            {
                size_t end = colArr.find('"', pos + 1);
                if (end == std::string::npos) break;
                CString col = Utf8ToCString(colArr.substr(pos + 1, end - pos - 1));
                html += _T("<th>") + HtmlEscape(col) + _T("</th>");
                pos = end + 1;
            }
            html += _T("</tr></thead>\n      <tbody>\n");

            // rows
            size_t rowsStart    = json.find("\"rows\"");
            size_t rowsArrStart = json.find('[', rowsStart);
            size_t searchPos    = rowsArrStart + 1;
            while ((searchPos = json.find('[', searchPos)) != std::string::npos)
            {
                size_t rowEnd = json.find(']', searchPos);
                if (rowEnd == std::string::npos) break;
                std::string rowStr = json.substr(searchPos + 1, rowEnd - searchPos - 1);

                html += _T("        <tr>");
                size_t vpos = 0;
                while ((vpos = rowStr.find('"', vpos)) != std::string::npos)
                {
                    size_t vend = rowStr.find('"', vpos + 1);
                    if (vend == std::string::npos) break;
                    CString val = Utf8ToCString(rowStr.substr(vpos + 1, vend - vpos - 1));
                    html += _T("<td>") + HtmlEscape(val) + _T("</td>");
                    vpos = vend + 1;
                }
                html += _T("</tr>\n");
                searchPos = rowEnd + 1;
            }
            html += _T("      </tbody>\n");
        }
        else if (isChartFormat)
        {
            // labels
            size_t labStart    = json.find("\"labels\"");
            size_t labArrStart = json.find('[', labStart);
            size_t labArrEnd   = json.find(']', labArrStart);
            std::string labArr = json.substr(labArrStart + 1, labArrEnd - labArrStart - 1);

            std::vector<std::string> labels;
            size_t lp = 0;
            while ((lp = labArr.find('"', lp)) != std::string::npos)
            {
                size_t le = labArr.find('"', lp + 1);
                if (le == std::string::npos) break;
                labels.push_back(labArr.substr(lp + 1, le - lp - 1));
                lp = le + 1;
            }

            std::vector<std::string> dsNames;
            std::vector<std::vector<std::string>> dsValues;

            size_t dsStart     = json.find("\"datasets\"");
            size_t dsArrStart  = json.find('[', dsStart);
            size_t dsSearchPos = dsArrStart + 1;

            while (true)
            {
                size_t namePos = json.find("\"name\"", dsSearchPos);
                if (namePos == std::string::npos) break;

                size_t ns = json.find('"', namePos + 6);
                size_t ne = json.find('"', ns + 1);
                dsNames.push_back(json.substr(ns + 1, ne - ns - 1));

                size_t valPos  = json.find("\"values\"", ne);
                size_t valArrS = json.find('[', valPos);
                size_t valArrE = json.find(']', valArrS);
                std::string valStr = json.substr(valArrS + 1, valArrE - valArrS - 1);

                std::vector<std::string> vals;
                std::istringstream vss(valStr);
                std::string token;
                while (std::getline(vss, token, ','))
                {
                    size_t s  = token.find_first_not_of(" \t\r\n");
                    size_t e2 = token.find_last_not_of(" \t\r\n");
                    if (s != std::string::npos)
                        vals.push_back(token.substr(s, e2 - s + 1));
                }
                dsValues.push_back(vals);
                dsSearchPos = valArrE + 1;
            }

            // 헤더 행
            html += _T("      <thead><tr><th></th>");
            for (size_t i = 0; i < dsNames.size(); ++i)
                html += _T("<th>") + HtmlEscape(Utf8ToCString(dsNames[i])) + _T("</th>");
            html += _T("</tr></thead>\n      <tbody>\n");

            // 데이터 행
            for (size_t r = 0; r < labels.size(); ++r)
            {
                html += _T("        <tr><td>") + HtmlEscape(Utf8ToCString(labels[r])) + _T("</td>");
                for (size_t d = 0; d < dsValues.size(); ++d)
                {
                    CString v;
                    if (r < dsValues[d].size())
                        v = HtmlEscape(Utf8ToCString(dsValues[d][r]));
                    html += _T("<td>") + v + _T("</td>");
                }
                html += _T("</tr>\n");
            }
            html += _T("      </tbody>\n");
        }
        else
        {
            // 알 수 없는 형식 — raw JSON을 한 셀에 출력
            html += _T("      <tbody><tr><td>") + HtmlEscape(dataJson) + _T("</td></tr></tbody>\n");
        }

        html += _T("    </table>\n");
        return html;
    }

}  // namespace ExportFormatter
