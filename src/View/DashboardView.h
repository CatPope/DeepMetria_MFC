#pragma once

// DashboardView.h — 대시보드 메인 뷰
// 시각화 카드를 그리드로 배치하고 WM_VISUALIZATION_READY 처리
// Architecture §3 / DetailedSpec §3.2 참조

#include "../Common/Types.h"

// 전방 선언
class ChartRenderer;

// ============================================================
// CDashboardView — CScrollView 파생
// ============================================================
class CDashboardView : public CScrollView
{
    DECLARE_DYNCREATE(CDashboardView)

public:
    CDashboardView();
    virtual ~CDashboardView();

    // 공개 인터페이스
    void AddVisualization(const VisualizationInfo& info);
    void RemoveVisualization(const CString& vizId);
    void ClearVisualizations();
    void SetDashboardDetail(const DashboardDetail& detail);

protected:
    // 시각화 카드 목록
    std::vector<VisualizationInfo> m_visualizations;

    // 카드 레이아웃 설정
    static const int CARD_MARGIN  = 8;   // 카드 간격 (px)
    static const int CARD_COLS    = 2;   // 그리드 열 수
    static const int CARD_H       = 320; // 카드 높이 (px)

    // 선택된 카드 ID
    CString m_selectedVizId;

    // 현재 대시보드 정보
    DashboardDetail m_dashboardDetail;

    // 카드 위치 계산
    CRect GetCardRect(int index) const;
    int   HitTestCard(CPoint pt) const;

    // 뷰 오버라이드
    virtual void OnInitialUpdate() override;
    virtual void OnDraw(CDC* pDC) override;
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    // 드래그 상태
    BOOL    m_bDragging;
    int     m_nDragCard;      // 드래그 중인 카드 인덱스
    CPoint  m_ptDragStart;    // 드래그 시작 점
    CPoint  m_ptDragOffset;   // 카드 내 오프셋

    // 리사이즈 상태
    BOOL    m_bResizing;
    int     m_nResizeCard;
    CPoint  m_ptResizeStart;
    int     m_nResizeEdge;    // 0=none, 1=right, 2=bottom, 3=corner

    // 메시지 핸들러
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

    // 커스텀 메시지 핸들러
    afx_msg LRESULT OnVisualizationReady(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    void RecalcScrollSize();
    void DrawCard(CDC* pDC, int index, const CRect& rect);
    void DrawCardBackground(CDC* pDC, const CRect& rect, BOOL bSelected);
    void DrawCardTitle(CDC* pDC, const CRect& rect, const CString& title);
    void DrawPlaceholder(CDC* pDC, const CRect& rect, const CString& vizType);

    // 툴팁
    CToolTipCtrl m_toolTip;
    int          m_nHoverCard;  // 현재 호버 중인 카드 인덱스
};
