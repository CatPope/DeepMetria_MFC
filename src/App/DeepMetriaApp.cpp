// DeepMetriaApp.cpp — CDeepMetriaApp 구현
// 앱 초기화(InitInstance), 정리(ExitInstance) 담당

#include "stdafx.h"
#include "DeepMetriaApp.h"
#include "MainFrm.h"
#include "../Document/DeepMetriaDoc.h"
#include "../Resources/resource.h"
#include "../Infrastructure/Storage/SQLiteDB.h"
#include "../Infrastructure/Cache/AnalysisCache.h"
#include "../Dialog/FileOpenDialog.h"
#include "../Dialog/SettingsDialog.h"
#include "../Dialog/ExportDialog.h"
#include "../Domain/DataSource/DataSourceService.h"
#include "../Infrastructure/LLM/LLMRouter.h"
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")
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
    ON_COMMAND(ID_FILE_OPEN_DATA,      &CDeepMetriaApp::OnFileOpenData)
    ON_COMMAND(ID_TOOLS_SETTINGS,      &CDeepMetriaApp::OnToolsSettings)
    ON_COMMAND(ID_TOOLS_EXPORT,        &CDeepMetriaApp::OnToolsExport)
    ON_COMMAND(ID_ANALYSIS_NEW_QUERY,  &CDeepMetriaApp::OnAnalysisNewQuery)
    ON_COMMAND(ID_HELP_ABOUT,          &CDeepMetriaApp::OnAppAbout)
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

    // ---- LLM 설정 동기화 ----
    SyncLLMSettings();

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

// ---- 파일 열기 (커스텀) ----
void CDeepMetriaApp::OnFileOpenData()
{
    CFileOpenDialog fileOpenDlg;
    CString filePath;

    if (!fileOpenDlg.ShowAndGetPath(filePath, m_pMainWnd))
        return; // 사용자 취소

    // 현재 문서 가져오기
    CDocument* pDoc = nullptr;
    POSITION pos = GetFirstDocTemplatePosition();
    if (pos)
    {
        CDocTemplate* pTemplate = GetNextDocTemplate(pos);
        if (pTemplate)
        {
            POSITION docPos = pTemplate->GetFirstDocPosition();
            if (docPos)
                pDoc = pTemplate->GetNextDoc(docPos);
        }
    }

    CDeepMetriaDoc* pMetriaDoc = dynamic_cast<CDeepMetriaDoc*>(pDoc);
    if (!pMetriaDoc)
    {
        AfxMessageBox(_T("문서 객체를 찾을 수 없습니다."), MB_ICONERROR);
        return;
    }

    // 파일 로드
    AppError loadError;
    if (!pMetriaDoc->LoadFile(filePath, loadError))
    {
        AfxMessageBox(
            CString(_T("파일 로드 실패: ")) + loadError.message,
            MB_ICONERROR);
        return;
    }

    // 문서 제목 갱신 및 뷰 업데이트
    pMetriaDoc->SetPathName(filePath, TRUE);
    pMetriaDoc->UpdateAllViews(nullptr);

    // 메인 프레임 상태바 업데이트
    if (m_pMainWnd)
        m_pMainWnd->PostMessage(WM_DATA_LOADED, 0, 0);
}

// ---- 도구 > 설정 ----
void CDeepMetriaApp::OnToolsSettings()
{
    CSettingsDialog dlg(m_pMainWnd);
    if (dlg.DoModal() == IDOK)
        SyncLLMSettings();
}

// ---- 도구 > 내보내기 ----
void CDeepMetriaApp::OnToolsExport()
{
    // 현재 문서 가져오기 (OnFileOpenData와 동일한 패턴)
    CDocument* pDoc = nullptr;
    POSITION pos = GetFirstDocTemplatePosition();
    if (pos)
    {
        CDocTemplate* pTemplate = GetNextDocTemplate(pos);
        if (pTemplate)
        {
            POSITION docPos = pTemplate->GetFirstDocPosition();
            if (docPos)
                pDoc = pTemplate->GetNextDoc(docPos);
        }
    }

    CDeepMetriaDoc* pMetriaDoc = dynamic_cast<CDeepMetriaDoc*>(pDoc);
    if (!pMetriaDoc)
    {
        AfxMessageBox(_T("문서 객체를 찾을 수 없습니다."), MB_ICONERROR);
        return;
    }

    // 활성 차트(시각화) 데이터 확보 — 문서의 첫 번째 시각화를 사용
    const std::vector<VisualizationInfo>& vizList = pMetriaDoc->GetVisualizations();
    if (vizList.empty())
    {
        AfxMessageBox(_T("내보낼 차트가 없습니다. 먼저 분석을 실행하여 차트를 생성하세요."),
                      MB_ICONINFORMATION);
        return;
    }

    // 내보내기 다이얼로그를 활성 차트 데이터로 시드하여 표시
    CExportDialog dlg(vizList.front(), m_pMainWnd);
    dlg.DoModal();
}

// ---- 분석 > 새 질문 ----
void CDeepMetriaApp::OnAnalysisNewQuery()
{
    // QueryInputView의 에디트 컨트롤에 포커스 이동
    if (m_pMainWnd)
        m_pMainWnd->PostMessage(WM_ANALYSIS_NEW_QUERY, 0, 0);
}

// ---- About 박스 ----
void CDeepMetriaApp::OnAppAbout()
{
    CDialog dlg(IDD_ABOUTBOX);
    dlg.DoModal();
}

// ---- SettingsDialog 레지스트리 → LLMRouter 동기화 ----
void CDeepMetriaApp::SyncLLMSettings()
{
    // SettingsDialog가 사용하는 레지스트리 경로에서 읽기
    CRegKey reg;
    if (reg.Open(HKEY_CURRENT_USER, _T("Software\\DeepMetria\\Settings"), KEY_READ) != ERROR_SUCCESS)
        return;

    // DPAPI 복호화 람다 (SettingsDialog::DecryptString과 동일)
    auto decrypt = [](const CString& cipherB64) -> CString {
        if (cipherB64.IsEmpty()) return _T("");

        DWORD binLen = 0;
        ::CryptStringToBinary(cipherB64, cipherB64.GetLength(),
                              CRYPT_STRING_BASE64, nullptr, &binLen, nullptr, nullptr);
        if (binLen == 0) return _T("");

        std::vector<BYTE> binData(binLen);
        ::CryptStringToBinary(cipherB64, cipherB64.GetLength(),
                              CRYPT_STRING_BASE64, binData.data(), &binLen, nullptr, nullptr);

        DATA_BLOB dataIn = { binLen, binData.data() };
        DATA_BLOB dataOut = {};
        if (!::CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut))
            return _T("");

        CString result(reinterpret_cast<LPCTSTR>(dataOut.pbData));
        ::LocalFree(dataOut.pbData);
        return result;
    };

    TCHAR buf[1024] = {};
    ULONG len = 0;

    // 프로바이더 설정
    len = _countof(buf);
    if (reg.QueryStringValue(_T("Provider"), buf, &len) == ERROR_SUCCESS)
    {
        CString provider(buf);
        if (provider == _T("Claude"))
            LLMRouter::Instance().SetProvider(LLMRouter::Provider::Claude);
        else if (provider == _T("OpenAI"))
            LLMRouter::Instance().SetProvider(LLMRouter::Provider::OpenAI);
        else if (provider == _T("Gemini"))
            LLMRouter::Instance().SetProvider(LLMRouter::Provider::Gemini);
    }

    // 모델 설정
    len = _countof(buf);
    if (reg.QueryStringValue(_T("Model"), buf, &len) == ERROR_SUCCESS)
        LLMRouter::Instance().SetModel(buf);

    // API 키 로드 (DPAPI 복호화)
    len = _countof(buf);
    if (reg.QueryStringValue(_T("ClaudeKey"), buf, &len) == ERROR_SUCCESS)
    {
        CString key = decrypt(buf);
        if (!key.IsEmpty())
            LLMRouter::Instance().SetApiKey(LLMRouter::Provider::Claude, key);
    }

    len = _countof(buf);
    if (reg.QueryStringValue(_T("OpenAIKey"), buf, &len) == ERROR_SUCCESS)
    {
        CString key = decrypt(buf);
        if (!key.IsEmpty())
            LLMRouter::Instance().SetApiKey(LLMRouter::Provider::OpenAI, key);
    }

    len = _countof(buf);
    if (reg.QueryStringValue(_T("GeminiKey"), buf, &len) == ERROR_SUCCESS)
    {
        CString key = decrypt(buf);
        if (!key.IsEmpty())
            LLMRouter::Instance().SetApiKey(LLMRouter::Provider::Gemini, key);
    }

    TRACE(_T("[DeepMetriaApp] LLM 설정 동기화 완료\n"));
}
