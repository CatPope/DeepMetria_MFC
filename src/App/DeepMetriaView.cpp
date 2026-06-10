// DeepMetriaView.cpp - 대시보드 렌더링 (GDI+ 기반)
#include "stdafx.h"
#include "DeepMetria.h"
#include "DeepMetriaView.h"
#include "MainFrame.h"
#include "ChartRenderer.h"
#include "Messages.h"
#include "DataSource.h"
#include "AnalysisTool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 색상 상수
static const Gdiplus::Color kColorPrimary(0xFF, 0x25, 0x63, 0xEB);  // #2563EB
static const Gdiplus::Color kColorBg(0xFF, 0xFF, 0xFF, 0xFF);        // 흰색
static const Gdiplus::Color kColorCanvasBg(0xFF, 0xF3, 0xF4, 0xF6);  // 격자 배경
static const Gdiplus::Color kColorGrid(0xFF, 0xE5, 0xE7, 0xEB);      // 격자 선
static const Gdiplus::Color kColorText(0xFF, 0x11, 0x18, 0x27);      // 본문 텍스트
static const Gdiplus::Color kColorSubText(0xFF, 0x6B, 0x72, 0x80);   // 보조 텍스트
static const Gdiplus::Color kColorSuccess(0xFF, 0x10, 0xB9, 0x81);   // #10B981
static const Gdiplus::Color kColorCard(0xFF, 0xFF, 0xFF, 0xFF);       // 카드 배경
static const Gdiplus::Color kColorCardBorder(0xFF, 0xE5, 0xE7, 0xEB);// 카드 경계

IMPLEMENT_DYNCREATE(CDeepMetriaView, CView)

BEGIN_MESSAGE_MAP(CDeepMetriaView, CView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_KEYDOWN()
    ON_WM_DROPFILES()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// 더블 버퍼링이 매 OnDraw 에서 전체 화면을 그리므로 배경 erase 는 불필요.
// 시스템 erase 를 막아 드래그/스크롤 깜빡임 제거.
BOOL CDeepMetriaView::OnEraseBkgnd(CDC* /*pDC*/)
{
    return TRUE;
}

CDeepMetriaView::CDeepMetriaView() = default;
CDeepMetriaView::~CDeepMetriaView() = default;

BOOL CDeepMetriaView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL;
    return CView::PreCreateWindow(cs);
}

int CDeepMetriaView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 드래그앤드롭 수신 등록
    DragAcceptFiles(TRUE);

    // 미리보기 표는 더 이상 CListCtrl 사용 안 함 — OnDraw에서 GDI+로 직접 렌더.
    // 사이드바 토글 시 ChildWindow 동기화 문제로 표가 사라지는 현상을 근본 차단.
    return 0;
}

// 외부에서 호출되는 진입점 유지 (no-op) — 데이터 갱신 시 Invalidate만 하면 됨
void CDeepMetriaView::RefreshPreviewList(const deepmetria::DataSource& /*ds*/)
{
    if (GetSafeHwnd()) Invalidate();
}

void CDeepMetriaView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);
    UpdateScrollBars();
}

// 가상 캔버스 크기 기준으로 스크롤 범위 설정 — 시각화 위치까지 자동 확장
void CDeepMetriaView::UpdateScrollBars()
{
    if (!GetSafeHwnd()) return;

    CRect rc; GetClientRect(&rc);

    // 기본 가상 캔버스 (클라이언트의 1.4배 / 1.6배)
    int virtualW = std::max((int)(rc.Width()  * 1.4), 1200);
    int virtualH = std::max((int)(rc.Height() * 1.6), 900);

    // 모든 시각화의 우/하단 영역까지 확장 — 화면 밖으로 나간 viz를 스크롤로 따라갈 수 있게
    if (CDeepMetriaDoc* pDoc = GetDocument())
    {
        for (const auto& v : pDoc->GetDashboard().Visualizations())
        {
            int right  = v.x + v.width  + 64;  // 약간의 패딩
            int bottom = v.y + v.height + 64;
            if (right  > virtualW) virtualW = right;
            if (bottom > virtualH) virtualH = bottom;
        }
    }

    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE;

    si.nMin   = 0;
    si.nMax   = virtualW;
    si.nPage  = rc.Width();
    SetScrollInfo(SB_HORZ, &si, TRUE);

    si.nMin   = 0;
    si.nMax   = virtualH;
    si.nPage  = rc.Height();
    SetScrollInfo(SB_VERT, &si, TRUE);
}

void CDeepMetriaView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* /*sb*/)
{
    SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_ALL;
    GetScrollInfo(SB_HORZ, &si);
    int cur = si.nPos;
    int line = ScrollLineH();
    switch (nSBCode)
    {
    case SB_LINELEFT:    cur -= line; break;
    case SB_LINERIGHT:   cur += line; break;
    case SB_PAGELEFT:    cur -= si.nPage; break;
    case SB_PAGERIGHT:   cur += si.nPage; break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: cur = nPos; break;
    case SB_LEFT:        cur = 0; break;
    case SB_RIGHT:       cur = si.nMax; break;
    }
    cur = std::max(0, std::min(cur, (int)(si.nMax - si.nPage + 1)));
    SetScrollPos(SB_HORZ, cur, TRUE);
    OnSize(0, 0, 0);  // 자식 컨트롤 재배치
    Invalidate();
}

void CDeepMetriaView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* /*sb*/)
{
    SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_ALL;
    GetScrollInfo(SB_VERT, &si);
    int cur = si.nPos;
    int line = ScrollLineV();
    switch (nSBCode)
    {
    case SB_LINEUP:      cur -= line; break;
    case SB_LINEDOWN:    cur += line; break;
    case SB_PAGEUP:      cur -= si.nPage; break;
    case SB_PAGEDOWN:    cur += si.nPage; break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: cur = nPos; break;
    case SB_TOP:         cur = 0; break;
    case SB_BOTTOM:      cur = si.nMax; break;
    }
    cur = std::max(0, std::min(cur, (int)(si.nMax - si.nPage + 1)));
    SetScrollPos(SB_VERT, cur, TRUE);
    OnSize(0, 0, 0);
    Invalidate();
}

BOOL CDeepMetriaView::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
    int steps = zDelta / WHEEL_DELTA;
    UINT msg = (steps > 0) ? SB_LINEUP : SB_LINEDOWN;
    for (int i = 0; i < std::abs(steps) * 3; ++i)
        OnVScroll(msg, 0, nullptr);
    return TRUE;
}

void CDeepMetriaView::SetActiveTab(deepmetria::CRibbonBar::ActiveTab tab)
{
    m_activeTab = tab;
    if (m_listPreview.GetSafeHwnd())
    {
        bool showList = (tab == deepmetria::CRibbonBar::ActiveTab::Summary);
        m_listPreview.ShowWindow(showList ? SW_SHOW : SW_HIDE);
    }
    Invalidate();
}

// -----------------------------------------------------------------------
// OnDraw — 탭별 렌더링 분기
// -----------------------------------------------------------------------
void CDeepMetriaView::OnDraw(CDC* pDC)
{
    CDeepMetriaDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (!pDoc) return;

    CRect client;
    GetClientRect(&client);
    int W = client.Width();
    int H = client.Height();

    // 더블 버퍼링
    CDC memDC;
    memDC.CreateCompatibleDC(pDC);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(pDC, W, H);
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);

    Gdiplus::Graphics g(memDC.GetSafeHdc());
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    // 전체 배경
    Gdiplus::SolidBrush bgBrush(kColorBg);
    g.FillRectangle(&bgBrush, 0, 0, W, H);

    // 스크롤 offset 적용 (가상 캔버스 좌표 ↔ 화면 좌표)
    int scrollX = GetScrollPos(SB_HORZ);
    int scrollY = GetScrollPos(SB_VERT);
    g.TranslateTransform(static_cast<Gdiplus::REAL>(-scrollX),
                         static_cast<Gdiplus::REAL>(-scrollY));

    if (pDoc->GetDataSource().IsEmpty())
    {
        DrawEmpty(g, W + scrollX, H + scrollY);
    }
    else
    {
        // 메인 화면 = 항상 대시보드
        DrawSummaryTab(g, W + scrollX, H + scrollY, pDoc);

        // Format 탭 활성 + viz 선택 시 8-핸들 오버레이
        if (m_activeTab == deepmetria::CRibbonBar::ActiveTab::Format &&
            m_selectedViz >= 0)
        {
            const auto& vizList = pDoc->GetDashboard().Visualizations();
            if (m_selectedViz < static_cast<int>(vizList.size()))
                DrawResizeHandles(g, vizList[m_selectedViz]);
        }
    }

    pDC->BitBlt(0, 0, W, H, &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);

    // 시각화 hover 툴팁 — viz 카드 자체를 가리지 않도록 진입 방향 기반 배치
    if (m_hoverVizIndex >= 0)
    {
        const auto& vizList = pDoc->GetDashboard().Visualizations();
        if (m_hoverVizIndex < (int)vizList.size())
        {
            const auto& hv = vizList[m_hoverVizIndex];
            CString tip = hv.description.empty()
                ? CString(hv.title.c_str())
                : (CString(hv.title.c_str()) + _T("\n") + CString(hv.description.c_str()));
            if (!tip.IsEmpty())
            {
                Gdiplus::Graphics tg(pDC->GetSafeHdc());
                tg.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                tg.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

                Gdiplus::FontFamily ff(L"맑은 고딕");
                Gdiplus::Font font(&ff, 9, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);

                // 가변 크기 — 텍스트 길이에 따라 박스 자동 확장.
                // 최대 너비는 viewport 60% (단, 240~520 px 범위 클램프) 로 제한해
                // 매우 긴 한 줄이 화면 끝까지 가는 것을 막고 줄바꿈 유도.
                const int padX = 12, padY = 10;
                const int gap  = 12;
                int maxTipW = std::max(240, std::min(520, (int)(W * 0.60)));
                int innerMax = maxTipW - padX * 2;

                Gdiplus::StringFormat sf;
                sf.SetFormatFlags(0);  // 줄바꿈 허용 (NoWrap 해제)
                Gdiplus::RectF measureRc(0, 0, (Gdiplus::REAL)innerMax, 4000.f);
                Gdiplus::RectF resultRc;
                tg.MeasureString(tip, -1, &font, measureRc, &sf, &resultRc);

                int tipW = std::min(maxTipW, (int)std::ceil(resultRc.Width)  + padX * 2);
                int tipH =                  (int)std::ceil(resultRc.Height) + padY * 2;
                // 최소 한 줄 보장
                if (tipH < 32) tipH = 32;
                if (tipW < 80) tipW = 80;

                // viz 카드의 화면 좌표 (가상 좌표 → 화면 좌표)
                int sx = GetScrollPos(SB_HORZ);
                int sy = GetScrollPos(SB_VERT);
                CRect cardScreen(hv.x - sx, hv.y - sy,
                                 hv.x - sx + hv.width, hv.y - sy + hv.height);

                // 마우스가 카드 어느 변에 가까운지 → 그 변 바깥에 툴팁 배치
                int dTop    = m_hoverPoint.y - cardScreen.top;
                int dBottom = cardScreen.bottom - m_hoverPoint.y;
                int dLeft   = m_hoverPoint.x - cardScreen.left;
                int dRight  = cardScreen.right - m_hoverPoint.x;
                int minD = std::min({ dTop, dBottom, dLeft, dRight });

                int tx, ty;
                if (minD == dTop)
                {
                    tx = m_hoverPoint.x - tipW / 2;
                    ty = cardScreen.top - tipH - gap;
                }
                else if (minD == dRight)
                {
                    tx = cardScreen.right + gap;
                    ty = m_hoverPoint.y - tipH / 2;
                }
                else if (minD == dLeft)
                {
                    tx = cardScreen.left - tipW - gap;
                    ty = m_hoverPoint.y - tipH / 2;
                }
                else  // dBottom
                {
                    tx = m_hoverPoint.x - tipW / 2;
                    ty = cardScreen.bottom + gap;
                }

                // 화면 밖이면 안쪽으로 보정
                if (tx + tipW > W) tx = W - tipW - 4;
                if (ty + tipH > H) ty = H - tipH - 4;
                if (tx < 4) tx = 4;
                if (ty < 4) ty = 4;

                Gdiplus::SolidBrush bg(Gdiplus::Color(240, 30, 41, 59));   // 어두운 슬레이트 (alpha)
                Gdiplus::Pen        border(Gdiplus::Color(220, 100, 116, 139), 1.0f);
                Gdiplus::Rect r(tx, ty, tipW, tipH);
                tg.FillRectangle(&bg, r);
                tg.DrawRectangle(&border, r);

                Gdiplus::SolidBrush text(Gdiplus::Color(255, 248, 250, 252));
                Gdiplus::RectF rcText(
                    (float)(tx + padX), (float)(ty + padY),
                    (float)(tipW - padX * 2), (float)(tipH - padY * 2));
                tg.DrawString(tip, -1, &font, rcText, &sf, &text);
            }
        }
    }
}

// -----------------------------------------------------------------------
// 빈 캔버스 — 드롭존
// -----------------------------------------------------------------------
void CDeepMetriaView::DrawEmpty(Gdiplus::Graphics& g, int W, int H)
{
    DrawGridBackground(g, W, H);

    // 중앙 점선 박스
    float bw = 360.0f, bh = 180.0f;
    float bx = (W - bw) / 2.0f;
    float by = (H - bh) / 2.0f;

    Gdiplus::Pen dashPen(Gdiplus::Color(0xFF, 0xD1, 0xD5, 0xDB), 2.0f);
    dashPen.SetDashStyle(Gdiplus::DashStyleDash);
    g.DrawRectangle(&dashPen, bx, by, bw, bh);

    // 아이콘 (간단한 업로드 화살표)
    Gdiplus::SolidBrush iconBrush(Gdiplus::Color(0xFF, 0x9C, 0xA3, 0xAF));
    Gdiplus::PointF arrowPts[] = {
        {W/2.0f,       by + 40},
        {W/2.0f - 20,  by + 65},
        {W/2.0f - 8,   by + 65},
        {W/2.0f - 8,   by + 85},
        {W/2.0f + 8,   by + 85},
        {W/2.0f + 8,   by + 65},
        {W/2.0f + 20,  by + 65},
    };
    g.FillPolygon(&iconBrush, arrowPts, 7);

    // 안내 텍스트
    Gdiplus::FontFamily ff(L"맑은 고딕");
    Gdiplus::Font fontMain(&ff, 13, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
    Gdiplus::Font fontSub(&ff, 10, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);

    Gdiplus::SolidBrush textBrush(kColorText);
    Gdiplus::SolidBrush subBrush(kColorSubText);

    Gdiplus::StringFormat sf;
    sf.SetAlignment(Gdiplus::StringAlignmentCenter);

    Gdiplus::RectF rcMain(bx, by + 95, bw, 24);
    g.DrawString(L"여기로 끌어다 놓거나 [열기] 클릭", -1, &fontMain, rcMain, &sf, &textBrush);

    Gdiplus::RectF rcSub(bx, by + 122, bw, 20);
    g.DrawString(L"CSV · JSON 파일 지원", -1, &fontSub, rcSub, &sf, &subBrush);
}

// -----------------------------------------------------------------------
// 격자 배경
// -----------------------------------------------------------------------
void CDeepMetriaView::DrawGridBackground(Gdiplus::Graphics& g, int W, int H)
{
    Gdiplus::SolidBrush canvasBg(kColorCanvasBg);
    g.FillRectangle(&canvasBg, 0, 0, W, H);

    Gdiplus::Pen gridPen(kColorGrid, 1.0f);
    constexpr int cellSize = 24;
    for (int x = 0; x < W; x += cellSize)
        g.DrawLine(&gridPen, x, 0, x, H);
    for (int y = 0; y < H; y += cellSize)
        g.DrawLine(&gridPen, 0, y, W, y);
}

// -----------------------------------------------------------------------
// KPI 카드 그리기
// -----------------------------------------------------------------------
void CDeepMetriaView::DrawKpiCard(Gdiplus::Graphics& g,
                                   float x, float y, float w, float h,
                                   LPCWSTR title, LPCWSTR value, LPCWSTR sub)
{
    // 카드 배경 (흰 배경 + 테두리)
    Gdiplus::SolidBrush cardBrush(kColorCard);
    Gdiplus::Pen borderPen(kColorCardBorder, 1.0f);
    Gdiplus::RectF rc(x, y, w, h);
    g.FillRectangle(&cardBrush, rc);
    g.DrawRectangle(&borderPen, rc);

    Gdiplus::FontFamily ff(L"맑은 고딕");
    Gdiplus::Font fontTitle(&ff, 9, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
    Gdiplus::Font fontValue(&ff, 18, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
    Gdiplus::Font fontSub  (&ff, 9,  Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);

    Gdiplus::SolidBrush subBrush(kColorSubText);
    Gdiplus::SolidBrush textBrush(kColorText);
    Gdiplus::SolidBrush greenBrush(kColorSuccess);

    Gdiplus::StringFormat sfLeft;
    sfLeft.SetAlignment(Gdiplus::StringAlignmentNear);

    float px = x + 14, py = y + 12;

    // 제목
    g.DrawString(title, -1, &fontTitle, Gdiplus::RectF(px, py, w-28, 16), &sfLeft, &subBrush);
    py += 20;
    // 값
    g.DrawString(value, -1, &fontValue, Gdiplus::RectF(px, py, w-28, 32), &sfLeft, &textBrush);
    py += 34;
    // 부제목 (성공 색상)
    g.DrawString(sub, -1, &fontSub, Gdiplus::RectF(px, py, w-28, 16), &sfLeft, &greenBrush);
}

// -----------------------------------------------------------------------
// 도넛 카드 그리기 — 컬럼 타입 비중(수치/텍스트/날짜)
// -----------------------------------------------------------------------
void CDeepMetriaView::DrawDonutCard(Gdiplus::Graphics& g,
                                     float x, float y, float w, float h,
                                     LPCWSTR title)
{
    Gdiplus::SolidBrush cardBrush(kColorCard);
    Gdiplus::Pen borderPen(kColorCardBorder, 1.0f);
    g.FillRectangle(&cardBrush, Gdiplus::RectF(x, y, w, h));
    g.DrawRectangle(&borderPen, Gdiplus::RectF(x, y, w, h));

    Gdiplus::FontFamily ff(L"맑은 고딕");
    Gdiplus::Font fontTitle(&ff, 9, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
    Gdiplus::SolidBrush subBrush(kColorSubText);
    Gdiplus::StringFormat sfLeft;
    sfLeft.SetAlignment(Gdiplus::StringAlignmentNear);
    g.DrawString(title, -1, &fontTitle, Gdiplus::RectF(x+14, y+12, w-28, 16), &sfLeft, &subBrush);

    // 활성 Document → DataSource 컬럼 타입 비중 계산
    int nNum = 0, nText = 0, nDate = 0;
    if (CDeepMetriaDoc* pDoc = GetDocument())
    {
        for (const auto& col : pDoc->GetDataSource().Columns())
        {
            switch (col.type)
            {
            case deepmetria::ColumnType::Number: ++nNum;  break;
            case deepmetria::ColumnType::Date:   ++nDate; break;
            default:                              ++nText; break;
            }
        }
    }
    int total = nNum + nText + nDate;
    if (total == 0)
    {
        // 데이터 없음 — 빈 도넛만 그리고 종료
        return;
    }
    float pNum  = static_cast<float>(nNum)  / total;
    float pText = static_cast<float>(nText) / total;
    float pDate = static_cast<float>(nDate) / total;

    float cr = std::min(w, h) * 0.28f;
    float cx = x + w * 0.45f;
    float cy = y + h * 0.55f;

    Gdiplus::SolidBrush blueBrush(kColorPrimary);
    Gdiplus::SolidBrush greenBrush(kColorSuccess);
    Gdiplus::SolidBrush grayBrush(Gdiplus::Color(0xFF, 0x9C, 0xA3, 0xAF));

    float a0 = -90.0f;
    float sweepNum  = 360.0f * pNum;
    float sweepText = 360.0f * pText;
    float sweepDate = 360.0f * pDate;
    g.FillPie(&blueBrush,  cx-cr, cy-cr, cr*2, cr*2, a0, sweepNum);
    g.FillPie(&greenBrush, cx-cr, cy-cr, cr*2, cr*2, a0 + sweepNum, sweepText);
    g.FillPie(&grayBrush,  cx-cr, cy-cr, cr*2, cr*2, a0 + sweepNum + sweepText, sweepDate);

    // 도넛 홀
    Gdiplus::SolidBrush holeBrush(kColorCard);
    float hr = cr * 0.6f;
    g.FillEllipse(&holeBrush, cx-hr, cy-hr, hr*2, hr*2);

    // 범례
    Gdiplus::Font fontLegend(&ff, 8, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
    float lx = x + w * 0.72f;
    float ly = y + h * 0.30f;

    Gdiplus::SolidBrush textBrush(kColorText);
    wchar_t buf[32];

    swprintf_s(buf, L"수치 %d", nNum);
    g.FillRectangle(&blueBrush, Gdiplus::RectF(lx, ly, 10, 10));
    g.DrawString(buf, -1, &fontLegend, Gdiplus::RectF(lx+14, ly-1, 60, 14), &sfLeft, &textBrush);
    ly += 14;

    swprintf_s(buf, L"텍스트 %d", nText);
    g.FillRectangle(&greenBrush, Gdiplus::RectF(lx, ly, 10, 10));
    g.DrawString(buf, -1, &fontLegend, Gdiplus::RectF(lx+14, ly-1, 60, 14), &sfLeft, &textBrush);
    ly += 14;

    swprintf_s(buf, L"날짜 %d", nDate);
    g.FillRectangle(&grayBrush, Gdiplus::RectF(lx, ly, 10, 10));
    g.DrawString(buf, -1, &fontLegend, Gdiplus::RectF(lx+14, ly-1, 60, 14), &sfLeft, &textBrush);
}

// -----------------------------------------------------------------------
// 대시보드 (메인 화면) — 항상 표시
// 데이터 없으면 빈 상태 안내, 있으면 미리보기 표 + 시각화 카드
// KPI / 도넛 / 컬럼 스키마 등 요약 정보는 좌측 사이드바(DataSummaryPane)로 이동
// -----------------------------------------------------------------------
void CDeepMetriaView::DrawSummaryTab(Gdiplus::Graphics& g, int W, int H,
                                      CDeepMetriaDoc* pDoc)
{
    Gdiplus::SolidBrush whiteBrush(kColorBg);
    g.FillRectangle(&whiteBrush, 0, 0, W, H);

    const deepmetria::DataSource& ds = pDoc->GetDataSource();
    if (ds.IsEmpty())
    {
        // 빈 캔버스 — 드롭존 안내만
        DrawEmpty(g, W, H);
        return;
    }

    // 미리보기 표 (GDI+ 직접 렌더 — ChildWindow 없음. 사이드바 토글에 안전)
    const int tableX = 8;
    const int tableY = 12;
    const int tableH = 180;
    const int tableW = W - 16;
    DrawPreviewTable(g, tableX, tableY, tableW, tableH, ds);
    // hit-test에 사용 — 마우스 좌표 → 가상 좌표 변환 후 이 영역과 비교
    m_tableRect = CRect(tableX, tableY, tableX + tableW, tableY + tableH);

    // 시각화 카드 렌더 (Dashboard.Visualizations() 가 있을 때만)
    if (!pDoc->GetDashboard().Visualizations().empty())
    {
        deepmetria::ChartRenderer renderer;
        CRect vizArea(0, tableY + tableH + 12, W, H);
        renderer.RenderDashboard(g, vizArea,
                                 pDoc->GetDataSource(), pDoc->GetDashboard());
    }
    else
    {
        Gdiplus::FontFamily ff(L"맑은 고딕");
        Gdiplus::Font fontGuide(&ff, 11, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
        Gdiplus::SolidBrush subBrush(kColorSubText);
        Gdiplus::StringFormat sfCenter;
        sfCenter.SetAlignment(Gdiplus::StringAlignmentCenter);
        g.DrawString(
            L"우측 [질문] 사이드바에 자연어로 질문하면\n시각화 카드가 여기에 표시됩니다.",
            -1, &fontGuide,
            Gdiplus::RectF(0, (float)(tableY + tableH + 60), (float)W, 60),
            &sfCenter, &subBrush);
    }
}

// 미리보기 표 — DataSource 의 상위 N행을 그리드로 직접 그림
void CDeepMetriaView::DrawPreviewTable(Gdiplus::Graphics& g,
                                       int x, int y, int w, int h,
                                       const deepmetria::DataSource& ds)
{
    Gdiplus::Pen borderPen(kColorCardBorder, 1.0f);
    Gdiplus::SolidBrush bgBrush(kColorBg);
    Gdiplus::SolidBrush headerBg(Gdiplus::Color(0xFF, 0xF8, 0xF9, 0xFB));
    Gdiplus::SolidBrush textBrush(kColorText);
    Gdiplus::SolidBrush subBrush(kColorSubText);
    Gdiplus::Pen gridPen(kColorCardBorder, 1.0f);

    g.FillRectangle(&bgBrush, x, y, w, h);
    g.DrawRectangle(&borderPen, x, y, w, h);

    Gdiplus::FontFamily ff(L"맑은 고딕");
    Gdiplus::Font fontHeader(&ff, 9, Gdiplus::FontStyleBold,    Gdiplus::UnitPoint);
    Gdiplus::Font fontCell  (&ff, 9, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);

    const int colN = std::min((int)ds.Columns().size(), 8);
    if (colN == 0) return;

    int colW = std::max(60, w / colN);
    const int headerH = 22;
    const int rowH    = 20;

    // 헤더 배경
    g.FillRectangle(&headerBg, x + 1, y + 1, w - 2, headerH);

    Gdiplus::StringFormat sf;
    sf.SetAlignment(Gdiplus::StringAlignmentNear);
    sf.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);

    // 컬럼 헤더
    for (int c = 0; c < colN; ++c)
    {
        const auto& col = ds.Columns()[c];
        Gdiplus::RectF rc((float)(x + 6 + c * colW), (float)(y + 4),
                          (float)(colW - 8), (float)(headerH - 4));
        g.DrawString(col.name.c_str(), -1, &fontHeader, rc, &sf, &textBrush);

        if (c > 0)
        {
            int sx = x + c * colW;
            g.DrawLine(&gridPen, sx, y + 1, sx, y + h - 1);
        }
    }
    // 헤더 하단 구분선
    g.DrawLine(&gridPen, x + 1, y + headerH, x + w - 1, y + headerH);

    // 데이터 행
    int rowCapacity = (h - headerH) / rowH;
    int rowN = std::min((int)ds.Rows().size(), rowCapacity);

    for (int r = 0; r < rowN; ++r)
    {
        int ry = y + headerH + r * rowH;
        const auto& row = ds.Rows()[r];
        for (int c = 0; c < colN && c < (int)row.size(); ++c)
        {
            Gdiplus::RectF rc((float)(x + 6 + c * colW), (float)(ry + 2),
                              (float)(colW - 8), (float)(rowH - 4));
            g.DrawString(row[c].c_str(), -1, &fontCell, rc, &sf, &textBrush);
        }
        if (r > 0)
            g.DrawLine(&gridPen, x + 1, ry, x + w - 1, ry);
    }

    if ((int)ds.Rows().size() > rowN)
    {
        wchar_t more[64];
        swprintf_s(more, L"… (총 %d행 중 %d행 표시)",
                   (int)ds.Rows().size(), rowN);
        Gdiplus::RectF rc((float)(x + 6), (float)(y + h - rowH + 2),
                          (float)(w - 12), (float)(rowH - 4));
        g.DrawString(more, -1, &fontCell, rc, &sf, &subBrush);
    }
}

// -----------------------------------------------------------------------
// Query 탭 — CoT 카드 (슬라이드 4) — 실제 ChainOfThinking 기반
// -----------------------------------------------------------------------
void CDeepMetriaView::DrawQueryTab(Gdiplus::Graphics& g, int W, int H,
                                    CDeepMetriaDoc* pDoc)
{
    DrawGridBackground(g, W, H);

    // CoT 카드
    float cardX = W * 0.06f;
    float cardY = H * 0.08f;
    float cardW = W * 0.58f;
    float cardH = H * 0.72f;

    Gdiplus::SolidBrush cardBrush(kColorCard);
    Gdiplus::Pen borderPen(kColorCardBorder, 1.0f);
    g.FillRectangle(&cardBrush, Gdiplus::RectF(cardX, cardY, cardW, cardH));
    g.DrawRectangle(&borderPen, Gdiplus::RectF(cardX, cardY, cardW, cardH));

    Gdiplus::FontFamily ff(L"맑은 고딕");
    Gdiplus::Font fontTitle(&ff, 12, Gdiplus::FontStyleBold,    Gdiplus::UnitPoint);
    Gdiplus::Font fontCode (&ff, 9,  Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);

    Gdiplus::SolidBrush textBrush(kColorText);
    Gdiplus::SolidBrush subBrush(kColorSubText);
    Gdiplus::SolidBrush greenBrush(kColorSuccess);
    Gdiplus::SolidBrush blueBrush(kColorPrimary);
    Gdiplus::SolidBrush grayBrush(Gdiplus::Color(0xFF, 0xD1, 0xD5, 0xDB));

    Gdiplus::StringFormat sfLeft;
    sfLeft.SetAlignment(Gdiplus::StringAlignmentNear);

    // 카드 헤더
    float px = cardX + 16, py = cardY + 14;
    g.DrawString(L"AI 분석 진행 — Chain of Thinking", -1, &fontTitle,
                 Gdiplus::RectF(px, py, cardW-100, 22), &sfLeft, &textBrush);

    // 진행률 배지 (우상단)
    wchar_t badgeText[32];
    if (m_progressPct < 0)
        swprintf_s(badgeText, L"대기");
    else if (m_progressPct >= 100)
        swprintf_s(badgeText, L"완료");
    else
        swprintf_s(badgeText, L"%d%%", m_progressPct);

    Gdiplus::SolidBrush badgeBg(Gdiplus::Color(0xFF, 0xEF, 0xF6, 0xFF));
    Gdiplus::Pen badgeBorder(kColorPrimary, 1.0f);
    float bx = cardX + cardW - 90, by2 = cardY + 12;
    g.FillRectangle(&badgeBg, Gdiplus::RectF(bx, by2, 76, 18));
    g.DrawRectangle(&badgeBorder, Gdiplus::RectF(bx, by2, 76, 18));
    g.DrawString(badgeText, -1, &fontCode,
                 Gdiplus::RectF(bx+4, by2+2, 72, 14), &sfLeft, &blueBrush);

    py += 30;

    // 실 CoT 단계 (없으면 안내 메시지)
    const deepmetria::ChainOfThinking* cot = pDoc->GetCurrentReasoning();
    if (!cot || cot->Steps().empty())
    {
        Gdiplus::Font fStep(&ff, 10, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
        g.DrawString(
            L"우측 질문 패널에 자연어로 질문하고 [분석 실행] 버튼을 누르면\n"
            L"CoT 단계가 실시간으로 표시됩니다.",
            -1, &fStep,
            Gdiplus::RectF(px, py, cardW-32, cardH-60), &sfLeft, &subBrush);
        return;
    }

    Gdiplus::SolidBrush whiteBrush2(kColorCard);
    Gdiplus::Font fontMark(&ff, 9, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
    Gdiplus::Font fStep(&ff, 10, Gdiplus::FontStyleBold,    Gdiplus::UnitPoint);
    Gdiplus::Font fDetail(&ff, 9, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);

    const auto& steps = cot->Steps();
    int total = static_cast<int>(steps.size());
    bool completed = (m_progressPct >= 100);

    for (int i = 0; i < total; ++i)
    {
        const auto& step = steps[i];

        // 단계 상태: 마지막 단계가 진행 중(완료 아님)이면 파랑, 완료면 초록
        bool isLast = (i == total - 1);
        Gdiplus::Color color = (isLast && !completed) ? kColorPrimary : kColorSuccess;

        wchar_t mark[8];
        if (isLast && !completed)
            swprintf_s(mark, L"%d", step.index);
        else
            swprintf_s(mark, L"✓");

        // 원형 마커
        Gdiplus::SolidBrush markerBrush(color);
        g.FillEllipse(&markerBrush, Gdiplus::RectF(px, py, 20, 20));
        g.DrawString(mark, -1, &fontMark,
                     Gdiplus::RectF(px, py+2, 20, 16), nullptr, &whiteBrush2);

        // 제목 (thought)
        Gdiplus::SolidBrush* pStepBrush =
            (isLast && !completed) ? &blueBrush : &textBrush;
        g.DrawString(step.thought.c_str(), -1, &fStep,
                     Gdiplus::RectF(px+28, py, cardW-50, 18), &sfLeft, pStepBrush);

        // 부제 (action · observation)
        std::wstring detail = step.action;
        if (!step.observation.empty())
        {
            if (!detail.empty()) detail += L" · ";
            detail += step.observation;
        }
        g.DrawString(detail.c_str(), -1, &fDetail,
                     Gdiplus::RectF(px+28, py+18, cardW-50, 16), &sfLeft, &subBrush);

        py += 48;
        if (py > cardY + cardH - 50) break;  // 카드 밖 단계는 생략
    }
}

// -----------------------------------------------------------------------
// 시각화 탭 — 5-grid (슬라이드 5/6)
// -----------------------------------------------------------------------
void CDeepMetriaView::DrawVisualizationTab(Gdiplus::Graphics& g, int W, int H,
                                            CDeepMetriaDoc* pDoc)
{
    DrawGridBackground(g, W, H);

    // 대시보드에 시각화가 있으면 ChartRenderer로 렌더
    if (!pDoc->GetDashboard().Visualizations().empty())
    {
        deepmetria::ChartRenderer renderer;
        CRect full(0, 0, W, H);
        renderer.RenderDashboard(g, full, pDoc->GetDataSource(), pDoc->GetDashboard());

        // Format 탭이면 선택된 시각화 외곽선 + 8핸들
        if (m_activeTab == deepmetria::CRibbonBar::ActiveTab::Format &&
            m_selectedViz >= 0)
        {
            auto& vizList = pDoc->GetDashboard().Visualizations();
            if (m_selectedViz < static_cast<int>(vizList.size()))
                DrawResizeHandles(g, vizList[m_selectedViz]);
        }
        return;
    }

    // 시각화 없을 때 — 빈 상태 안내
    Gdiplus::FontFamily ff(L"맑은 고딕");
    Gdiplus::Font fontGuide(&ff, 11, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
    Gdiplus::SolidBrush subBrush(kColorSubText);
    Gdiplus::StringFormat sfCenter;
    sfCenter.SetAlignment(Gdiplus::StringAlignmentCenter);
    g.DrawString(
        L"우측 질문 패널에 자연어로 질문하면\n분석 결과 시각화가 여기에 표시됩니다.",
        -1, &fontGuide,
        Gdiplus::RectF(0, H * 0.4f, (float)W, 80),
        &sfCenter, &subBrush);
}

// -----------------------------------------------------------------------
// 8방향 리사이즈 핸들
// -----------------------------------------------------------------------
void CDeepMetriaView::DrawResizeHandles(Gdiplus::Graphics& g,
                                         const deepmetria::Visualization& viz)
{
    // 선택 외곽선
    Gdiplus::Pen selPen(kColorPrimary, 2.0f);
    g.DrawRectangle(&selPen,
                    (float)viz.x, (float)viz.y,
                    (float)viz.width, (float)viz.height);

    // 8개 핸들 위치
    float hx = (float)viz.x, hy = (float)viz.y;
    float hw = (float)viz.width, hh = (float)viz.height;
    float hs = 8.0f;  // 핸들 사이즈

    Gdiplus::PointF handles[] = {
        {hx,           hy          }, // 좌상
        {hx + hw/2,    hy          }, // 상중
        {hx + hw - hs, hy          }, // 우상
        {hx,           hy + hh/2   }, // 좌중
        {hx + hw - hs, hy + hh/2   }, // 우중
        {hx,           hy + hh - hs}, // 좌하
        {hx + hw/2,    hy + hh - hs}, // 하중
        {hx + hw - hs, hy + hh - hs}, // 우하
    };

    Gdiplus::SolidBrush handleFill(kColorCard);
    Gdiplus::Pen handleBorder(kColorPrimary, 1.5f);
    for (auto& pt : handles)
    {
        g.FillRectangle(&handleFill, Gdiplus::RectF(pt.X, pt.Y, hs, hs));
        g.DrawRectangle(&handleBorder, Gdiplus::RectF(pt.X, pt.Y, hs, hs));
    }
}

// -----------------------------------------------------------------------
// 마우스 / 입력 핸들러
// -----------------------------------------------------------------------
void CDeepMetriaView::OnLButtonDown(UINT nFlags, CPoint point)
{
    SetFocus();
    SetCapture();

    // 스크롤 offset 보정 — 화면 좌표 → 가상 좌표
    CPoint vp(point.x + GetScrollPos(SB_HORZ),
              point.y + GetScrollPos(SB_VERT));
    m_dragOrigin = vp;

    auto& dash = GetDocument()->GetDashboard();

    // 선택 viz의 8핸들 hit-test (가장 우선) — Format 사이드바 활성 시
    if (m_selectedViz >= 0 &&
        m_selectedViz < (int)dash.Visualizations().size())
    {
        const auto& sviz = dash.Visualizations()[m_selectedViz];
        const int hs = 10;  // 핸들 hit-area (조금 큼)
        struct Handle { int x, y; DragMode mode; };
        Handle handles[] = {
            { sviz.x,                      sviz.y,                        DragMode::ResizeTL },
            { sviz.x + sviz.width/2 - hs/2,sviz.y,                        DragMode::ResizeT  },
            { sviz.x + sviz.width  - hs,   sviz.y,                        DragMode::ResizeTR },
            { sviz.x,                      sviz.y + sviz.height/2 - hs/2, DragMode::ResizeL  },
            { sviz.x + sviz.width  - hs,   sviz.y + sviz.height/2 - hs/2, DragMode::ResizeR  },
            { sviz.x,                      sviz.y + sviz.height - hs,     DragMode::ResizeBL },
            { sviz.x + sviz.width/2 - hs/2,sviz.y + sviz.height - hs,     DragMode::ResizeB  },
            { sviz.x + sviz.width  - hs,   sviz.y + sviz.height - hs,     DragMode::ResizeBR },
        };
        for (auto& h : handles)
        {
            CRect hr(h.x, h.y, h.x + hs, h.y + hs);
            if (hr.PtInRect(vp))
            {
                m_dragMode = h.mode;
                m_dragVizIndex = m_selectedViz;
                CView::OnLButtonDown(nFlags, point);
                return;
            }
        }
    }

    // 표 영역 hit-test 두번째 — 드래그하면 새 시각화 생성
    if (m_tableRect.PtInRect(vp))
    {
        m_dragMode = DragMode::TableSpawn;
        CView::OnLButtonDown(nFlags, point);
        return;
    }

    m_dragVizIndex = dash.HitTest(vp.x, vp.y);
    if (m_dragVizIndex >= 0)
    {
        m_selectedViz = m_dragVizIndex;
        m_dragMode = DragMode::Move;

        if (CFrameWnd* pFrame = GetParentFrame())
            pFrame->PostMessage(deepmetria::WM_USER_VIZ_SELECTED,
                                static_cast<WPARAM>(m_selectedViz), 0);
        Invalidate();
    }
    else
    {
        // 빈 영역 클릭 — viz 선택 해제 신호 → Format 탭 비활성화
        if (m_selectedViz >= 0)
        {
            m_selectedViz = -1;
            if (CFrameWnd* pFrame = GetParentFrame())
                pFrame->PostMessage(deepmetria::WM_USER_VIZ_SELECTED,
                                    static_cast<WPARAM>(-1), 0);
            Invalidate();
        }
    }
    CView::OnLButtonDown(nFlags, point);
}

void CDeepMetriaView::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (GetCapture() == this) ReleaseCapture();

    // 표 영역에서 드래그 후 표 밖에서 떼면 새 Table 시각화 생성
    if (m_dragMode == DragMode::TableSpawn)
    {
        CPoint vp(point.x + GetScrollPos(SB_HORZ),
                  point.y + GetScrollPos(SB_VERT));
        bool stillInsideTable = m_tableRect.PtInRect(vp);

        if (!stillInsideTable)
        {
            // Drop 위치 기준으로 새 Table viz 생성. 기존 viz와 겹치지 않게 자동 배치.
            CDeepMetriaDoc* pDoc = GetDocument();
            if (pDoc)
            {
                deepmetria::Visualization viz =
                    deepmetria::AnalysisTool::TableSample(pDoc->GetDataSource(), 20);
                viz.title = L"표 사본";
                viz.width  = 560;
                viz.height = 320;

                // 시작 위치: drop 좌표 — 표 영역 바로 아래로 보정
                int desiredX = vp.x - viz.width / 2;
                int desiredY = vp.y - 20;
                if (desiredX < 16) desiredX = 16;
                if (desiredY < m_tableRect.bottom + 16)
                    desiredY = m_tableRect.bottom + 16;

                viz.x = desiredX;
                viz.y = desiredY;

                // 겹침 회피 — 기존 viz 와 부딪치면 아래로 이동
                auto& dash = pDoc->GetDashboard();
                bool collided = true;
                while (collided)
                {
                    collided = false;
                    CRect r1(viz.x, viz.y, viz.x + viz.width, viz.y + viz.height);
                    for (const auto& exist : dash.Visualizations())
                    {
                        CRect r2(exist.x, exist.y,
                                 exist.x + exist.width, exist.y + exist.height);
                        CRect inter;
                        if (inter.IntersectRect(r1, r2))
                        {
                            viz.y = exist.y + exist.height + 16;
                            collided = true;
                            break;
                        }
                    }
                }

                dash.Add(viz);
                // 가상 캔버스 스크롤 범위 확장
                UpdateScrollBars();
                // 새 viz가 viewport 밖이면 자동 스크롤
                CRect cli; GetClientRect(&cli);
                int needX = std::max(0, viz.x + viz.width  - cli.Width());
                int needY = std::max(0, viz.y + viz.height - cli.Height());
                if (GetScrollPos(SB_HORZ) < needX) SetScrollPos(SB_HORZ, needX, TRUE);
                if (GetScrollPos(SB_VERT) < needY) SetScrollPos(SB_VERT, needY, TRUE);
                Invalidate();
            }
        }

        m_dragMode = DragMode::None;
        m_dragVizIndex = -1;
        CView::OnLButtonUp(nFlags, point);
        return;
    }

    m_dragMode     = DragMode::None;
    m_dragVizIndex = -1;
    CView::OnLButtonUp(nFlags, point);
}

void CDeepMetriaView::OnMouseMove(UINT nFlags, CPoint point)
{
    // 드래그 중이 아닐 때 hover 감지
    if (m_dragMode == DragMode::None)
    {
        CPoint vp(point.x + GetScrollPos(SB_HORZ),
                  point.y + GetScrollPos(SB_VERT));
        int hit = -1;
        if (CDeepMetriaDoc* pDoc = GetDocument())
            hit = pDoc->GetDashboard().HitTest(vp.x, vp.y);
        if (hit != m_hoverVizIndex)
        {
            m_hoverVizIndex = hit;
            m_hoverPoint = point;
            Invalidate();
        }
        else if (hit >= 0)
        {
            m_hoverPoint = point;
        }
    }

    if (m_dragMode != DragMode::None &&
        m_dragMode != DragMode::TableSpawn &&
        m_dragVizIndex >= 0)
    {
        CPoint vp(point.x + GetScrollPos(SB_HORZ),
                  point.y + GetScrollPos(SB_VERT));
        auto& viz = GetDocument()->GetDashboard().Visualizations()[m_dragVizIndex];
        int dx = vp.x - m_dragOrigin.x;
        int dy = vp.y - m_dragOrigin.y;
        const int kMinW = 80, kMinH = 60;

        switch (m_dragMode)
        {
        case DragMode::Move:
            viz.x += dx; viz.y += dy; break;
        case DragMode::ResizeBR:
            viz.width  = std::max(kMinW, viz.width  + dx);
            viz.height = std::max(kMinH, viz.height + dy);
            break;
        case DragMode::ResizeBL:
        {
            int newW = std::max(kMinW, viz.width  - dx);
            viz.x   += (viz.width - newW);
            viz.width = newW;
            viz.height = std::max(kMinH, viz.height + dy);
            break;
        }
        case DragMode::ResizeTR:
        {
            int newH = std::max(kMinH, viz.height - dy);
            viz.y   += (viz.height - newH);
            viz.height = newH;
            viz.width  = std::max(kMinW, viz.width + dx);
            break;
        }
        case DragMode::ResizeTL:
        {
            int newW = std::max(kMinW, viz.width  - dx);
            int newH = std::max(kMinH, viz.height - dy);
            viz.x   += (viz.width  - newW);
            viz.y   += (viz.height - newH);
            viz.width  = newW; viz.height = newH;
            break;
        }
        case DragMode::ResizeT:
        {
            int newH = std::max(kMinH, viz.height - dy);
            viz.y   += (viz.height - newH);
            viz.height = newH;
            break;
        }
        case DragMode::ResizeB:
            viz.height = std::max(kMinH, viz.height + dy); break;
        case DragMode::ResizeL:
        {
            int newW = std::max(kMinW, viz.width - dx);
            viz.x   += (viz.width - newW);
            viz.width = newW;
            break;
        }
        case DragMode::ResizeR:
            viz.width = std::max(kMinW, viz.width + dx); break;
        default: break;
        }
        m_dragOrigin = vp;
        UpdateScrollBars();  // 화면 밖으로 나가면 스크롤 자동 확장
        Invalidate();
    }
    CView::OnMouseMove(nFlags, point);
}

void CDeepMetriaView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_DELETE && m_selectedViz >= 0)
    {
        if (CDeepMetriaDoc* pDoc = GetDocument())
        {
            auto& vizList = pDoc->GetDashboard().Visualizations();
            if (m_selectedViz < (int)vizList.size())
            {
                int id = vizList[m_selectedViz].id;
                pDoc->GetDashboard().Remove(id);
                m_selectedViz = -1;
                m_dragVizIndex = -1;
                m_dragMode = DragMode::None;
                // 서식/내보내기 탭 비활성화 신호
                if (CFrameWnd* pFrame = GetParentFrame())
                    pFrame->PostMessage(deepmetria::WM_USER_VIZ_SELECTED,
                                        static_cast<WPARAM>(-1), 0);
                UpdateScrollBars();
                Invalidate();
                return;
            }
        }
    }
    CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CDeepMetriaView::OnDropFiles(HDROP hDrop)
{
    TCHAR buf[MAX_PATH] = {};
    if (DragQueryFile(hDrop, 0, buf, MAX_PATH) > 0)
    {
        GetDocument()->OnOpenDocument(buf);
    }
    DragFinish(hDrop);
}

#ifdef _DEBUG
CDeepMetriaDoc* CDeepMetriaView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDeepMetriaDoc)));
    return reinterpret_cast<CDeepMetriaDoc*>(m_pDocument);
}
#endif
