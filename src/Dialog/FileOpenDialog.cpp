#include "stdafx.h"
#include "FileOpenDialog.h"

// ============================================================
// 상수 정의
// ============================================================
const TCHAR CFileOpenDialog::FILE_FILTER[] =
    _T("데이터 파일 (*.csv;*.xlsx;*.json)|*.csv;*.xlsx;*.json|")
    _T("CSV 파일 (*.csv)|*.csv|")
    _T("Excel 파일 (*.xlsx)|*.xlsx|")
    _T("JSON 파일 (*.json)|*.json|")
    _T("모든 파일 (*.*)|*.*||");

const TCHAR CFileOpenDialog::MRU_REG_KEY[] =
    _T("Software\\DeepMetria\\RecentFiles");

// ============================================================
// 생성
// ============================================================
CFileOpenDialog::CFileOpenDialog()
{
    LoadMRUFromRegistry();
}

// ============================================================
// 공개 인터페이스
// ============================================================
BOOL CFileOpenDialog::ShowAndGetPath(CString& outPath, CWnd* pParent)
{
    CFileDialog dlg(
        TRUE,                                   // 열기 모드
        _T("csv"),                              // 기본 확장자
        nullptr,                                // 초기 파일명
        OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY,
        FILE_FILTER,
        pParent
    );

    // 마지막 열었던 디렉터리로 초기 경로 설정
    if (!m_lastDirectory.IsEmpty())
        dlg.m_ofn.lpstrInitialDir = m_lastDirectory;

    if (dlg.DoModal() != IDOK)
        return FALSE;

    outPath = dlg.GetPathName();

    // 디렉터리 저장 (다음 열기 시 사용)
    m_lastDirectory = dlg.GetFolderPath();

    // MRU 갱신
    AddToMRU(outPath);

    return TRUE;
}

// ============================================================
// MRU 관리
// ============================================================
void CFileOpenDialog::AddToMRU(const CString& path)
{
    // 중복 제거 (이미 있으면 맨 앞으로 이동)
    for (int i = 0; i < m_mruList.GetSize(); ++i)
    {
        if (m_mruList[i].CompareNoCase(path) == 0)
        {
            m_mruList.RemoveAt(i);
            break;
        }
    }

    // 맨 앞에 추가
    m_mruList.InsertAt(0, path);

    // 최대 개수 유지
    while (m_mruList.GetSize() > MRU_MAX_COUNT)
        m_mruList.RemoveAt(m_mruList.GetSize() - 1);

    SaveMRUToRegistry();
}

void CFileOpenDialog::GetMRUList(CStringArray& outList) const
{
    outList.Copy(m_mruList);
}

void CFileOpenDialog::ClearMRU()
{
    m_mruList.RemoveAll();
    SaveMRUToRegistry();
}

// ============================================================
// 레지스트리 저장 / 로드
// ============================================================
void CFileOpenDialog::SaveMRUToRegistry()
{
    CRegKey reg;
    if (reg.Create(HKEY_CURRENT_USER, MRU_REG_KEY) != ERROR_SUCCESS)
        return;

    // 기존 항목 삭제 후 재작성
    for (int i = 0; i < MRU_MAX_COUNT; ++i)
    {
        CString valueName;
        valueName.Format(_T("File%d"), i);
        reg.DeleteValue(valueName);
    }

    for (int i = 0; i < m_mruList.GetSize(); ++i)
    {
        CString valueName;
        valueName.Format(_T("File%d"), i);
        reg.SetStringValue(valueName, m_mruList[i]);
    }
}

void CFileOpenDialog::LoadMRUFromRegistry()
{
    m_mruList.RemoveAll();

    CRegKey reg;
    if (reg.Open(HKEY_CURRENT_USER, MRU_REG_KEY, KEY_READ) != ERROR_SUCCESS)
        return;

    for (int i = 0; i < MRU_MAX_COUNT; ++i)
    {
        CString valueName;
        valueName.Format(_T("File%d"), i);

        TCHAR buf[MAX_PATH] = {};
        ULONG len = MAX_PATH;
        if (reg.QueryStringValue(valueName, buf, &len) == ERROR_SUCCESS && len > 0)
            m_mruList.Add(buf);
        else
            break; // 연속되지 않으면 중단
    }
}
