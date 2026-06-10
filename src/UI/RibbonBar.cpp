// RibbonBar.cpp - 자체 그림 리본 바 구현
#include "stdafx.h"
#include "RibbonBar.h"
#include "Messages.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace deepmetria {

// 색상 상수
static const COLORREF kColorBg         = RGB(0xFF, 0xFF, 0xFF); // 흰 배경
static const COLORREF kColorActiveBg   = RGB(0x25, 0x63, 0xEB); // 활성 탭 배경 (Primary Blue)
static const COLORREF kColorActiveText = RGB(0xFF, 0xFF, 0xFF); // 활성 탭 흰 글씨
static const COLORREF kColorNormalText = RGB(0x6B, 0x72, 0x80); // 비활성 텍스트 회색
static const COLORREF kColorHoverBg    = RGB(0xEE, 0xF2, 0xFF); // 호버 배경
static const COLORREF kColorBorder     = RGB(0xD8, 0xDC, 0xE5); // 하단 경계선

IMPLEMENT_DYNAMIC(CRibbonBar, CWnd)

BEGIN_MESSAGE_MAP(CRibbonBar, CWnd)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_SIZE()
    ON_MESSAGE(WM_MOUSELEAVE, &CRibbonBar::OnMouseLeave)
END_MESSAGE_MAP()

CRibbonBar::CRibbonBar()
{
    // 6개 탭 초기화 (순서: Open, Save, Summary, Query, Format, Export)
    struct TabInit { UINT cmdId; LPCTSTR label; };
    static const TabInit kTabInits[] = {
        { ID_FILE_OPEN,    _T("열기")     },
        { ID_FILE_SAVE,    _T("저장")     },
        { ID_VIEW_SUMMARY, _T("요약")     },
        { ID_VIEW_QUERY,   _T("질문")     },
        { ID_VIEW_FORMAT,  _T("서식")     },
        { ID_FILE_EXPORT,  _T("내보내기") },
    };

    for (int i = 0; i < 6; ++i)
    {
        Tab t;
        t.cmdId    = kTabInits[i].cmdId;
        t.label    = kTabInits[i].label;
        // Summary 만 시작 시 열림. Format은 viz 선택 전까지 비활성.
        t.isActive = (i == static_cast<int>(ActiveTab::Summary));
        t.isHover  = false;
        t.isEnabled = (i != static_cast<int>(ActiveTab::Format) &&
                       i != static_cast<int>(ActiveTab::Export));
        m_tabs.Add(t);
    }
}

void CRibbonBar::SetTabOpen(ActiveTab tab, bool open)
{
    int idx = static_cast<int>(tab);
    if (idx < 0 || idx >= m_tabs.GetCount()) return;
    m_tabs[idx].isActive = open;
    if (open) m_active = tab;
    if (GetSafeHwnd()) Invalidate();
}

bool CRibbonBar::IsTabOpen(ActiveTab tab) const
{
    int idx = static_cast<int>(tab);
    if (idx < 0 || idx >= m_tabs.GetCount()) return false;
    return m_tabs[idx].isActive;
}

void CRibbonBar::SetTabEnabled(ActiveTab tab, bool enabled)
{
    int idx = static_cast<int>(tab);
    if (idx < 0 || idx >= m_tabs.GetCount()) return;
    m_tabs[idx].isEnabled = enabled;
    if (!enabled) m_tabs[idx].isActive = false;  // 비활성화 시 자동 닫힘
    if (GetSafeHwnd()) Invalidate();
}

bool CRibbonBar::IsTabEnabled(ActiveTab tab) const
{
    int idx = static_cast<int>(tab);
    if (idx < 0 || idx >= m_tabs.GetCount()) return false;
    return m_tabs[idx].isEnabled;
}

CRibbonBar::~CRibbonBar() = default;

BOOL CRibbonBar::Create(CWnd* pParent)
{
    LPCTSTR wndClass = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW,
        ::LoadCursor(nullptr, IDC_ARROW),
        reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
        nullptr);

    return CWnd::CreateEx(
        0,
        wndClass,
        _T(""),
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        CRect(0, 0, 0, kHeight),
        pParent,
        0);
}

void CRibbonBar::SetActiveTab(ActiveTab tab)
{
    m_active = tab;
    int tabIdx = static_cast<int>(tab);
    for (int i = 0; i < m_tabs.GetCount(); ++i)
        m_tabs[i].isActive = (i == tabIdx && tabIdx < 6);
    if (GetSafeHwnd())
        Invalidate();
}

// 탭 레이아웃 계산: 좌측 6개 탭(각 88px), 우측 설정(64px)
void CRibbonBar::LayoutTabs(int cx)
{
    constexpr int kTabWidth      = 88;
    constexpr int kSettingsWidth = 64;

    for (int i = 0; i < m_tabs.GetCount(); ++i)
    {
        int x = i * kTabWidth;
        m_tabs[i].rect = CRect(x, 0, x + kTabWidth, kHeight);
    }

    // 설정 버튼: 우측 끝
    m_rcSettings = CRect(cx - kSettingsWidth, 0, cx, kHeight);
}

// 아이콘 글리프 그리기 (GDI 단순 도형)
void CRibbonBar::DrawIconGlyph(CDC& dc, const CRect& iconRect, int tabIndex)
{
    bool active = m_tabs[tabIndex].isActive;
    COLORREF color = active ? kColorActiveText : kColorNormalText;
    CPen pen(PS_SOLID, 2, color);
    CBrush brush(color);
    CPen*   oldPen   = dc.SelectObject(&pen);
    CBrush* oldBrush = dc.SelectObject(&brush);

    int cx = iconRect.CenterPoint().x;
    int cy = iconRect.CenterPoint().y;
    int r  = 9;

    COLORREF bgFill = active ? kColorActiveBg : kColorBg;

    switch (tabIndex)
    {
    case 0: // 열기 - 폴더
    {
        CRect folder(cx - r, cy - 5, cx + r, cy + 7);
        dc.FillSolidRect(&folder, color);
        CRect tab(cx - r, cy - 8, cx - 1, cy - 3);
        dc.FillSolidRect(&tab, color);
        CRect inner(cx - r + 2, cy - 3, cx + r - 2, cy + 5);
        dc.FillSolidRect(&inner, bgFill);
        break;
    }
    case 1: // 저장 - 디스크
    {
        CRect disk(cx - r, cy - r + 2, cx + r, cy + r - 2);
        dc.FillSolidRect(&disk, color);
        CRect label(cx - 4, cy - r + 3, cx + 4, cy - 1);
        dc.FillSolidRect(&label, bgFill);
        CRect bottom(cx - r + 2, cy + 1, cx + r - 2, cy + r - 3);
        dc.FillSolidRect(&bottom, bgFill);
        break;
    }
    case 2: // 요약 - 리스트 줄 3개
    {
        for (int line = 0; line < 3; ++line)
        {
            int y0 = cy - 5 + line * 5;
            CRect lineRect(cx - r, y0, cx + r, y0 + 2);
            dc.FillSolidRect(&lineRect, color);
        }
        break;
    }
    case 3: // 질문 - 말풍선
    {
        dc.Ellipse(cx - r, cy - 7, cx + r, cy + 5);
        POINT tail[] = { {cx - 2, cy + 5}, {cx - 6, cy + 9}, {cx + 2, cy + 5} };
        dc.Polygon(tail, 3);
        break;
    }
    case 4: // 서식 - 슬라이더
    {
        for (int line = 0; line < 3; ++line)
        {
            int y0 = cy - 4 + line * 4;
            CRect lineRect(cx - r, y0, cx + r, y0 + 2);
            dc.FillSolidRect(&lineRect, color);
        }
        CBrush whiteBrush(bgFill);
        CPen   whitePen(PS_SOLID, 1, bgFill);
        dc.SelectObject(&whitePen);
        dc.SelectObject(&whiteBrush);
        dc.Ellipse(cx - 2, cy - 5, cx + 4, cy + 1);
        dc.SelectObject(&pen);
        dc.SelectObject(&brush);
        break;
    }
    case 5: // 내보내기 - 위쪽 화살표
    {
        POINT arrow[] = {
            {cx,     cy - r + 2},
            {cx - 5, cy - 2    },
            {cx - 2, cy - 2    },
            {cx - 2, cy + r - 2},
            {cx + 2, cy + r - 2},
            {cx + 2, cy - 2    },
            {cx + 5, cy - 2    },
        };
        dc.Polygon(arrow, 7);
        break;
    }
    default:
        break;
    }

    dc.SelectObject(oldPen);
    dc.SelectObject(oldBrush);
}

void CRibbonBar::DrawTab(CDC& dc, const Tab& t)
{
    CRect rc = t.rect;

    // 배경 채우기 — disabled 시 호버/액티브 무시하고 회색
    COLORREF bgColor = kColorBg;
    if (!t.isEnabled)
        bgColor = RGB(0xF5, 0xF5, 0xF5);
    else if (t.isActive)
        bgColor = kColorActiveBg;
    else if (t.isHover)
        bgColor = kColorHoverBg;

    dc.FillSolidRect(&rc, bgColor);

    // 활성/호버 탭: 둥근 모서리 (RoundRect)
    if (t.isActive || t.isHover)
    {
        CPen pen(PS_SOLID, 1, bgColor);
        CBrush brush(bgColor);
        CPen*   oldPen   = dc.SelectObject(&pen);
        CBrush* oldBrush = dc.SelectObject(&brush);
        dc.RoundRect(rc.left + 2, rc.top + 4, rc.right - 2, rc.bottom - 4, 8, 8);
        dc.SelectObject(oldPen);
        dc.SelectObject(oldBrush);
    }

    // 탭 인덱스 계산
    int tabIdx = -1;
    for (int i = 0; i < m_tabs.GetCount(); ++i)
    {
        if (&m_tabs[i] == &t) { tabIdx = i; break; }
    }

    // 아이콘 영역: 상단 22px 정사각형 (수평 중앙)
    int iconSize = 22;
    int iconX = rc.CenterPoint().x - iconSize / 2;
    int iconY = rc.top + 8;
    CRect iconRect(iconX, iconY, iconX + iconSize, iconY + iconSize);

    if (tabIdx >= 0)
        DrawIconGlyph(dc, iconRect, tabIdx);

    // 라벨 텍스트
    LOGFONT lf = {};
    _tcscpy_s(lf.lfFaceName, _T("맑은 고딕"));
    lf.lfHeight  = -MulDiv(10, dc.GetDeviceCaps(LOGPIXELSY), 72);
    lf.lfWeight  = t.isActive ? FW_SEMIBOLD : FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;

    CFont font;
    font.CreateFontIndirect(&lf);
    CFont* oldFont = dc.SelectObject(&font);

    COLORREF textColor;
    if (!t.isEnabled)
        textColor = RGB(0xC0, 0xC4, 0xCC);  // 비활성 회색
    else if (t.isActive)
        textColor = kColorActiveText;
    else
        textColor = kColorNormalText;
    dc.SetTextColor(textColor);
    dc.SetBkMode(TRANSPARENT);

    CRect labelRect(rc.left, rc.top + 38, rc.right, rc.bottom - 2);
    dc.DrawText(t.label, &labelRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    dc.SelectObject(oldFont);
}

void CRibbonBar::DrawSettingsButton(CDC& dc)
{
    COLORREF settingsBg = m_hoverSettings ? kColorHoverBg : kColorBg;
    dc.FillSolidRect(&m_rcSettings, settingsBg);

    COLORREF iconColor = kColorNormalText;
    int cx = m_rcSettings.CenterPoint().x;
    int cy = m_rcSettings.top + 20;

    CPen pen(PS_SOLID, 2, iconColor);
    CBrush brush(iconColor);
    CPen*   oldPen   = dc.SelectObject(&pen);
    CBrush* oldBrush = dc.SelectObject(&brush);

    // 외부 원 (톱니 대신 단순 원형 아이콘)
    dc.Ellipse(cx - 9, cy - 9, cx + 9, cy + 9);
    // 내부 원 (흰 색으로 링 효과)
    CBrush whiteBrush(settingsBg);
    CPen whitePen(PS_SOLID, 1, settingsBg);
    dc.SelectObject(&whitePen);
    dc.SelectObject(&whiteBrush);
    dc.Ellipse(cx - 5, cy - 5, cx + 5, cy + 5);

    dc.SelectObject(oldPen);
    dc.SelectObject(oldBrush);

    // 라벨
    LOGFONT lf = {};
    _tcscpy_s(lf.lfFaceName, _T("맑은 고딕"));
    lf.lfHeight  = -MulDiv(10, dc.GetDeviceCaps(LOGPIXELSY), 72);
    lf.lfWeight  = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;

    CFont font;
    font.CreateFontIndirect(&lf);
    CFont* oldFont = dc.SelectObject(&font);

    dc.SetTextColor(kColorNormalText);
    dc.SetBkMode(TRANSPARENT);

    CRect labelRect(m_rcSettings.left, m_rcSettings.top + 38,
                    m_rcSettings.right, m_rcSettings.bottom - 2);
    dc.DrawText(_T("설정"), &labelRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    dc.SelectObject(oldFont);
}

void CRibbonBar::OnPaint()
{
    CPaintDC dc(this);

    CRect clientRect;
    GetClientRect(&clientRect);

    // 전체 배경
    dc.FillSolidRect(&clientRect, kColorBg);

    // 하단 경계선
    CRect borderRect(clientRect.left, clientRect.bottom - 1,
                     clientRect.right, clientRect.bottom);
    dc.FillSolidRect(&borderRect, kColorBorder);

    // 탭 그리기
    for (int i = 0; i < m_tabs.GetCount(); ++i)
        DrawTab(dc, m_tabs[i]);

    // 설정 버튼
    DrawSettingsButton(dc);
}

void CRibbonBar::OnLButtonDown(UINT /*nFlags*/, CPoint point)
{
    for (int i = 0; i < m_tabs.GetCount(); ++i)
    {
        if (!m_tabs[i].rect.PtInRect(point)) continue;

        // 비활성 탭은 무시
        if (!m_tabs[i].isEnabled) return;

        ActiveTab newTab = static_cast<ActiveTab>(i);

        // Open/Save/Export — 즉시 명령
        if (newTab == ActiveTab::Open || newTab == ActiveTab::Save || newTab == ActiveTab::Export)
        {
            CFrameWnd* pFrame = GetParentFrame();
            if (pFrame)
                pFrame->PostMessage(WM_COMMAND, MAKEWPARAM(m_tabs[i].cmdId, 0), 0);
            return;
        }

        // Summary/Query/Format — 토글 (재클릭 시 닫힘, 다중 동시 표시 가능)
        m_tabs[i].isActive = !m_tabs[i].isActive;
        if (m_tabs[i].isActive) m_active = newTab;
        Invalidate();

        if (CFrameWnd* pFrame = GetParentFrame())
            pFrame->PostMessage(deepmetria::WM_USER_RIBBON_TAB_CHANGED,
                                static_cast<WPARAM>(i), 0);
        return;
    }

    // 설정 버튼 hit-test
    if (m_rcSettings.PtInRect(point))
    {
        CFrameWnd* pFrame = GetParentFrame();
        if (pFrame)
            pFrame->PostMessage(WM_COMMAND, MAKEWPARAM(ID_APP_SETTINGS, 0), 0);
    }
}

void CRibbonBar::OnMouseMove(UINT /*nFlags*/, CPoint point)
{
    if (!m_tracking)
    {
        TRACKMOUSEEVENT tme = {};
        tme.cbSize    = sizeof(tme);
        tme.dwFlags   = TME_LEAVE;
        tme.hwndTrack = GetSafeHwnd();
        TrackMouseEvent(&tme);
        m_tracking = true;
    }

    bool changed = false;

    for (int i = 0; i < m_tabs.GetCount(); ++i)
    {
        bool hov = !!m_tabs[i].rect.PtInRect(point);
        if (m_tabs[i].isHover != hov)
        {
            m_tabs[i].isHover = hov;
            changed = true;
        }
    }

    bool settingsHov = !!m_rcSettings.PtInRect(point);
    if (m_hoverSettings != settingsHov)
    {
        m_hoverSettings = settingsHov;
        changed = true;
    }

    if (changed)
        Invalidate(FALSE);
}

LRESULT CRibbonBar::OnMouseLeave(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_tracking = false;
    bool changed = false;

    for (int i = 0; i < m_tabs.GetCount(); ++i)
    {
        if (m_tabs[i].isHover) { m_tabs[i].isHover = false; changed = true; }
    }
    if (m_hoverSettings) { m_hoverSettings = false; changed = true; }
    if (changed) Invalidate(FALSE);
    return 0;
}

void CRibbonBar::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    LayoutTabs(cx);
    Invalidate(FALSE);
}

} // namespace deepmetria
