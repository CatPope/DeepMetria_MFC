#pragma once

// DashboardView.h — 대시보드 메인 뷰
// 시각화 카드를 그리드로 배치하고 WM_VISUALIZATION_READY 처리
// Architecture §3 / DetailedSpec §3.2 참조

#include "../stdafx.h"
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
    static const int CARD_H       = 240; // 카드 높이 (px)

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

    // 메시지 핸들러
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    // post-MVP: 드래그 앤 드롭 카드 순서 변경 (빈 핸들러)
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

    // 커스텀 메시지 핸들러
    afx_msg LRESULT OnVisualizationReady(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    void RecalcScrollSize();
    void DrawCard(CDC* pDC, int index, const CRect& rect);
    void DrawCardBackground(CDC* pDC, const CRect& rect, BOOL bSelected);
    void DrawCardTitle(CDC* pDC, const CRect& rect, const CString& title);
    void DrawPlaceholder(CDC* pDC, const CRect& rect, const CString& vizType);
};
