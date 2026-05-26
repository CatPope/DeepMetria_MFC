#include "stdafx.h"
#include "ExportDialog.h"
#include "../Renderer/ChartRenderer.h"

// GDI+ (PNG/BMP 내보내기)
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include <sstream>

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
    if (width <= 0 || height <= 0)
        return FALSE;

    CChartRenderer::RenderToFile(path, width, height, m_vizInfo.chartConfig);
    return (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES);
}

BOOL CExportDialog::ExportAsBmp(const CString& path, int width, int height)
{
    if (width <= 0 || height <= 0)
        return FALSE;

    CChartRenderer::RenderToFile(path, width, height, m_vizInfo.chartConfig);
    return (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES);
}

BOOL CExportDialog::ExportAsCsv(const CString& path)
{
    CStdioFile file;
    if (!file.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
        return FALSE;

    const CString& dataJson = m_vizInfo.chartConfig.dataJson;
    if (dataJson.IsEmpty())
    {
        file.Close();
        return FALSE;
    }

    CT2A utf8Json(dataJson, CP_UTF8);
    std::string json(utf8Json);

    bool isTableFormat = (json.find("\"columns\"") != std::string::npos &&
                          json.find("\"rows\"") != std::string::npos);
    bool isChartFormat = (json.find("\"labels\"") != std::string::npos &&
                          json.find("\"datasets\"") != std::string::npos);

    if (isTableFormat)
    {
        // columns/rows 형식 → CSV
        size_t colStart = json.find("\"columns\"");
        size_t arrStart = json.find('[', colStart);
        size_t arrEnd   = json.find(']', arrStart);
        std::string colArr = json.substr(arrStart + 1, arrEnd - arrStart - 1);

        std::vector<CString> columns;
        size_t pos = 0;
        while ((pos = colArr.find('"', pos)) != std::string::npos)
        {
            size_t end = colArr.find('"', pos + 1);
            if (end == std::string::npos) break;
            std::string col = colArr.substr(pos + 1, end - pos - 1);
            int wlen = MultiByteToWideChar(CP_UTF8, 0, col.c_str(), -1, nullptr, 0);
            CString cs;
            MultiByteToWideChar(CP_UTF8, 0, col.c_str(), -1, cs.GetBufferSetLength(wlen), wlen);
            cs.ReleaseBuffer();
            columns.push_back(cs);
            pos = end + 1;
        }

        CString headerLine;
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (i > 0) headerLine += _T(",");
            headerLine += columns[i];
        }
        file.WriteString(headerLine + _T("\n"));

        size_t rowsStart    = json.find("\"rows\"");
        size_t rowsArrStart = json.find('[', rowsStart);
        size_t searchPos    = rowsArrStart + 1;
        while ((searchPos = json.find('[', searchPos)) != std::string::npos)
        {
            size_t rowEnd = json.find(']', searchPos);
            if (rowEnd == std::string::npos) break;
            std::string rowStr = json.substr(searchPos + 1, rowEnd - searchPos - 1);

            CString rowLine;
            size_t vpos = 0;
            bool first = true;
            while ((vpos = rowStr.find('"', vpos)) != std::string::npos)
            {
                size_t vend = rowStr.find('"', vpos + 1);
                if (vend == std::string::npos) break;
                std::string val = rowStr.substr(vpos + 1, vend - vpos - 1);
                int wlen = MultiByteToWideChar(CP_UTF8, 0, val.c_str(), -1, nullptr, 0);
                CString cs;
                MultiByteToWideChar(CP_UTF8, 0, val.c_str(), -1, cs.GetBufferSetLength(wlen), wlen);
                cs.ReleaseBuffer();
                if (!first) rowLine += _T(",");
                rowLine += cs;
                first = false;
                vpos = vend + 1;
            }
            file.WriteString(rowLine + _T("\n"));
            searchPos = rowEnd + 1;
        }
    }
    else if (isChartFormat)
    {
        // labels/datasets 형식 → CSV
        size_t labStart    = json.find("\"labels\"");
        size_t labArrStart = json.find('[', labStart);
        size_t labArrEnd   = json.find(']', labArrStart);
        std::string labArr = json.substr(labArrStart + 1, labArrEnd - labArrStart - 1);

        std::vector<std::string> labels;
        size_t lp = 0;
        while ((lp = labArr.find('"', lp)) != std::string::npos)
        {
            size_t le = labArr.find('"', lp + 1);
            if (le == std::string::npos) break;
            labels.push_back(labArr.substr(lp + 1, le - lp - 1));
            lp = le + 1;
        }

        std::vector<std::string> dsNames;
        std::vector<std::vector<std::string>> dsValues;

        size_t dsStart     = json.find("\"datasets\"");
        size_t dsArrStart  = json.find('[', dsStart);
        size_t dsSearchPos = dsArrStart + 1;

        while (true)
        {
            size_t namePos = json.find("\"name\"", dsSearchPos);
            if (namePos == std::string::npos) break;

            size_t ns = json.find('"', namePos + 6);
            size_t ne = json.find('"', ns + 1);
            dsNames.push_back(json.substr(ns + 1, ne - ns - 1));

            size_t valPos  = json.find("\"values\"", ne);
            size_t valArrS = json.find('[', valPos);
            size_t valArrE = json.find(']', valArrS);
            std::string valStr = json.substr(valArrS + 1, valArrE - valArrS - 1);

            std::vector<std::string> vals;
            std::istringstream vss(valStr);
            std::string token;
            while (std::getline(vss, token, ','))
            {
                size_t s  = token.find_first_not_of(" \t\r\n");
                size_t e2 = token.find_last_not_of(" \t\r\n");
                if (s != std::string::npos)
                    vals.push_back(token.substr(s, e2 - s + 1));
            }
            dsValues.push_back(vals);
            dsSearchPos = valArrE + 1;
        }

        // 헤더 행
        CString headerLine;
        for (size_t i = 0; i < dsNames.size(); ++i)
        {
            headerLine += _T(",");
            int wl = MultiByteToWideChar(CP_UTF8, 0, dsNames[i].c_str(), -1, nullptr, 0);
            CString nm;
            MultiByteToWideChar(CP_UTF8, 0, dsNames[i].c_str(), -1, nm.GetBufferSetLength(wl), wl);
            nm.ReleaseBuffer();
            headerLine += nm;
        }
        file.WriteString(headerLine + _T("\n"));

        // 데이터 행
        for (size_t r = 0; r < labels.size(); ++r)
        {
            int wl = MultiByteToWideChar(CP_UTF8, 0, labels[r].c_str(), -1, nullptr, 0);
            CString lb;
            MultiByteToWideChar(CP_UTF8, 0, labels[r].c_str(), -1, lb.GetBufferSetLength(wl), wl);
            lb.ReleaseBuffer();
            CString line = lb;
            for (size_t d = 0; d < dsValues.size(); ++d)
            {
                line += _T(",");
                if (r < dsValues[d].size())
                {
                    int wl2 = MultiByteToWideChar(CP_UTF8, 0, dsValues[d][r].c_str(), -1, nullptr, 0);
                    CString v;
                    MultiByteToWideChar(CP_UTF8, 0, dsValues[d][r].c_str(), -1, v.GetBufferSetLength(wl2), wl2);
                    v.ReleaseBuffer();
                    line += v;
                }
            }
            file.WriteString(line + _T("\n"));
        }
    }
    else
    {
        // 알 수 없는 형식 — 제목 + raw data 출력
        file.WriteString(m_vizInfo.title + _T("\n"));
        file.WriteString(dataJson + _T("\n"));
    }

    file.Close();
    return TRUE;
}

CString CExportDialog::GetSelectedFormat() const
{
    CString fmt;
    const_cast<CComboBox&>(m_comboFormat).GetWindowText(fmt);
    return fmt;
}
