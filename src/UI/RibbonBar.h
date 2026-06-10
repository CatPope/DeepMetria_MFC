// RibbonBar.h - 자체 그림 리본 바 (CWnd 직접 상속, RecalcLayout 기반 배치)
#pragma once

namespace deepmetria {

class CRibbonBar : public CWnd
{
    DECLARE_DYNAMIC(CRibbonBar)

public:
    static constexpr int kHeight = 64;

    struct Tab {
        UINT    cmdId;
        CString label;
        CRect   rect;
        bool    isActive  = false;   // Summary/Query/Format 의 사이드바 열림 상태 (토글)
        bool    isHover   = false;
        bool    isEnabled = true;    // Format은 viz 선택 시에만 활성화
    };

    enum class ActiveTab {
        Open    = 0,
        Save    = 1,
        Summary = 2,
        Query   = 3,
        Format  = 4,
        Export  = 5,
        Settings = 6,   // 우측 설정 버튼 (탭이 아닌 액션)
        None    = 99,
    };

    CRibbonBar();
    virtual ~CRibbonBar();

    BOOL Create(CWnd* pParent);

    void      SetActiveTab(ActiveTab tab);
    ActiveTab GetActiveTab() const { return m_active; }

    // 사이드바 토글 — Summary/Query/Format 각각 독립
    void      SetTabOpen(ActiveTab tab, bool open);
    bool      IsTabOpen(ActiveTab tab) const;

    // 탭 활성/비활성 — Format은 viz 미선택 시 disabled
    void      SetTabEnabled(ActiveTab tab, bool enabled);
    bool      IsTabEnabled(ActiveTab tab) const;

protected:
    afx_msg void    OnPaint();
    afx_msg void    OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void    OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void    OnSize(UINT nType, int cx, int cy);
    afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

private:
    void LayoutTabs(int cx);
    void DrawTab(CDC& dc, const Tab& t);
    void DrawIconGlyph(CDC& dc, const CRect& iconRect, int tabIndex);
    void DrawSettingsButton(CDC& dc);

    CArray<Tab, Tab&> m_tabs;
    CRect             m_rcSettings;
    bool              m_hoverSettings = false;
    ActiveTab         m_active        = ActiveTab::Summary;
    bool              m_tracking      = false;
};

} // namespace deepmetria
