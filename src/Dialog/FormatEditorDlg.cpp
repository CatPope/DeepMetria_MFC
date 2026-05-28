#include "stdafx.h"
#include "FormatEditorDlg.h"

// ============================================================
// IMPLEMENT_DYNAMIC / 메시지 맵
// ============================================================
IMPLEMENT_DYNAMIC(CFormatEditorDlg, CDialogEx)

BEGIN_MESSAGE_MAP(CFormatEditorDlg, CDialogEx)
    ON_WM_HSCROLL()
    ON_BN_CLICKED(IDC_BTN_COLOR,        &CFormatEditorDlg::OnBnClickedColor)
    ON_BN_CLICKED(IDC_BTN_APPLY,        &CFormatEditorDlg::OnBnClickedApply)
    ON_BN_CLICKED(IDC_BTN_FMT_CANCEL,   &CFormatEditorDlg::OnCancel)
    ON_CBN_SELCHANGE(IDC_COMBO_CHART_TYPE, &CFormatEditorDlg::OnCbnSelchangeChartType)
END_MESSAGE_MAP()

// ============================================================
// 생성 / 소멸
// ============================================================
CFormatEditorDlg::CFormatEditorDlg(const VisualizationInfo& vizInfo, CWnd* pParent)
    : CDialogEx(CFormatEditorDlg::IDD, pParent)
    , m_vizInfo(vizInfo)
    , m_currentColor(RGB(65, 105, 225))
{
    // 기존 style 색상 파싱 (hex "#RRGGBB")
    CString hex = vizInfo.style.primaryColor;
    if (hex.GetLength() == 7 && hex[0] == _T('#'))
    {
        int r = _tcstol(hex.Mid(1, 2), nullptr, 16);
        int g = _tcstol(hex.Mid(3, 2), nullptr, 16);
        int b = _tcstol(hex.Mid(5, 2), nullptr, 16);
        m_currentColor = RGB(r, g, b);
    }
}

CFormatEditorDlg::~CFormatEditorDlg()
{
}

// ============================================================
// DDX
// ============================================================
void CFormatEditorDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SLIDER_WIDTH,         m_sliderWidth);
    DDX_Control(pDX, IDC_SLIDER_HEIGHT,        m_sliderHeight);
    DDX_Control(pDX, IDC_EDIT_FMT_WIDTH,       m_editWidth);
    DDX_Control(pDX, IDC_EDIT_FMT_HEIGHT,      m_editHeight);
    DDX_Control(pDX, IDC_BTN_COLOR,            m_btnColor);
    DDX_Control(pDX, IDC_STATIC_COLOR_PREVIEW, m_staticColorPreview);
    DDX_Control(pDX, IDC_COMBO_CHART_TYPE,     m_comboChartType);
    DDX_Control(pDX, IDC_STATIC_PREVIEW,       m_staticPreview);
}

// ============================================================
// 초기화
// ============================================================
BOOL CFormatEditorDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 슬라이더 범위 설정 (100 ~ 2000)
    m_sliderWidth.SetRange(100, 2000);
    m_sliderHeight.SetRange(100, 2000);

    // 현재 viz 크기로 슬라이더 초기값 설정
    int initW = (m_vizInfo.position.w > 0) ? m_vizInfo.position.w * 100 : 400;
    int initH = (m_vizInfo.position.h > 0) ? m_vizInfo.position.h * 100 : 300;
    initW = max(100, min(initW, 2000));
    initH = max(100, min(initH, 2000));

    m_sliderWidth.SetPos(initW);
    m_sliderHeight.SetPos(initH);

    CString strW, strH;
    strW.Format(_T("%d"), initW);
    strH.Format(_T("%d"), initH);
    m_editWidth.SetWindowText(strW);
    m_editHeight.SetWindowText(strH);

    // 차트 타입 콤보 채우기
    m_comboChartType.AddString(_T("LINE"));
    m_comboChartType.AddString(_T("BAR"));
    m_comboChartType.AddString(_T("SCATTER"));
    m_comboChartType.AddString(_T("PIE"));
    m_comboChartType.AddString(_T("TABLE"));

    // 현재 차트 타입 선택
    CString curType = m_vizInfo.vizType;
    curType.MakeUpper();
    // "bar_chart" 형식 처리 — 첫 단어만 추출
    int underscorePos = curType.Find(_T('_'));
    if (underscorePos >= 0)
        curType = curType.Left(underscorePos);

    int selIdx = m_comboChartType.FindStringExact(-1, curType);
    m_comboChartType.SetCurSel(selIdx >= 0 ? selIdx : 0);

    // 색상 프리뷰 초기화
    UpdatePreview();

    return TRUE;
}

// ============================================================
// 슬라이더 변경 — Edit 컨트롤 + m_vizInfo 갱신
// ============================================================
void CFormatEditorDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
    UpdateSizeFromSliders();
}

void CFormatEditorDlg::UpdateSizeFromSliders()
{
    int w = m_sliderWidth.GetPos();
    int h = m_sliderHeight.GetPos();

    CString strW, strH;
    strW.Format(_T("%d"), w);
    strH.Format(_T("%d"), h);
    m_editWidth.SetWindowText(strW);
    m_editHeight.SetWindowText(strH);

    // 그리드 단위로 환산 (100px = 1 단위)
    m_vizInfo.position.w = max(1, w / 100);
    m_vizInfo.position.h = max(1, h / 100);

    UpdatePreview();
}

// ============================================================
// 색상 선택
// ============================================================
void CFormatEditorDlg::OnBnClickedColor()
{
    CColorDialog dlg(m_currentColor, CC_FULLOPEN, this);
    if (dlg.DoModal() == IDOK)
    {
        m_currentColor = dlg.GetColor();

        // hex 문자열로 변환하여 style에 저장
        CString hexColor;
        hexColor.Format(_T("#%02X%02X%02X"),
            GetRValue(m_currentColor),
            GetGValue(m_currentColor),
            GetBValue(m_currentColor));
        m_vizInfo.style.primaryColor = hexColor;

        UpdatePreview();
    }
}

// ============================================================
// 적용 — m_vizInfo 확정 후 OK 종료
// ============================================================
void CFormatEditorDlg::OnBnClickedApply()
{
    // Edit 컨트롤 값을 최종 반영 (슬라이더 대신 직접 입력한 경우 대비)
    CString strW, strH;
    m_editWidth.GetWindowText(strW);
    m_editHeight.GetWindowText(strH);

    int w = _ttoi(strW);
    int h = _ttoi(strH);
    if (w >= 100 && w <= 2000)
        m_vizInfo.position.w = max(1, w / 100);
    if (h >= 100 && h <= 2000)
        m_vizInfo.position.h = max(1, h / 100);

    OnOK();
}

// ============================================================
// 차트 타입 변경
// ============================================================
void CFormatEditorDlg::OnCbnSelchangeChartType()
{
    CString selType;
    m_comboChartType.GetWindowText(selType);
    selType.MakeLower();
    m_vizInfo.vizType        = selType;
    m_vizInfo.chartConfig.chartType = selType;

    UpdatePreview();
}

// ============================================================
// 미리보기 갱신 — Static 배경색으로 색상 표시
// ============================================================
void CFormatEditorDlg::UpdatePreview()
{
    // 색상 프리뷰 static 갱신 — 강제 재그리기
    m_staticColorPreview.Invalidate();
    m_staticColorPreview.UpdateWindow();

    // 미리보기 static 갱신
    m_staticPreview.Invalidate();
    m_staticPreview.UpdateWindow();
}
