#include "stdafx.h"
#include "CrashRecoveryManager.h"
#include <fstream>
#include <sstream>
#include <ShlObj.h>

namespace CrashRecovery {

CString CrashRecoveryManager::GetAppDataDir()
{
    TCHAR appDataPath[MAX_PATH] = {};
    ::SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath);
    CString dir;
    dir.Format(_T("%s\\DeepMetria"), appDataPath);
    return dir;
}

CString CrashRecoveryManager::GetStateFilePath()
{
    return GetAppDataDir() + _T("\\last_state.json");
}

BOOL CrashRecoveryManager::SaveLastState(const AppLastState& state)
{
    CString dir = GetAppDataDir();
    ::CreateDirectory(dir, nullptr);

    CString path = GetStateFilePath();
    CStdioFile file;
    if (!file.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
        return FALSE;

    CString json;
    json.Format(
        _T("{\n")
        _T("  \"lastFilePath\": \"%s\",\n")
        _T("  \"activeDashboardId\": \"%s\",\n")
        _T("  \"windowX\": %d,\n")
        _T("  \"windowY\": %d,\n")
        _T("  \"windowW\": %d,\n")
        _T("  \"windowH\": %d,\n")
        _T("  \"dirty\": %s\n")
        _T("}"),
        (LPCTSTR)state.lastFilePath,
        (LPCTSTR)state.activeDashboardId,
        state.windowX, state.windowY, state.windowW, state.windowH,
        state.dirty ? _T("true") : _T("false"));

    file.WriteString(json);
    file.Close();
    return TRUE;
}

BOOL CrashRecoveryManager::LoadLastState(AppLastState& outState)
{
    CString path = GetStateFilePath();
    if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    std::wstring wPath((LPCTSTR)path);
    std::ifstream fin(wPath, std::ios::binary);
    if (!fin.is_open())
        return FALSE;

    std::string raw((std::istreambuf_iterator<char>(fin)),
                     std::istreambuf_iterator<char>());
    fin.close();

    if (raw.empty())
        return FALSE;

    // 간단한 JSON 파싱
    auto extractString = [&](const std::string& key) -> CString {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = raw.find(searchKey);
        if (pos == std::string::npos) return _T("");
        pos = raw.find('"', pos + searchKey.size() + 1);
        if (pos == std::string::npos) return _T("");
        size_t end = raw.find('"', pos + 1);
        if (end == std::string::npos) return _T("");
        std::string val = raw.substr(pos + 1, end - pos - 1);
        int wlen = MultiByteToWideChar(CP_UTF8, 0, val.c_str(), -1, nullptr, 0);
        CString result;
        MultiByteToWideChar(CP_UTF8, 0, val.c_str(), -1, result.GetBufferSetLength(wlen), wlen);
        result.ReleaseBuffer();
        return result;
    };

    auto extractInt = [&](const std::string& key) -> int {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = raw.find(searchKey);
        if (pos == std::string::npos) return 0;
        pos = raw.find(':', pos);
        if (pos == std::string::npos) return 0;
        return std::atoi(raw.c_str() + pos + 1);
    };

    auto extractBool = [&](const std::string& key) -> bool {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = raw.find(searchKey);
        if (pos == std::string::npos) return false;
        return (raw.find("true", pos) != std::string::npos &&
                raw.find("true", pos) < raw.find('\n', pos));
    };

    outState.lastFilePath       = extractString("lastFilePath");
    outState.activeDashboardId  = extractString("activeDashboardId");
    outState.windowX            = extractInt("windowX");
    outState.windowY            = extractInt("windowY");
    outState.windowW            = extractInt("windowW");
    outState.windowH            = extractInt("windowH");
    outState.dirty              = extractBool("dirty");

    return TRUE;
}

BOOL CrashRecoveryManager::HandleCorruptState()
{
    CString path = GetStateFilePath();
    if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    // 파일 내용 검증
    std::wstring wPath((LPCTSTR)path);
    std::ifstream fin(wPath, std::ios::binary);
    if (!fin.is_open())
        return FALSE;

    std::string raw((std::istreambuf_iterator<char>(fin)),
                     std::istreambuf_iterator<char>());
    fin.close();

    // 기본적인 JSON 유효성 검사: '{' 로 시작, '}' 로 끝나야 함
    size_t first = raw.find_first_not_of(" \t\r\n");
    size_t last  = raw.find_last_not_of(" \t\r\n");
    if (first == std::string::npos || last == std::string::npos ||
        raw[first] != '{' || raw[last] != '}')
    {
        // 손상된 파일 삭제
        DeleteFile(path);
        return TRUE;
    }

    return FALSE; // 정상 파일
}

} // namespace CrashRecovery
