// DeepMetriaView.h - 대시보드 CView (GDI+ 캔버스)
#pragma once
#include "DeepMetriaDoc.h"
#include "RibbonBar.h"

class CDeepMetriaView : public CView
{
protected:
    DECLARE_DYNCREATE(CDeepMetriaView)
    CDeepMetriaView();
    virtual ~CDeepMetriaView();

public:
    CDeepMetriaDoc* GetDocument() const;

    virtual void OnDraw(CDC* pDC) override;
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;

    // 활성 탭 갱신 (MainFrame에서 PostMessage 후 호출)
    void SetActiveTab(deepmetria::CRibbonBar::ActiveTab tab);

    // 실제 DataSource로 미리보기 리스트 컬럼/행 갱신
    void RefreshPreviewList(const deepmetria::DataSource& ds);

    // 분석 진행률 갱신 (CoT 카드 표시용)
    void SetAnalysisProgress(int pct) { m_progressPct = pct; Invalidate(); }

    // FormatPane 활성 가드용 — 선택된 시각화 인덱스 (-1: 미선택)
    int GetSelectedViz() const { return m_selectedViz; }

    // 외부에서 호출 가능 — MainFrame이 새 viz 추가 후 스크롤 범위 갱신
    void UpdateScrollBars();

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnDropFiles(HDROP hDrop);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);  // 깜빡임 제거
    DECLARE_MESSAGE_MAP()

    // 스크롤 인프라 (UpdateScrollBars는 public 선언)
    int  ScrollLineH() const { return 16; }
    int  ScrollLineV() const { return 16; }

private:
    void DrawEmpty(Gdiplus::Graphics& g, int w, int h);
    void DrawSummaryTab(Gdiplus::Graphics& g, int w, int h, CDeepMetriaDoc* pDoc);
    void DrawQueryTab(Gdiplus::Graphics& g, int w, int h, CDeepMetriaDoc* pDoc);
    void DrawVisualizationTab(Gdiplus::Graphics& g, int w, int h, CDeepMetriaDoc* pDoc);
    void DrawKpiCard(Gdiplus::Graphics& g, float x, float y, float w, float h,
                     LPCWSTR title, LPCWSTR value, LPCWSTR sub);
    void DrawDonutCard(Gdiplus::Graphics& g, float x, float y, float w, float h,
                       LPCWSTR title);
    void DrawGridBackground(Gdiplus::Graphics& g, int w, int h);
    void DrawResizeHandles(Gdiplus::Graphics& g, const deepmetria::Visualization& viz);
    void DrawPreviewTable(Gdiplus::Graphics& g, int x, int y, int w, int h,
                          const deepmetria::DataSource& ds);

    // 미리보기 표는 더 이상 ChildWindow 가 아닌 OnDraw에서 GDI+로 직접 렌더.
    // m_listPreview는 호환성을 위해 남기지만 사용하지 않음.
    CListCtrl m_listPreview;

    deepmetria::CRibbonBar::ActiveTab m_activeTab =
        deepmetria::CRibbonBar::ActiveTab::None;

    enum class DragMode {
        None, Move, TableSpawn,
        ResizeTL, ResizeT, ResizeTR,
        ResizeL,            ResizeR,
        ResizeBL, ResizeB, ResizeBR
    };
    DragMode m_dragMode      = DragMode::None;
    CPoint   m_dragOrigin    = {};
    int      m_dragVizIndex  = -1;
    int      m_selectedViz   = -1;

    // 메인 화면 미리보기 표 영역 (DrawSummaryTab의 (8, 12, W-16, 180))
    CRect    m_tableRect     = {};

    // 분석 진행률 (0~100). -1이면 미실행.
    int      m_progressPct   = -1;

    // 시각화 hover 툴팁
    int      m_hoverVizIndex = -1;
    CPoint   m_hoverPoint    = {};
};

#ifndef _DEBUG
inline CDeepMetriaDoc* CDeepMetriaView::GetDocument() const
{
    return reinterpret_cast<CDeepMetriaDoc*>(m_pDocument);
}
#endif
