// ExportDialog.cpp - 내보내기 모달 구현
#include "stdafx.h"
#include "ExportDialog.h"
#include "ChartRenderer.h"
#include <shlobj.h>  // SHCreateDirectoryEx

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CExportDialog, CDialogEx)

BEGIN_MESSAGE_MAP(CExportDialog, CDialogEx)
    ON_BN_CLICKED(IDC_EXPORT_BROWSE, &CExportDialog::OnBtnBrowse)
    ON_BN_CLICKED(IDOK,              &CExportDialog::OnBtnSave)
END_MESSAGE_MAP()

CExportDialog::CExportDialog(
    const deepmetria::DataSource& ds,
    const deepmetria::Dashboard&  dash,
    int selectedVizIdx,
    CWnd* pParent)
    : CDialogEx(IDD_EXPORT, pParent)
    , m_ds(ds)
    , m_dash(dash)
    , m_selectedVizIdx(selectedVizIdx)
{
}

CExportDialog::~CExportDialog() = default;

void CExportDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EXPORT_PREVIEW,    m_stcPreview);
    DDX_Control(pDX, IDC_EXPORT_PATH,       m_edPath);
    DDX_Control(pDX, IDC_EXPORT_BROWSE,     m_btnBrowse);
    DDX_Control(pDX, IDC_EXPORT_RESOLUTION, m_cmbResolution);
}

BOOL CExportDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 해상도 목록
    m_cmbResolution.AddString(_T("1x (540×360)"));
    m_cmbResolution.AddString(_T("2x 고해상도 (1080×720)"));
    m_cmbResolution.AddString(_T("4x 초고해상도 (2160×1440)"));
    m_cmbResolution.SetCurSel(1);

    // 기본 저장 경로 — 사용자 바탕화면 (권한 안전, 즉시 확인 가능)
    wchar_t profile[MAX_PATH] = {};
    DWORD n = GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH);
    CString defaultPath;
    if (n > 0)
        defaultPath.Format(_T("%s\\Desktop\\dashboard.png"), profile);
    else
        defaultPath = _T("dashboard.png");
    m_edPath.SetWindowText(defaultPath);

    // PNG 라디오 기본 선택
    CheckDlgButton(IDC_EXPORT_FORMAT_PNG, BST_CHECKED);

    // 선택된 viz 가 있으면 [선택중인 그래프만] 기본, 없으면 [대시보드 전체]
    if (m_selectedVizIdx >= 0)
        CheckDlgButton(IDC_EXPORT_RADIO_SINGLE, BST_CHECKED);
    else
    {
        CheckDlgButton(IDC_EXPORT_RADIO_ALL, BST_CHECKED);
        // 선택된 시각화가 없으면 [선택중인 그래프만] 옵션은 비활성화
        if (CWnd* pSingle = GetDlgItem(IDC_EXPORT_RADIO_SINGLE))
            pSingle->EnableWindow(FALSE);
    }

    return TRUE;
}

void CExportDialog::OnBtnBrowse()
{
    CFileDialog dlg(FALSE, _T("png"), _T("dashboard.png"),
        OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
        _T("PNG 이미지 (*.png)|*.png|BMP 이미지 (*.bmp)|*.bmp|JPEG 이미지 (*.jpg)|*.jpg||"));
    if (dlg.DoModal() == IDOK)
        m_edPath.SetWindowText(dlg.GetPathName());
}

void CExportDialog::OnBtnSave()
{
    CString path;
    m_edPath.GetWindowText(path);
    path.Trim();
    if (path.IsEmpty())
    {
        AfxMessageBox(_T("저장 경로를 입력하세요."));
        return;
    }

    // 부모 폴더 자동 생성 (없으면 GDI+ Bitmap.Save가 실패)
    int slash = path.ReverseFind(_T('\\'));
    if (slash > 0)
    {
        CString dir = path.Left(slash);
        if (GetFileAttributes(dir) == INVALID_FILE_ATTRIBUTES)
        {
            // 재귀 mkdir (간이) — SHCreateDirectoryEx 사용
            int rc = SHCreateDirectoryEx(nullptr, dir, nullptr);
            if (rc != ERROR_SUCCESS && rc != ERROR_FILE_EXISTS && rc != ERROR_ALREADY_EXISTS)
            {
                CString msg;
                msg.Format(_T("저장 폴더를 만들 수 없습니다 (오류 %d).\n%s"), rc, dir.GetString());
                AfxMessageBox(msg, MB_OK | MB_ICONERROR);
                return;
            }
        }
    }

    int resSel = m_cmbResolution.GetCurSel();
    int w = 540, h = 360;
    if (resSel == 1) { w = 1080; h = 720; }
    else if (resSel == 2) { w = 2160; h = 1440; }

    CRect client(0, 0, w, h);
    deepmetria::ChartRenderer renderer;

    // 선택중인 그래프만 vs 대시보드 전체
    bool onlySelected = (IsDlgButtonChecked(IDC_EXPORT_RADIO_SINGLE) == BST_CHECKED);
    bool ok = false;

    if (onlySelected && m_selectedVizIdx >= 0 &&
        m_selectedVizIdx < (int)m_dash.Visualizations().size())
    {
        // 임시 Dashboard에 선택 viz만 담아 export (원본은 const)
        deepmetria::Dashboard tmpDash;
        auto viz = m_dash.Visualizations()[m_selectedVizIdx];
        viz.x = 16; viz.y = 16;  // 좌상단에 배치
        viz.width  = w - 32;
        viz.height = h - 32;
        tmpDash.Add(viz);
        ok = renderer.ExportDashboardToImage(client, m_ds, tmpDash,
                                             std::wstring(path.GetString()));
    }
    else
    {
        ok = renderer.ExportDashboardToImage(client, m_ds, m_dash,
                                             std::wstring(path.GetString()));
    }

    if (ok)
    {
        CString msg;
        msg.Format(_T("내보내기 완료!\n%s"), path.GetString());
        AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
        CDialogEx::OnOK();
    }
    else
    {
        CString msg;
        msg.Format(
            _T("내보내기에 실패했습니다.\n\n경로: %s\n\n")
            _T("• 폴더 쓰기 권한이 있는지 확인하세요\n")
            _T("• 확장자가 .png / .bmp / .jpg 인지 확인하세요\n")
            _T("• 다른 프로그램에서 파일을 열고 있지 않은지 확인하세요"),
            path.GetString());
        AfxMessageBox(msg, MB_OK | MB_ICONERROR);
    }
}
