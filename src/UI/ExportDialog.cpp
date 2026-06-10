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
    m_cmbResolution.AddString(_T("1x (원본 크기)"));
    m_cmbResolution.AddString(_T("2x 고해상도"));
    m_cmbResolution.AddString(_T("4x 초고해상도"));
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

    // 해상도 옵션은 scale 배수로 동작 (1x / 2x / 4x)
    int resSel = m_cmbResolution.GetCurSel();
    int scale = 1;
    if (resSel == 1) scale = 2;
    else if (resSel == 2) scale = 4;

    deepmetria::ChartRenderer renderer;

    // 선택중인 그래프만 vs 대시보드 전체
    bool onlySelected = (IsDlgButtonChecked(IDC_EXPORT_RADIO_SINGLE) == BST_CHECKED);
    bool ok = false;

    if (onlySelected && m_selectedVizIdx >= 0 &&
        m_selectedVizIdx < (int)m_dash.Visualizations().size())
    {
        // 선택 viz만 — 좌상단(16,16) 기준 캔버스에 맞춤
        const int baseW = 720, baseH = 480;
        int outW = baseW * scale, outH = baseH * scale;

        deepmetria::Dashboard tmpDash;
        auto viz = m_dash.Visualizations()[m_selectedVizIdx];
        viz.x = 16 * scale;
        viz.y = 16 * scale;
        viz.width  = outW - 32 * scale;
        viz.height = outH - 32 * scale;
        tmpDash.Add(viz);

        CRect client(0, 0, outW, outH);
        ok = renderer.ExportDashboardToImage(client, m_ds, tmpDash,
                                             std::wstring(path.GetString()));
    }
    else
    {
        // 대시보드 전체 — viz 들의 bounding box를 계산해 캔버스 사이즈 자동 결정
        const auto& vizs = m_dash.Visualizations();
        if (vizs.empty())
        {
            AfxMessageBox(_T("내보낼 시각화가 없습니다.\n먼저 분석으로 시각화를 만들어 주세요."),
                          MB_OK | MB_ICONWARNING);
            return;
        }
        int maxX = 0, maxY = 0;
        for (const auto& v : vizs)
        {
            if (v.x + v.width  > maxX) maxX = v.x + v.width;
            if (v.y + v.height > maxY) maxY = v.y + v.height;
        }
        maxX += 16;  // 우측 패딩
        maxY += 16;  // 하단 패딩

        // 임시 dashboard 에 scale 배수 적용한 사본 담기
        deepmetria::Dashboard tmpDash;
        for (const auto& v : vizs)
        {
            auto sv = v;
            sv.x *= scale; sv.y *= scale;
            sv.width *= scale; sv.height *= scale;
            tmpDash.Add(sv);
        }

        CRect client(0, 0, maxX * scale, maxY * scale);
        ok = renderer.ExportDashboardToImage(client, m_ds, tmpDash,
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
