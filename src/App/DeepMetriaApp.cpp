// DeepMetriaApp.cpp — CDeepMetriaApp 구현
// 앱 초기화(InitInstance), 정리(ExitInstance) 담당

#include "../stdafx.h"
#include "DeepMetriaApp.h"
#include "MainFrm.h"
#include "../Document/DeepMetriaDoc.h"
#include "../Resources/resource.h"

// View 전방 선언 (헤더 include 최소화)
class CDashboardView;

// ============================================================
// theApp 전역 인스턴스
// ============================================================
CDeepMetriaApp theApp;

// ============================================================
// CDeepMetriaApp
// ============================================================

BEGIN_MESSAGE_MAP(CDeepMetriaApp, CWinApp)
    ON_COMMAND(ID_FILE_OPEN_DATA, &CWinApp::OnFileOpen)
    ON_COMMAND(ID_HELP_ABOUT,     OnAppAbout)
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
    // SQLiteDB::GetInstance().Initialize() — 스켈레톤 단계에서 전방 선언만 유지
    // (Infrastructure/Storage/SQLiteDB.h 구현 후 연결)

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
    // SQLiteDB::GetInstance().Close();
    // AnalysisCache::GetInstance().Clear();

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
void OnAppAbout()
{
    CDialog dlg(IDD_ABOUTBOX);
    dlg.DoModal();
}
