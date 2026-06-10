// DeepMetria.cpp - 응용 프로그램 진입점
#include "stdafx.h"
#include "DeepMetria.h"
#include "MainFrame.h"
#include "DeepMetriaDoc.h"
#include "DeepMetriaView.h"
#include "SecretsStore.h"
#include "SelfTest.h"
#include <afxvisualmanagerwindows7.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CDeepMetriaApp, CWinAppEx)
    ON_COMMAND(ID_APP_ABOUT, &CDeepMetriaApp::OnAppAbout)
    ON_COMMAND(ID_FILE_NEW,  &CWinAppEx::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
END_MESSAGE_MAP()

CDeepMetriaApp::CDeepMetriaApp()
{
    SetAppID(_T("DeepMetria.AI.1"));
}

CDeepMetriaApp theApp;

BOOL CDeepMetriaApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls{};
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC  = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinAppEx::InitInstance();

    // GDI+ 초기화
    Gdiplus::GdiplusStartupInput gsi;
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &gsi, nullptr);

    // --self-test : 자동 검증 모드. 윈도우 안 띄우고 즉시 종료.
    for (int i = 1; i < __argc; ++i)
    {
        if (_wcsicmp(__wargv[i], L"--self-test") == 0 ||
            _wcsicmp(__wargv[i], L"--selftest")  == 0)
        {
            if (!AttachConsole(ATTACH_PARENT_PROCESS))
                AllocConsole();
            FILE* fp = nullptr;
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);

            int rc = deepmetria::RunSelfTestSuite();

            if (m_gdiplusToken) { Gdiplus::GdiplusShutdown(m_gdiplusToken); m_gdiplusToken = 0; }
            ExitProcess(static_cast<UINT>(rc));
        }
    }

    AfxOleInit();
    EnableTaskbarInteraction(FALSE);

    SetRegistryKey(_T("DeepMetria"));
    LoadStdProfileSettings(8);

    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));

    InitContextMenuManager();
    InitKeyboardManager();
    InitTooltipManager();
    CMFCToolTipInfo ttParams;
    ttParams.m_bVislManagerTheme = TRUE;
    GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
        RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

    // LLM Router 구성 — DPAPI 레지스트리에서 키를 복호화
    auto secrets = std::make_shared<deepmetria::SecretsStore>(L"Software\\DeepMetria\\Settings");
    m_llmRouter  = std::make_shared<deepmetria::LLMRouter>(secrets);
    m_llmRouter->LoadConfig();

    CSingleDocTemplate* pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CDeepMetriaDoc),
        RUNTIME_CLASS(CMainFrame),
        RUNTIME_CLASS(CDeepMetriaView));
    if (!pDocTemplate)
        return FALSE;
    AddDocTemplate(pDocTemplate);

    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // SDI 자동 OnFileNew 우회 — 모달 메시지박스 회피
    if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
        cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

    BOOL psc = ProcessShellCommand(cmdInfo);
    if (!psc)
        return FALSE;

    // 수동 SDI 부트스트랩 — view까지 만들고 SingleDocTemplate에 정식 등록.
    // 표준 ProcessShellCommand(FileNew)는 일부 환경에서 모달을 띄우므로 우회하되,
    // 우회 후 SingleDocTemplate::m_pOnlyDoc 미등록이 되면 [열기]가 새 frame을
    // 또 만들기 때문에, 여기서 doc + frame + view를 수동으로 생성하고 등록한다.
    if (!m_pMainWnd)
    {
        CCreateContext context = {};
        context.m_pNewViewClass   = RUNTIME_CLASS(CDeepMetriaView);
        context.m_pNewDocTemplate = pDocTemplate;
        context.m_pCurrentDoc     = nullptr;
        context.m_pCurrentFrame   = nullptr;
        context.m_pLastView       = nullptr;

        CDeepMetriaDoc* pDoc = static_cast<CDeepMetriaDoc*>(
            pDocTemplate->CreateNewDocument());
        if (!pDoc) return FALSE;
        if (!pDoc->OnNewDocument()) { delete pDoc; return FALSE; }
        context.m_pCurrentDoc = pDoc;

        CMainFrame* pFrame = new CMainFrame;
        if (!pFrame) { delete pDoc; return FALSE; }

        BOOL lf = pFrame->LoadFrame(IDR_MAINFRAME,
            WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, nullptr, &context);

        if (!lf)
        {
            // context로 LoadFrame 실패 시 (view 생성 실패) — context 없이 재시도.
            // 이 경우 view는 없지만 frame이라도 띄움.
            delete pFrame;
            pFrame = new CMainFrame;
            if (!pFrame ||
                !pFrame->LoadFrame(IDR_MAINFRAME,
                    WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, nullptr, nullptr))
            {
                delete pFrame; delete pDoc;
                return FALSE;
            }
            m_pMainWnd = pFrame;
        }
        else
        {
            // 정상: SingleDocTemplate에 doc/frame 등록 → 표준 OnFileOpen이
            // 기존 doc를 재사용하여 새 frame을 만들지 않게 된다.
            pDocTemplate->InitialUpdateFrame(pFrame, pDoc, TRUE);
            m_pMainWnd = pFrame;
        }
    }

    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();

    return TRUE;
}

int CDeepMetriaApp::ExitInstance()
{
    if (m_gdiplusToken)
    {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }
    return CWinAppEx::ExitInstance();
}

// 정보 대화 상자
class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override { CDialogEx::DoDataExchange(pDX); }
    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

void CDeepMetriaApp::OnAppAbout()
{
    CAboutDlg about;
    about.DoModal();
}
