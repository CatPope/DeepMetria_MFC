#include "stdafx.h"
#include "ChartView.h"

// ChartRenderer 연동 (구현 완료 후 활성화)
// #include "../Renderer/ChartRenderer.h"

// ============================================================
// IMPLEMENT_DYNCREATE / 메시지 맵
// ============================================================
IMPLEMENT_DYNCREATE(CChartView, CView)

BEGIN_MESSAGE_MAP(CChartView, CView)
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_WM_SIZE()
END_MESSAGE_MAP()

// ============================================================
// 생성 / 소멸
// ============================================================
CChartView::CChartView()
    : CView()
{
}

CChartView::~CChartView()
{
}

// ============================================================
// 공개 인터페이스
// ============================================================
void CChartView::SetChartConfig(const ChartConfig& config, const CString& data)
{
    m_chartConfig = config;
    m_chartData   = data;
    Invalidate();
}

// ============================================================
// 초기화
// ============================================================
void CChartView::OnInitialUpdate()
{
    CView::OnInitialUpdate();

    // 툴팁 초기화
    m_toolTip.Create(this);
    m_toolTip.Activate(TRUE);
    m_toolTip.AddTool(this, _T(""));
}

BOOL CChartView::PreTranslateMessage(MSG* pMsg)
{
    if (::IsWindow(m_toolTip.GetSafeHwnd()))
        m_toolTip.RelayEvent(pMsg);
    return CView::PreTranslateMessage(pMsg);
}

// ============================================================
// 그리기
// ============================================================
void CChartView::OnDraw(CDC* pDC)
{
    CRect clientRect;
    GetClientRect(&clientRect);

    // 배경
    pDC->FillSolidRect(&clientRect, RGB(255, 255, 255));

    if (m_chartConfig.chartType.IsEmpty())
    {
        // 차트 미설정 안내
        pDC->SetTextColor(RGB(160, 160, 160));
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(_T("차트 데이터가 없습니다."), &clientRect,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    // 제목 출력
    if (!m_chartConfig.title.IsEmpty())
    {
        CRect titleRect = clientRect;
        titleRect.bottom = titleRect.top + 36;

        CFont titleFont;
        LOGFONT lf = {};
        lf.lfHeight = -18;
        lf.lfWeight = FW_BOLD;
        _tcscpy_s(lf.lfFaceName, _T("Segoe UI"));
        titleFont.CreateFontIndirect(&lf);
        CFont* pOldFont = pDC->SelectObject(&titleFont);

        pDC->SetTextColor(RGB(32, 32, 32));
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(m_chartConfig.title, &titleRect,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        pDC->SelectObject(pOldFont);
    }

    // 차트 렌더링 영역
    CRect chartRect = clientRect;
    chartRect.top   += 40;
    chartRect.DeflateRect(16, 16);

    // ChartRenderer 연동 (구현 완료 후 활성화)
    // ChartRenderer::Render(pDC, m_chartConfig, chartRect);

    // 임시: 차트 영역 테두리
    CPen pen(PS_DASH, 1, RGB(180, 180, 180));
    CPen* pOld = pDC->SelectObject(&pen);
    pDC->SelectStockObject(NULL_BRUSH);
    pDC->Rectangle(chartRect);
    pDC->SelectObject(pOld);

    CString hint;
    hint.Format(_T("[%s 차트]"), (LPCTSTR)m_chartConfig.chartType);
    pDC->SetTextColor(RGB(160, 160, 160));
    pDC->SetBkMode(TRANSPARENT);
    pDC->DrawText(hint, &chartRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// 메시지 핸들러
// ============================================================
void CChartView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);
    Invalidate();
}

void CChartView::OnMouseMove(UINT nFlags, CPoint point)
{
    UpdateTooltip(point);
    m_ptLastMouse = point;
    CView::OnMouseMove(nFlags, point);
}

BOOL CChartView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    ::SetCursor(::LoadCursor(nullptr, IDC_CROSS));
    return TRUE;
}

// ============================================================
// 툴팁 처리
// ============================================================
void CChartView::UpdateTooltip(CPoint pt)
{
    CString text = BuildTooltipText(pt);
    if (text != m_tooltipText)
    {
        m_tooltipText = text;
        if (::IsWindow(m_toolTip.GetSafeHwnd()))
        {
            m_toolTip.UpdateTipText(m_tooltipText, this);
            m_toolTip.Activate(!m_tooltipText.IsEmpty());
        }
    }
}

CString CChartView::BuildTooltipText(CPoint pt) const
{
    // ChartRenderer가 구현되면 히트 테스트로 데이터 값 조회
    // 현재는 좌표를 그대로 표시
    CString text;
    text.Format(_T("(%d, %d)"), pt.x, pt.y);
    return text;
}
