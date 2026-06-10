// FormatPane.cpp - 우측 서식 편집 패널 구현 (CWnd 직접 child)
#include "stdafx.h"
#include "FormatPane.h"
#include "Messages.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CFormatPane, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_BN_CLICKED(3001, &CFormatPane::OnBtnApply)
    ON_BN_CLICKED(3002, &CFormatPane::OnBtnReset)
    // 차트 유형 콤보 변경 시 즉시 [적용]
    ON_CBN_SELCHANGE(3016, &CFormatPane::OnBtnApply)
    // 체크박스 토글 시 즉시 [적용]
    ON_BN_CLICKED(3017, &CFormatPane::OnBtnApply)
    ON_BN_CLICKED(3018, &CFormatPane::OnBtnApply)
    // 색상 버튼
    ON_BN_CLICKED(3019, &CFormatPane::OnBtnPickColor)
END_MESSAGE_MAP()

CFormatPane::CFormatPane() = default;
CFormatPane::~CFormatPane() = default;

BOOL CFormatPane::Create(CWnd* pParent, UINT id)
{
    return CWnd::Create(
        nullptr, _T(""),
        WS_CHILD,
        CRect(0, 0, 1, 1),
        pParent, id);
}

int CFormatPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 크기 헤더
    m_lblSize.Create(_T("크기 (px)"), WS_CHILD | WS_VISIBLE | SS_LEFT,
                     CRect(0,0,0,0), this);

    m_edWidth.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                     CRect(0,0,0,0), this, 3010);
    m_edWidth.SetWindowText(_T("360"));

    m_edHeight.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                      CRect(0,0,0,0), this, 3011);
    m_edHeight.SetWindowText(_T("230"));

    m_spinWidth.Create(WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_SETBUDDYINT,
                       CRect(0,0,0,0), this, 3012);
    m_spinWidth.SetBuddy(&m_edWidth);
    m_spinWidth.SetRange(80, 2000);

    m_spinHeight.Create(WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_SETBUDDYINT,
                        CRect(0,0,0,0), this, 3013);
    m_spinHeight.SetBuddy(&m_edHeight);
    m_spinHeight.SetRange(60, 2000);

    // 위치 헤더
    m_lblPos.Create(_T("위치 (px)"), WS_CHILD | WS_VISIBLE | SS_LEFT,
                    CRect(0,0,0,0), this);

    m_edX.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                 CRect(0,0,0,0), this, 3014);
    m_edX.SetWindowText(_T("18"));

    m_edY.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                 CRect(0,0,0,0), this, 3015);
    m_edY.SetWindowText(_T("18"));

    // 차트 유형
    m_lblChartType.Create(_T("차트 유형"), WS_CHILD | WS_VISIBLE | SS_LEFT,
                          CRect(0,0,0,0), this);

    m_cmbChartType.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST,
                          CRect(0,0,0,0), this, 3016);
    m_cmbChartType.AddString(_T("꺾은선 (LINE)"));
    m_cmbChartType.AddString(_T("막대 (BAR)"));
    m_cmbChartType.AddString(_T("도넛 (DONUT)"));
    m_cmbChartType.AddString(_T("표 (TABLE)"));
    m_cmbChartType.SetCurSel(0);

    // 옵션 토글 — BS_AUTOCHECKBOX 로 클릭 시 자동 토글
    m_chkTitle.Create(_T("제목 표시"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                      CRect(0,0,0,0), this, 3017);
    m_chkTitle.SetCheck(BST_CHECKED);

    m_chkGrid.Create(_T("그리드선 표시"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                     CRect(0,0,0,0), this, 3018);
    m_chkGrid.SetCheck(BST_CHECKED);

    // 색상
    m_lblColor.Create(_T("색상"), WS_CHILD | WS_VISIBLE | SS_LEFT,
                      CRect(0,0,0,0), this);
    m_btnColor.Create(_T("색상 선택…"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      CRect(0,0,0,0), this, 3019);

    // 하단 버튼
    m_btnApply.Create(_T("적용"), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                      CRect(0,0,0,0), this, 3001);
    m_btnReset.Create(_T("초기화"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      CRect(0,0,0,0), this, 3002);

    return 0;
}

void CFormatPane::LayoutChildren(int cx, int cy)
{
    if (cx <= 0 || cy <= 0) return;

    int margin = 8;
    int y      = margin;
    int lblH   = 16;
    int editH  = 20;
    int halfW  = (cx - margin * 3) / 2;
    if (halfW < 1) halfW = 1;

    // 크기
    m_lblSize.SetWindowPos(nullptr, margin, y, cx - margin*2, lblH, SWP_NOZORDER);
    y += lblH + 2;
    m_edWidth.SetWindowPos(nullptr,  margin,           y, halfW - 16, editH, SWP_NOZORDER);
    m_spinWidth.SetWindowPos(nullptr, margin + halfW - 16, y, 16, editH, SWP_NOZORDER);
    m_edHeight.SetWindowPos(nullptr, margin*2 + halfW, y, halfW - 16, editH, SWP_NOZORDER);
    m_spinHeight.SetWindowPos(nullptr, margin*2 + halfW*2 - 16, y, 16, editH, SWP_NOZORDER);
    y += editH + margin;

    // 위치
    m_lblPos.SetWindowPos(nullptr, margin, y, cx - margin*2, lblH, SWP_NOZORDER);
    y += lblH + 2;
    m_edX.SetWindowPos(nullptr, margin,           y, halfW, editH, SWP_NOZORDER);
    m_edY.SetWindowPos(nullptr, margin*2 + halfW, y, halfW, editH, SWP_NOZORDER);
    y += editH + margin;

    // 차트 유형
    m_lblChartType.SetWindowPos(nullptr, margin, y, cx - margin*2, lblH, SWP_NOZORDER);
    y += lblH + 2;
    m_cmbChartType.SetWindowPos(nullptr, margin, y, cx - margin*2, 100, SWP_NOZORDER);
    y += editH + margin;

    // 옵션
    m_chkTitle.SetWindowPos(nullptr, margin, y, cx - margin*2, editH, SWP_NOZORDER);
    y += editH + 4;
    m_chkGrid.SetWindowPos(nullptr, margin, y, cx - margin*2, editH, SWP_NOZORDER);
    y += editH + margin;

    // 색상
    m_lblColor.SetWindowPos(nullptr, margin, y, cx - margin*2, lblH, SWP_NOZORDER);
    y += lblH + 2;
    m_btnColor.SetWindowPos(nullptr, margin, y, cx - margin*2, editH + 2, SWP_NOZORDER);
    y += editH + margin;

    // 버튼 (하단)
    int btnH = 24;
    int btnW = (cx - margin*3) / 2;
    if (btnW < 1) btnW = 1;
    int btnY = cy - margin - btnH;
    if (btnY < y) btnY = y;
    m_btnApply.SetWindowPos(nullptr, margin,           btnY, btnW, btnH, SWP_NOZORDER);
    m_btnReset.SetWindowPos(nullptr, margin*2 + btnW,  btnY, btnW, btnH, SWP_NOZORDER);
}

void CFormatPane::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    LayoutChildren(cx, cy);
}

void CFormatPane::SetSelectedViz(int vizIndex, LPCTSTR title)
{
    m_vizId = vizIndex;
    (void)title;

    // 편집 필드는 ApplyToViz 와 반대 방향 — 호출자가 viz 값을 넘겨야 채울 수 있으므로
    // MainFrame::OnVizSelected 에서 viz 참조를 통해 직접 채움.
    // 여기서는 vizIndex만 저장.
}

void CFormatPane::ApplyToViz(deepmetria::Visualization& viz) const
{
    // 크기
    CString s;
    m_edWidth.GetWindowText(s);
    int w = _ttoi(s);
    if (w >= 80) viz.width = w;

    m_edHeight.GetWindowText(s);
    int h = _ttoi(s);
    if (h >= 60) viz.height = h;

    // 위치
    m_edX.GetWindowText(s);
    viz.x = _ttoi(s);

    m_edY.GetWindowText(s);
    viz.y = _ttoi(s);

    // 차트 유형
    int sel = m_cmbChartType.GetCurSel();
    switch (sel)
    {
    case 0: viz.type = deepmetria::VizType::Line;    break;
    case 1: viz.type = deepmetria::VizType::Bar;     break;
    case 2: viz.type = deepmetria::VizType::Pie;     break;
    case 3: viz.type = deepmetria::VizType::Table;   break;
    default: break;
    }

    // 표시 옵션
    viz.showTitle = (m_chkTitle.GetCheck() == BST_CHECKED);
    viz.showGrid  = (m_chkGrid.GetCheck()  == BST_CHECKED);

    // 색상 — COLORREF(0x00BBGGRR) → DWORD(0x00RRGGBB) 변환
    DWORD c = static_cast<DWORD>(m_colorFill);
    viz.fillColor = ((c & 0xFF) << 16) | (c & 0xFF00) | ((c >> 16) & 0xFF);
}

void CFormatPane::FillFromViz(const deepmetria::Visualization& viz)
{
    CString s;

    s.Format(_T("%d"), viz.width);
    m_edWidth.SetWindowText(s);

    s.Format(_T("%d"), viz.height);
    m_edHeight.SetWindowText(s);

    s.Format(_T("%d"), viz.x);
    m_edX.SetWindowText(s);

    s.Format(_T("%d"), viz.y);
    m_edY.SetWindowText(s);

    int sel = 0;
    switch (viz.type)
    {
    case deepmetria::VizType::Line:  sel = 0; break;
    case deepmetria::VizType::Bar:   sel = 1; break;
    case deepmetria::VizType::Pie:   sel = 2; break;
    case deepmetria::VizType::Table: sel = 3; break;
    default:                          sel = 0; break;
    }
    m_cmbChartType.SetCurSel(sel);

    // 표시 옵션
    m_chkTitle.SetCheck(viz.showTitle ? BST_CHECKED : BST_UNCHECKED);
    m_chkGrid .SetCheck(viz.showGrid  ? BST_CHECKED : BST_UNCHECKED);

    // 색상: viz.fillColor (0x00RRGGBB) → COLORREF (0x00BBGGRR)
    DWORD c = viz.fillColor;
    m_colorFill = RGB((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
}

void CFormatPane::OnBtnApply()
{
    if (m_vizId < 0) return;
    CWnd* pFrame = GetParentFrame();
    if (pFrame)
        pFrame->PostMessage(deepmetria::WM_USER_VIZ_FORMAT_APPLIED,
                            static_cast<WPARAM>(m_vizId), 0);
}

void CFormatPane::OnBtnReset()
{
    m_edWidth.SetWindowText(_T("360"));
    m_edHeight.SetWindowText(_T("230"));
    m_edX.SetWindowText(_T("18"));
    m_edY.SetWindowText(_T("18"));
    m_cmbChartType.SetCurSel(0);
    m_chkTitle.SetCheck(BST_CHECKED);
    m_chkGrid.SetCheck(BST_CHECKED);
    m_colorFill = RGB(0x4F, 0x46, 0xE5);
}

// 그래프 색상 선택 — 시스템 표준 ChooseColor 다이얼로그
void CFormatPane::OnBtnPickColor()
{
    CColorDialog dlg(m_colorFill, CC_FULLOPEN, this);
    if (dlg.DoModal() == IDOK)
    {
        m_colorFill = dlg.GetColor();
        // 즉시 적용
        OnBtnApply();
    }
}
