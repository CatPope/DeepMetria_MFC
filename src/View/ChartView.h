#pragma once

// ChartView.h — 단일 차트 확대 뷰
// DashboardView에서 카드 더블클릭 시 상세 보기
// Architecture §3 / DetailedSpec §3.2 참조

#include "../Common/Types.h"

// 전방 선언
class ChartRenderer;

// ============================================================
// CChartView — CView 파생
// ============================================================
class CChartView : public CView
{
    DECLARE_DYNCREATE(CChartView)

public:
    CChartView();
    virtual ~CChartView();

    // 차트 설정 및 데이터 주입
    void SetChartConfig(const ChartConfig& config, const CString& data);

protected:
    ChartConfig m_chartConfig;  // 현재 차트 설정
    CString     m_chartData;    // 차트 데이터 (JSON)

    // 툴팁
    CToolTipCtrl m_toolTip;
    CString      m_tooltipText;
    CPoint       m_ptLastMouse;

    // 뷰 오버라이드
    virtual void OnDraw(CDC* pDC) override;
    virtual void OnInitialUpdate() override;
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    // 메시지 핸들러
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()

private:
    void UpdateTooltip(CPoint pt);
    CString BuildTooltipText(CPoint pt) const;
};
