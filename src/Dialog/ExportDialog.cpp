#include "stdafx.h"
#include "ExportDialog.h"
#include "../Renderer/ChartRenderer.h"

// GDI+ (PNG/BMP 내보내기)
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// PDF 내보내기 (벤더링된 공용 도메인 라이브러리)
#include "../Infrastructure/Export/pdfgen.h"

// CSV/HTML 포맷 순수 문자열 빌더 (헤더 전용, ATL 전용 — 단위 테스트 대상)
#include "../Infrastructure/Export/ExportFormatter.h"

#include <wincrypt.h>          // CryptBinaryToString (HTML base64 data URI)
#pragma comment(lib, "crypt32.lib")

#include <sstream>
#include <vector>

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
    m_comboFormat.AddString(_T("HTML"));
    m_comboFormat.AddString(_T("PDF"));
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
    // PNG/BMP뿐 아니라 HTML/PDF도 차트 이미지를 포함하므로 크기 컨트롤 사용
    BOOL bImage = (fmt == _T("PNG") || fmt == _T("BMP") ||
                   fmt == _T("HTML") || fmt == _T("PDF"));
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
    else if (fmt == _T("HTML"))
        bOk = ExportAsHtml(path, width, height);
    else if (fmt == _T("PDF"))
        bOk = ExportAsPdf(path, width, height);

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

    // 알 수 없는 형식일 때만 제목 줄을 추가로 기록한다(기존 동작 유지).
    // 포맷 빌더는 순수 문자열 빌딩만 담당한다.
    CT2A utf8Json(dataJson, CP_UTF8);
    std::string json(static_cast<const char*>(utf8Json));
    bool isTableFormat = (json.find("\"columns\"") != std::string::npos &&
                          json.find("\"rows\"") != std::string::npos);
    bool isChartFormat = (json.find("\"labels\"") != std::string::npos &&
                          json.find("\"datasets\"") != std::string::npos);
    if (!isTableFormat && !isChartFormat)
        file.WriteString(m_vizInfo.title + _T("\n"));

    file.WriteString(ExportFormatter::BuildCsvFromDataJson(dataJson));

    file.Close();
    return TRUE;
}

// ============================================================
// 임시 PNG 렌더링 (HTML/PDF 내보내기용 공통 헬퍼)
// ============================================================
CString CExportDialog::RenderChartToTempPng(int width, int height) const
{
    if (width <= 0 || height <= 0)
        return CString();

    TCHAR tempDir[MAX_PATH] = {};
    if (::GetTempPath(MAX_PATH, tempDir) == 0)
        return CString();

    TCHAR tempFile[MAX_PATH] = {};
    if (::GetTempFileName(tempDir, _T("dmx"), 0, tempFile) == 0)
        return CString();

    // GetTempFileName은 .tmp 파일을 만든다 → .png 확장자로 교체
    CString pngPath(tempFile);
    ::DeleteFile(pngPath);
    int dot = pngPath.ReverseFind(_T('.'));
    if (dot >= 0)
        pngPath = pngPath.Left(dot);
    pngPath += _T(".png");

    CChartRenderer::RenderToFile(pngPath, width, height, m_vizInfo.chartConfig);
    if (::GetFileAttributes(pngPath) == INVALID_FILE_ATTRIBUTES)
        return CString();

    return pngPath;
}

// ============================================================
// dataJson → HTML <table> 변환
// columns/rows 형식과 labels/datasets 형식을 모두 지원
// ============================================================
CString CExportDialog::BuildHtmlTable() const
{
    // 순수 문자열 빌딩은 ExportFormatter(헤더 전용, 단위 테스트 대상)에 위임한다.
    return ExportFormatter::BuildHtmlTableFromDataJson(m_vizInfo.chartConfig.dataJson);
}

// ============================================================
// HTML 내보내기 — 차트 이미지(base64 data URI) + 데이터 표를
// 담은 자체완결형 .html 파일 생성
// ============================================================
BOOL CExportDialog::ExportAsHtml(const CString& path, int width, int height)
{
    if (width <= 0 || height <= 0)
        return FALSE;

    // 1) 차트를 임시 PNG로 렌더링
    CString tempPng = RenderChartToTempPng(width, height);
    if (tempPng.IsEmpty())
        return FALSE;

    // 2) PNG 바이트 읽기
    CFile imgFile;
    if (!imgFile.Open(tempPng, CFile::modeRead | CFile::typeBinary))
    {
        ::DeleteFile(tempPng);
        return FALSE;
    }
    ULONGLONG len64 = imgFile.GetLength();
    DWORD pngLen = static_cast<DWORD>(len64);
    std::vector<BYTE> pngBytes(pngLen);
    if (pngLen > 0)
        imgFile.Read(pngBytes.data(), pngLen);
    imgFile.Close();
    ::DeleteFile(tempPng);

    if (pngLen == 0)
        return FALSE;

    // 3) base64 인코딩 (data URI용)
    DWORD b64Len = 0;
    ::CryptBinaryToString(pngBytes.data(), pngLen,
                          CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                          nullptr, &b64Len);
    if (b64Len == 0)
        return FALSE;

    CString base64;
    ::CryptBinaryToString(pngBytes.data(), pngLen,
                          CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                          base64.GetBuffer(b64Len), &b64Len);
    base64.ReleaseBuffer();

    // 4) HTML 본문 구성 (UTF-8로 저장)
    CString title = m_vizInfo.title;
    title.Replace(_T("<"), _T("&lt;"));
    title.Replace(_T(">"), _T("&gt;"));

    CString htmlTable = BuildHtmlTable();

    CString html;
    html += _T("<!DOCTYPE html>\n");
    html += _T("<html lang=\"ko\">\n<head>\n");
    html += _T("  <meta charset=\"UTF-8\">\n");
    html += _T("  <title>") + title + _T("</title>\n");
    html += _T("  <style>\n");
    html += _T("    body { font-family: 'Malgun Gothic', sans-serif; margin: 24px; color: #222; }\n");
    html += _T("    h1 { font-size: 20px; }\n");
    html += _T("    img { max-width: 100%; height: auto; border: 1px solid #ddd; }\n");
    html += _T("    table { border-collapse: collapse; margin-top: 16px; }\n");
    html += _T("    th, td { border: 1px solid #ccc; padding: 4px 10px; text-align: left; }\n");
    html += _T("    th { background: #f2f2f2; }\n");
    html += _T("  </style>\n</head>\n<body>\n");
    html += _T("  <h1>") + title + _T("</h1>\n");
    html += _T("  <img src=\"data:image/png;base64,") + base64 + _T("\" alt=\"chart\">\n");
    html += htmlTable;
    html += _T("</body>\n</html>\n");

    // 5) UTF-8 파일로 저장
    CFile out;
    if (!out.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
        return FALSE;

    CT2A utf8Html(html, CP_UTF8);
    int byteLen = static_cast<int>(strlen(static_cast<const char*>(utf8Html)));

    // UTF-8 BOM
    static const BYTE bom[3] = { 0xEF, 0xBB, 0xBF };
    out.Write(bom, 3);
    out.Write(static_cast<const char*>(utf8Html), byteLen);
    out.Close();

    return (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES);
}

// ============================================================
// PDF 내보내기 — 차트 이미지를 임베드한 단일 페이지 PDF 생성
// (벤더링된 공용 도메인 PDFGen 라이브러리 사용)
//
// 한글 제한: PDFGen 코어 폰트(Helvetica 등)는 라틴 문자만 지원하므로
// 제목은 ASCII("DeepMetria Chart Export")로 표기한다. 한글 제목은
// 깨지므로 사용하지 않으며, 한글 데이터는 차트 이미지로만 표현된다.
// ============================================================
BOOL CExportDialog::ExportAsPdf(const CString& path, int width, int height)
{
    if (width <= 0 || height <= 0)
        return FALSE;

    // 1) 차트를 임시 PNG로 렌더링
    CString tempPng = RenderChartToTempPng(width, height);
    if (tempPng.IsEmpty())
        return FALSE;

    // PDFGen API는 char* (ANSI/UTF-8) 경로를 받는다 → 변환
    CT2A pngPathA(tempPng);
    CT2A outPathA(path);

    // 2) PDF 문서 생성 (A4)
    struct pdf_info info;
    memset(&info, 0, sizeof(info));
    strncpy_s(info.creator,  sizeof(info.creator),  "DeepMetria",        _TRUNCATE);
    strncpy_s(info.producer, sizeof(info.producer), "DeepMetria PDFGen", _TRUNCATE);
    strncpy_s(info.title,    sizeof(info.title),    "DeepMetria Chart",  _TRUNCATE);

    struct pdf_doc* pdf = pdf_create(PDF_A4_WIDTH, PDF_A4_HEIGHT, &info);
    if (!pdf)
    {
        ::DeleteFile(tempPng);
        return FALSE;
    }

    pdf_set_font(pdf, "Helvetica-Bold");
    pdf_append_page(pdf);

    float pageW = pdf_width(pdf);
    float pageH = pdf_height(pdf);

    // 3) 제목 (ASCII만 — 한글 폰트 미지원)
    pdf_add_text(pdf, NULL, "DeepMetria Chart Export", 16.0f,
                 40.0f, pageH - 50.0f, PDF_BLACK);

    // 4) 차트 이미지 임베드 — 페이지 폭에 맞춰 비율 유지(높이 음수)
    float margin   = 40.0f;
    float imgWidth = pageW - margin * 2.0f;
    float imgY     = pageH - 70.0f - (imgWidth * (float)height / (float)width);
    if (imgY < margin)
        imgY = margin;

    pdf_add_image_file(pdf, NULL, margin, imgY, imgWidth, -1.0f,
                       static_cast<const char*>(pngPathA));

    // 5) 저장 및 정리
    int rc = pdf_save(pdf, static_cast<const char*>(outPathA));
    pdf_destroy(pdf);
    ::DeleteFile(tempPng);

    if (rc < 0)
        return FALSE;

    return (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES);
}

CString CExportDialog::GetSelectedFormat() const
{
    CString fmt;
    const_cast<CComboBox&>(m_comboFormat).GetWindowText(fmt);
    return fmt;
}
