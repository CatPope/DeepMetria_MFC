#include "stdafx.h"
#include "DashboardView.h"
#include "../Domain/Orchestrator/AnalysisFlow.h"
#include "../Renderer/ChartRenderer.h"

// ============================================================
// IMPLEMENT_DYNCREATE / 메시지 맵
// ============================================================
IMPLEMENT_DYNCREATE(CDashboardView, CScrollView)

BEGIN_MESSAGE_MAP(CDashboardView, CScrollView)
    ON_WM_SIZE()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_MESSAGE(WM_VISUALIZATION_READY, &CDashboardView::OnVisualizationReady)
END_MESSAGE_MAP()

// ============================================================
// 생성 / 소멸
// ============================================================
CDashboardView::CDashboardView()
    : CScrollView()
    , m_nHoverCard(-1)
{
}

CDashboardView::~CDashboardView()
{
}

// ============================================================
// 초기화
// ============================================================
void CDashboardView::OnInitialUpdate()
{
    CScrollView::OnInitialUpdate();
    RecalcScrollSize();

    // 툴팁 초기화
    m_toolTip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX);
    m_toolTip.SetMaxTipWidth(400);
    m_toolTip.SetDelayTime(TTDT_AUTOPOP, 15000); // 15초간 표시
    m_toolTip.AddTool(this, _T(""));
    m_toolTip.Activate(TRUE);
}

void CDashboardView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    Invalidate();
}

// ============================================================
// 공개 인터페이스
// ============================================================
void CDashboardView::AddVisualization(const VisualizationInfo& info)
{
    // 동일 ID가 있으면 교체
    for (auto& viz : m_visualizations)
    {
        if (viz.id == info.id)
        {
            viz = info;
            RecalcScrollSize();
            Invalidate();
            return;
        }
    }
    m_visualizations.push_back(info);
    RecalcScrollSize();
    Invalidate();
}

void CDashboardView::RemoveVisualization(const CString& vizId)
{
    auto it = std::remove_if(
        m_visualizations.begin(), m_visualizations.end(),
        [&vizId](const VisualizationInfo& v) { return v.id == vizId; });
    if (it != m_visualizations.end())
    {
        m_visualizations.erase(it, m_visualizations.end());
        RecalcScrollSize();
        Invalidate();
    }
}

void CDashboardView::ClearVisualizations()
{
    m_visualizations.clear();
    m_selectedVizId.Empty();
    RecalcScrollSize();
    Invalidate();
}

void CDashboardView::SetDashboardDetail(const DashboardDetail& detail)
{
    m_dashboardDetail = detail;
    m_visualizations  = detail.visualizations;
    RecalcScrollSize();
    Invalidate();
}

// ============================================================
// 레이아웃 계산
// ============================================================
CRect CDashboardView::GetCardRect(int index) const
{
    CRect clientRect;
    GetClientRect(&clientRect);

    int col      = index % CARD_COLS;
    int row      = index / CARD_COLS;
    int cardW    = (clientRect.Width() - CARD_MARGIN * (CARD_COLS + 1)) / CARD_COLS;
    int x        = CARD_MARGIN + col * (cardW + CARD_MARGIN);
    int y        = CARD_MARGIN + row * (CARD_H + CARD_MARGIN) - GetScrollPos(SB_VERT);
    return CRect(x, y, x + cardW, y + CARD_H);
}

int CDashboardView::HitTestCard(CPoint pt) const
{
    for (int i = 0; i < (int)m_visualizations.size(); ++i)
    {
        if (GetCardRect(i).PtInRect(pt))
            return i;
    }
    return -1;
}

void CDashboardView::RecalcScrollSize()
{
    if (m_visualizations.empty())
    {
        SetScrollSizes(MM_TEXT, CSize(0, 0));
        return;
    }

    CRect clientRect;
    GetClientRect(&clientRect);

    int rows    = ((int)m_visualizations.size() + CARD_COLS - 1) / CARD_COLS;
    int totalH  = rows * (CARD_H + CARD_MARGIN) + CARD_MARGIN;
    SetScrollSizes(MM_TEXT, CSize(clientRect.Width(), totalH));
}

// ============================================================
// 그리기
// ============================================================
void CDashboardView::OnDraw(CDC* pDC)
{
    if (m_visualizations.empty())
    {
        // 빈 상태 안내 텍스트
        CRect clientRect;
        GetClientRect(&clientRect);
        pDC->SetTextColor(RGB(160, 160, 160));
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(_T("질문을 입력하면 분석 결과가 여기에 표시됩니다."),
                      &clientRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    for (int i = 0; i < (int)m_visualizations.size(); ++i)
    {
        CRect cardRect = GetCardRect(i);
        // 화면 밖이면 스킵
        CRect clientRect;
        GetClientRect(&clientRect);
        CRect intersect;
        if (!intersect.IntersectRect(&cardRect, &clientRect))
            continue;

        BOOL bSelected = (m_visualizations[i].id == m_selectedVizId);
        DrawCard(pDC, i, cardRect);
        if (bSelected)
        {
            // 선택 테두리 강조
            CPen pen(PS_SOLID, 2, RGB(0, 120, 215));
            CPen* pOld = pDC->SelectObject(&pen);
            pDC->SelectStockObject(NULL_BRUSH);
            pDC->Rectangle(cardRect);
            pDC->SelectObject(pOld);
        }
    }
}

void CDashboardView::DrawCard(CDC* pDC, int index, const CRect& rect)
{
    const VisualizationInfo& viz = m_visualizations[index];
    BOOL bSelected = (viz.id == m_selectedVizId);

    DrawCardBackground(pDC, rect, bSelected);
    DrawCardTitle(pDC, rect, viz.title);

    // 차트 영역 (제목 아래)
    CRect chartRect = rect;
    chartRect.top += 44; // 제목 높이(2줄) 만큼 내림
    chartRect.DeflateRect(4, 0, 4, 4);

    // ChartRenderer로 실제 차트 렌더링
    if (!viz.chartConfig.dataJson.IsEmpty())
        CChartRenderer::Render(pDC, chartRect, viz.chartConfig);
    else
        DrawPlaceholder(pDC, chartRect, viz.vizType);
}

void CDashboardView::DrawCardBackground(CDC* pDC, const CRect& rect, BOOL bSelected)
{
    // 흰색 배경 + 그림자 효과
    CBrush bgBrush(RGB(255, 255, 255));
    CBrush shadowBrush(RGB(220, 220, 220));

    CRect shadowRect = rect;
    shadowRect.OffsetRect(2, 2);
    pDC->FillRect(&shadowRect, &shadowBrush);
    pDC->FillRect(&rect, &bgBrush);

    // 테두리
    COLORREF borderColor = bSelected ? RGB(0, 120, 215) : RGB(200, 200, 200);
    CPen borderPen(PS_SOLID, 1, borderColor);
    CPen* pOldPen = pDC->SelectObject(&borderPen);
    CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
    pDC->Rectangle(rect);
    pDC->SelectObject(pOldPen);
    pDC->SelectObject(pOldBrush);
}

void CDashboardView::DrawCardTitle(CDC* pDC, const CRect& rect, const CString& title)
{
    CRect titleRect = rect;
    titleRect.bottom = titleRect.top + 44;
    titleRect.DeflateRect(8, 4, 8, 2);

    pDC->SetTextColor(RGB(32, 32, 32));
    pDC->SetBkMode(TRANSPARENT);

    CFont* pOldFont = nullptr;
    CFont titleFont;
    LOGFONT lf = {};
    lf.lfHeight = -12;
    lf.lfWeight = FW_SEMIBOLD;
    _tcscpy_s(lf.lfFaceName, _T("Segoe UI"));
    if (titleFont.CreateFontIndirect(&lf))
        pOldFont = pDC->SelectObject(&titleFont);

    // 2줄 워드랩 + 말줄임
    pDC->DrawText(title, &titleRect, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);

    if (pOldFont)
        pDC->SelectObject(pOldFont);
}

void CDashboardView::DrawPlaceholder(CDC* pDC, const CRect& rect, const CString& vizType)
{
    pDC->SetTextColor(RGB(180, 180, 180));
    pDC->SetBkMode(TRANSPARENT);
    CString text;
    text.Format(_T("[%s]"), (LPCTSTR)vizType);
    pDC->DrawText(text, const_cast<CRect*>(&rect),
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// 메시지 핸들러
// ============================================================
void CDashboardView::OnSize(UINT nType, int cx, int cy)
{
    CScrollView::OnSize(nType, cx, cy);
    RecalcScrollSize();
    Invalidate();
}

void CDashboardView::OnLButtonDown(UINT nFlags, CPoint point)
{
    int idx = HitTestCard(point);
    if (idx >= 0)
        m_selectedVizId = m_visualizations[idx].id;
    else
        m_selectedVizId.Empty();
    Invalidate();
    CScrollView::OnLButtonDown(nFlags, point);
}

void CDashboardView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    int idx = HitTestCard(point);
    if (idx >= 0)
    {
        // post-MVP: ChartView 팝업으로 확대 보기
        // CChartView dlg; dlg.SetChartConfig(...); dlg.DoModal();
        TRACE(_T("CDashboardView: 카드 더블클릭 — vizId=%s\n"),
              (LPCTSTR)m_visualizations[idx].id);
    }
    CScrollView::OnLButtonDblClk(nFlags, point);
}

void CDashboardView::OnMouseMove(UINT nFlags, CPoint point)
{
    int idx = HitTestCard(point);
    if (idx != m_nHoverCard)
    {
        m_nHoverCard = idx;
        if (idx >= 0 && idx < (int)m_visualizations.size())
        {
            // 카드 제목 전체를 툴팁으로 표시
            m_toolTip.UpdateTipText(m_visualizations[idx].title, this);
            m_toolTip.Activate(TRUE);
        }
        else
        {
            m_toolTip.Activate(FALSE);
        }
    }
    CScrollView::OnMouseMove(nFlags, point);
}

void CDashboardView::OnLButtonUp(UINT nFlags, CPoint point)
{
    CScrollView::OnLButtonUp(nFlags, point);
}

BOOL CDashboardView::PreTranslateMessage(MSG* pMsg)
{
    if (m_toolTip.GetSafeHwnd())
        m_toolTip.RelayEvent(pMsg);
    return CScrollView::PreTranslateMessage(pMsg);
}

// WM_VISUALIZATION_READY: AnalysisFlow에서 시각화 카드 생성
LRESULT CDashboardView::OnVisualizationReady(WPARAM wParam, LPARAM lParam)
{
    AnalysisFlow* pFlow = reinterpret_cast<AnalysisFlow*>(lParam);
    if (!pFlow)
    {
        InvalidateRect(nullptr);
        return 0;
    }

    // AnalysisFlow → VisualizationInfo 변환
    VisualizationInfo viz;

    // 고유 ID 생성 (타임스탬프 기반)
    viz.id.Format(_T("viz_%I64d"), (INT64)::GetTickCount64());

    // 차트 설정 반영 (ChartSelector가 이미 dataJson을 차트 형식으로 변환함)
    viz.chartConfig = pFlow->chartConfig;
    viz.vizType     = pFlow->chartConfig.chartType;

    // 제목: 인사이트 또는 질문
    if (!pFlow->insight.IsEmpty())
        viz.title = pFlow->insight;
    else
        viz.title = pFlow->question;

    AddVisualization(viz);

    delete pFlow;
    return 0;
}
