#pragma once

// FileOpenDialog.h — 파일 열기 다이얼로그 래퍼
// CSV, Excel 필터 지원, 최근 파일 목록(MRU) 관리
// Architecture §3 / DetailedSpec §3.3 참조

#include "../stdafx.h"

// ============================================================
// CFileOpenDialog — CFileDialog 래퍼
// ============================================================
class CFileOpenDialog
{
public:
    CFileOpenDialog();
    ~CFileOpenDialog() = default;

    // 다이얼로그 표시 후 선택된 경로 반환
    // 반환값: TRUE = 파일 선택됨, FALSE = 취소
    BOOL ShowAndGetPath(CString& outPath, CWnd* pParent = nullptr);

    // 최근 파일 목록 (MRU)
    void        AddToMRU(const CString& path);
    void        GetMRUList(CStringArray& outList) const;
    void        ClearMRU();

private:
    // 지원 파일 필터 문자열
    static const TCHAR FILE_FILTER[];

    // 최근 파일 레지스트리 키
    static const TCHAR MRU_REG_KEY[];
    static const int   MRU_MAX_COUNT = 10;

    // 마지막 열었던 디렉터리 (다음 열기 시 초기 경로로 사용)
    CString m_lastDirectory;

    // 레지스트리에서 MRU 로드/저장
    void LoadMRUFromRegistry();
    void SaveMRUToRegistry();

    // MRU 내부 목록
    CStringArray m_mruList;
};
