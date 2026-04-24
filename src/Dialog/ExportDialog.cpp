#include "../stdafx.h"
#include "ExportDialog.h"

// GDI+ (PNG/BMP 내보내기)
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// ============================================================
// IMPLEMENT_DYNAMIC / 메시지 맵
// ============================================================
IMPLEMENT_DYNAMIC(CExportDialog, CDialogEx)

BEGIN_MESSAGE_MAP(CExportDialog, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BROWSE,       &CExportDialog::OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_BTN_EXPORT,       &CExportDialog::OnBnClickedExport)
    ON_CBN_SELCHANGE(IDC_COMBO_FORMAT,  &CExportDialog::OnCbnSelchangeFormat)
END_MESSAGE_MAP()

// ============================================================
// 생성 / 소멸
// ============================================================
CExportDialog::CExportDialog(const VisualizationInfo& vizInfo, CWnd* pParent)
    : CDialogEx(CExportDialog::IDD, pParent)
    , m_vizInfo(vizInfo)
{
}

CExportDialog::~CExportDialog()
{
}

// ============================================================
// DDX
// ============================================================
void CExportDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_FORMAT,  m_comboFormat);
    DDX_Control(pDX, IDC_EDIT_PATH,     m_editPath);
    DDX_Control(pDX, IDC_BTN_BROWSE,    m_btnBrowse);
    DDX_Control(pDX, IDC_SPIN_WIDTH,    m_spinWidth);
    DDX_Control(pDX, IDC_SPIN_HEIGHT,   m_spinHeight);
    DDX_Control(pDX, IDC_EDIT_WIDTH,    m_editWidth);
    DDX_Control(pDX, IDC_EDIT_HEIGHT,   m_editHeight);
}

// ============================================================
// 초기화
// ============================================================
BOOL CExportDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 형식 목록
    m_comboFormat.AddString(_T("PNG"));
    m_comboFormat.AddString(_T("BMP"));
    m_comboFormat.AddString(_T("CSV"));
    m_comboFormat.SetCurSel(0);

    // 기본 이미지 크기
    m_spinWidth.SetRange(100, 4096);
    m_spinWidth.SetPos(1280);
    m_spinHeight.SetRange(100, 4096);
    m_spinHeight.SetPos(720);

    m_editWidth.SetWindowText(_T("1280"));
    m_editHeight.SetWindowText(_T("720"));

    // 기본 저장 경로 (바탕화면)
    TCHAR desktopPath[MAX_PATH] = {};
    ::SHGetFolderPath(nullptr, CSIDL_DESKTOP, nullptr, 0, desktopPath);
    CString defaultPath;
    defaultPath.Format(_T("%s\\%s.png"), desktopPath, (LPCTSTR)m_vizInfo.title);
    m_editPath.SetWindowText(defaultPath);

    UpdateSizeControls();
    return TRUE;
}

// ============================================================
// 형식 변경
// ============================================================
void CExportDialog::OnCbnSelchangeFormat()
{
    UpdateSizeControls();

    // 확장자 자동 변경
    CString path, fmt = GetSelectedFormat();
    m_editPath.GetWindowText(path);
    int dot = path.ReverseFind(_T('.'));
    if (dot >= 0)
    {
        path = path.Left(dot + 1);
        path += fmt.MakeLower();
        m_editPath.SetWindowText(path);
    }
}

void CExportDialog::UpdateSizeControls()
{
    CString fmt = GetSelectedFormat();
    BOOL bImage = (fmt == _T("PNG") || fmt == _T("BMP"));
    m_spinWidth.EnableWindow(bImage);
    m_spinHeight.EnableWindow(bImage);
    m_editWidth.EnableWindow(bImage);
    m_editHeight.EnableWindow(bImage);
}

// ============================================================
// 폴더/파일 선택
// ============================================================
void CExportDialog::OnBnClickedBrowse()
{
    CString fmt    = GetSelectedFormat();
    CString filter = fmt + _T(" 파일 (*.") + fmt.MakeLower() + _T(")|*.") +
                     fmt.MakeLower() + _T("||");

    CFileDialog dlg(FALSE, fmt.MakeLower(), m_vizInfo.title,
                    OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
                    filter, this);
    if (dlg.DoModal() == IDOK)
        m_editPath.SetWindowText(dlg.GetPathName());
}

// ============================================================
// 내보내기
// ============================================================
void CExportDialog::OnBnClickedExport()
{
    CString path, fmt = GetSelectedFormat();
    m_editPath.GetWindowText(path);
    path.Trim();

    if (path.IsEmpty())
    {
        AfxMessageBox(_T("저장 경로를 입력하세요."), MB_ICONWARNING);
        return;
    }

    int width = 0, height = 0;
    CString wStr, hStr;
    m_editWidth.GetWindowText(wStr);
    m_editHeight.GetWindowText(hStr);
    width  = _ttoi(wStr);
    height = _ttoi(hStr);

    BOOL bOk = FALSE;
    if (fmt == _T("PNG"))
        bOk = ExportAsPng(path, width, height);
    else if (fmt == _T("BMP"))
        bOk = ExportAsBmp(path, width, height);
    else if (fmt == _T("CSV"))
        bOk = ExportAsCsv(path);

    if (bOk)
    {
        AfxMessageBox(_T("내보내기가 완료되었습니다."), MB_ICONINFORMATION);
        OnOK();
    }
    else
    {
        AfxMessageBox(_T("내보내기에 실패했습니다."), MB_ICONERROR);
    }
}

// ============================================================
// 내보내기 구현
// ============================================================
BOOL CExportDialog::ExportAsPng(const CString& path, int width, int height)
{
    // GDI+ Bitmap에 ChartRenderer로 렌더링 후 PNG 저장
    // ChartRenderer 구현 완료 후 활성화
    // Gdiplus::Bitmap bmp(width, height, PixelFormat32bppARGB);
    // ...
    // bmp.Save(path, &pngClsid);

    TRACE(_T("ExportDialog: PNG 내보내기 — %s (%dx%d)\n"),
          (LPCTSTR)path, width, height);
    return TRUE; // 임시 성공 반환
}

BOOL CExportDialog::ExportAsBmp(const CString& path, int width, int height)
{
    TRACE(_T("ExportDialog: BMP 내보내기 — %s (%dx%d)\n"),
          (LPCTSTR)path, width, height);
    return TRUE; // 임시 성공 반환
}

BOOL CExportDialog::ExportAsCsv(const CString& path)
{
    // 차트 데이터(JSON)를 CSV로 변환
    TRACE(_T("ExportDialog: CSV 내보내기 — %s\n"), (LPCTSTR)path);

    CStdioFile file;
    if (!file.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
        return FALSE;

    // 임시: 제목과 데이터 JSON을 그대로 저장
    file.WriteString(m_vizInfo.title + _T("\n"));
    file.WriteString(m_vizInfo.chartConfig.dataJson + _T("\n"));
    file.Close();
    return TRUE;
}

CString CExportDialog::GetSelectedFormat() const
{
    CString fmt;
    const_cast<CComboBox&>(m_comboFormat).GetWindowText(fmt);
    return fmt;
}
