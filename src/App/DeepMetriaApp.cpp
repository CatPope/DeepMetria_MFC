// DeepMetriaApp.cpp — CDeepMetriaApp 구현
// 앱 초기화(InitInstance), 정리(ExitInstance) 담당

#include "../stdafx.h"
#include "DeepMetriaApp.h"
#include "MainFrm.h"
#include "../Document/DeepMetriaDoc.h"
#include "../Resources/resource.h"
#include "../Infrastructure/Storage/SQLiteDB.h"
#include "../Infrastructure/Cache/AnalysisCache.h"
#include <shlobj.h>   // SHGetFolderPath, SHCreateDirectoryEx
#include <shlwapi.h>  // PathIsDirectory
#pragma comment(lib, "shlwapi.lib")

// ============================================================
// theApp 전역 인스턴스
// ============================================================
CDeepMetriaApp theApp;

// ============================================================
// 전역 AnalysisCache 인스턴스
// ============================================================
static AnalysisCache g_analysisCache;

// ============================================================
// CDeepMetriaApp
// ============================================================

BEGIN_MESSAGE_MAP(CDeepMetriaApp, CWinApp)
    ON_COMMAND(ID_FILE_OPEN_DATA, &CWinApp::OnFileOpen)
    ON_COMMAND(ID_HELP_ABOUT,     &CDeepMetriaApp::OnAppAbout)
END_MESSAGE_MAP()

CDeepMetriaApp::CDeepMetriaApp()
    : m_gdiplusToken(0)
{
}

BOOL CDeepMetriaApp::InitInstance()
{
    // ---- 공통 MFC 초기화 ----
    if (!CWinApp::InitInstance())
        return FALSE;

    // ---- GDI+ 초기화 ----
    Gdiplus::GdiplusStartupInput gdiplusInput;
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusInput, nullptr);

    // ---- SQLite DB 초기화 ----
    // %APPDATA%\DeepMetria\deepmetria.db 경로 구성, 폴더 자동 생성
    {
        TCHAR szAppData[MAX_PATH] = {};
        if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, szAppData)))
        {
            CString dbDir;
            dbDir.Format(_T("%s\\DeepMetria"), szAppData);

            // 폴더가 없으면 생성
            if (!PathIsDirectory(dbDir))
                SHCreateDirectoryEx(nullptr, dbDir, nullptr);

            CString dbPath;
            dbPath.Format(_T("%s\\deepmetria.db"), dbDir);

            AppError dbError;
            if (!SQLiteDB::Instance().Initialize(dbPath, dbError))
            {
                AfxMessageBox(
                    CString(_T("데이터베이스 초기화 실패: ")) + dbError.message,
                    MB_ICONERROR | MB_OK);
                return FALSE;
            }
        }
        else
        {
            AfxMessageBox(_T("APPDATA 경로를 가져올 수 없습니다."),
                          MB_ICONERROR | MB_OK);
            return FALSE;
        }
    }

    // ---- AnalysisCache 초기화 ----
    g_analysisCache.Clear();

    // ---- 공통 컨트롤 초기화 ----
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC  = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // ---- SDI 문서 템플릿 생성 ----
    // DeepMetriaDoc, CMainFrame, CDashboardView 연결
    CSingleDocTemplate* pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CDeepMetriaDoc),
        RUNTIME_CLASS(CMainFrame),
        nullptr  // CDashboardView는 OnCreateClient에서 Splitter로 생성
    );
    if (!pDocTemplate)
        return FALSE;

    AddDocTemplate(pDocTemplate);

    // ---- 명령줄 파싱 ----
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // 새 문서로 시작 (빈 대시보드)
    cmdInfo.m_nShellCommand = CCommandLineInfo::FileNew;

    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    // ---- 메인 윈도우 표시 ----
    m_pMainWnd->ShowWindow(m_nCmdShow);
    m_pMainWnd->UpdateWindow();

    return TRUE;
}

int CDeepMetriaApp::ExitInstance()
{
    // ---- DB / 캐시 정리 ----
    SQLiteDB::Instance().Close();
    g_analysisCache.Clear();

    // ---- GDI+ 종료 ----
    if (m_gdiplusToken != 0)
    {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }

    return CWinApp::ExitInstance();
}

// ---- About 박스 ----
// IDD_ABOUTBOX 다이얼로그는 리소스에 정의됨
void CDeepMetriaApp::OnAppAbout()
{
    CDialog dlg(IDD_ABOUTBOX);
    dlg.DoModal();
}
