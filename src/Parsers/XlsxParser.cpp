// Parsers/XlsxParser.cpp
// PowerShell + Excel COM 으로 .xlsx → 임시 CSV 변환 → 기존 CsvParser 로 파싱.
// 장점: 구현 단순, deflate/XML 라이브러리 불필요
// 제약: Microsoft Excel 또는 호환 Office 가 설치되어 있어야 함
#include "stdafx.h"
#include "XlsxParser.h"
#include "CsvParser.h"
#include <windows.h>
#include <string>
#include <vector>

namespace deepmetria {

namespace {

std::wstring MakeTempCsvPath()
{
    wchar_t tempDir[MAX_PATH] = {};
    DWORD n = GetTempPathW(MAX_PATH, tempDir);
    std::wstring dir = (n > 0) ? std::wstring(tempDir) : std::wstring(L".\\");

    SYSTEMTIME st{};
    GetSystemTime(&st);
    DWORD pid = GetCurrentProcessId();
    wchar_t name[128];
    swprintf_s(name, L"deepmetria_xlsx_%04d%02d%02d_%02d%02d%02d_%u.csv",
               st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, pid);
    return dir + name;
}

// PowerShell 명령 한 줄 — 작은따옴표로 경로 감싸기 (경로 내 single-quote 는 escape 필요)
std::wstring EscapeForPsSingleQuote(const std::wstring& s)
{
    std::wstring out;
    out.reserve(s.size());
    for (wchar_t c : s)
    {
        if (c == L'\'') out += L"''";
        else out += c;
    }
    return out;
}

// Excel COM 으로 xlsx 열고 CSV(xlCSV=6) 형식으로 SaveAs.
// timeoutMs 안에 안 끝나면 강제 종료.
bool RunPowerShellCommand(const std::wstring& cmdLine, DWORD timeoutMs)
{
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    std::vector<wchar_t> buf(cmdLine.begin(), cmdLine.end());
    buf.push_back(0);

    BOOL ok = CreateProcessW(
        nullptr,
        buf.data(),
        nullptr, nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr, nullptr,
        &si, &pi);
    if (!ok) return false;

    DWORD wait = WaitForSingleObject(pi.hProcess, timeoutMs);
    if (wait == WAIT_TIMEOUT)
    {
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 2000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return false;
    }
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode == 0;
}

} // namespace

bool XlsxParser::Parse(const std::wstring& path, DataSource& out)
{
    out.Reset();
    out.SetSourcePath(path);

    // 1) 임시 CSV 경로 준비
    std::wstring csvPath = MakeTempCsvPath();

    std::wstring inEsc  = EscapeForPsSingleQuote(path);
    std::wstring outEsc = EscapeForPsSingleQuote(csvPath);

    // 2) PowerShell + Excel COM 한 줄 명령
    //    - DisplayAlerts 끄고 SaveAs 6 = xlCSV
    //    - 종료 후 Excel COM 객체 해제
    std::wstring ps =
        L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "
        L"\""
        L"try { "
        L"$x = New-Object -ComObject Excel.Application; "
        L"$x.Visible = $false; "
        L"$x.DisplayAlerts = $false; "
        L"$w = $x.Workbooks.Open('" + inEsc + L"', 0, $true); "
        L"$w.SaveAs('" + outEsc + L"', 6); "
        L"$w.Close($false); "
        L"$x.Quit(); "
        L"[System.Runtime.InteropServices.Marshal]::ReleaseComObject($x) | Out-Null; "
        L"exit 0 "
        L"} catch { exit 1 }"
        L"\"";

    // 3) 실행 — 60초 안에 변환 완료 기대
    bool psOk = RunPowerShellCommand(ps, 60 * 1000);
    if (!psOk)
    {
        // 임시 파일이 일부 만들어졌을 수 있으니 정리
        DeleteFileW(csvPath.c_str());
        return false;
    }

    // 4) 만들어진 CSV 를 기존 파서로 읽기
    CsvParser csvParser;
    bool ok = csvParser.Parse(csvPath, out);
    out.SetSourcePath(path);  // 원본 경로 복원 (CsvParser 가 csv 경로로 덮어쓰니까)

    // 5) 임시 파일 정리
    DeleteFileW(csvPath.c_str());
    return ok;
}

} // namespace deepmetria
