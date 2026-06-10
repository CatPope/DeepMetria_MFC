// QueryInputDockPane.cpp - 우측 자연어 질문 패널 구현 (CWnd 직접 child)
#include "stdafx.h"
#include "QueryInputDockPane.h"
#include "Messages.h"
#include <gdiplus.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static const wchar_t* kPlaceholderText = L"자연어로 질문을 입력하세요...";

BEGIN_MESSAGE_MAP(CQueryInputDockPane, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_CTLCOLOR()
    ON_WM_MOUSEWHEEL()
    ON_WM_LBUTTONDOWN()
    ON_BN_CLICKED(2001, &CQueryInputDockPane::OnBtnSubmit)
    ON_EN_SETFOCUS (2011, &CQueryInputDockPane::OnEdSetFocus)
    ON_EN_KILLFOCUS(2011, &CQueryInputDockPane::OnEdKillFocus)
END_MESSAGE_MAP()

CQueryInputDockPane::CQueryInputDockPane() = default;
CQueryInputDockPane::~CQueryInputDockPane() = default;

BOOL CQueryInputDockPane::Create(CWnd* pParent, UINT id)
{
    return CWnd::Create(
        nullptr, _T(""),
        WS_CHILD,
        CRect(0, 0, 1, 1),
        pParent, id);
}

int CQueryInputDockPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 질문 입력 레이블
    m_lblQuery.Create(_T("질문 입력"), WS_CHILD | WS_VISIBLE | SS_LEFT,
                      CRect(0,0,0,0), this);

    // 질문 입력 편집기 (placeholder는 클릭 시 사라짐)
    m_edQuery.Create(
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        CRect(0,0,0,0), this, 2011);
    m_isPlaceholder = true;
    m_edQuery.SetWindowText(kPlaceholderText);

    // 분석 실행 버튼
    m_btnSubmit.Create(_T("→ 분석 실행"), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                       CRect(0,0,0,0), this, 2001);

    return 0;
}

void CQueryInputDockPane::LayoutChildren(int cx, int cy)
{
    if (cx <= 0 || cy <= 0) return;

    int margin = 8;

    // 하단 입력 영역: 레이블 16 + 입력칸 ≈2줄(44) + 버튼 28 + 여유
    int btnH    = 28;
    int editH   = 44;
    int lblH    = 16;
    int bottomTotal = lblH + 4 + editH + margin + btnH + margin;

    // 채팅 영역은 위쪽 나머지 전부 — 누적 메시지 + 스크롤
    m_chatAreaH = std::max(80, cy - bottomTotal);

    int y = m_chatAreaH + margin;

    m_lblQuery.SetWindowPos(nullptr, margin, y, cx - margin*2, lblH, SWP_NOZORDER);
    y += lblH + 4;
    m_edQuery.SetWindowPos(nullptr, margin, y, cx - margin*2, editH, SWP_NOZORDER);
    y += editH + margin;

    int btnW = 110;
    m_btnSubmit.SetWindowPos(nullptr, cx - margin - btnW, y, btnW, btnH, SWP_NOZORDER);
}

void CQueryInputDockPane::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    LayoutChildren(cx, cy);
}

void CQueryInputDockPane::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);
    dc.FillSolidRect(&rcClient, RGB(0xFF, 0xFF, 0xFF));

    if (m_chatAreaH <= 0) return;

    Gdiplus::Graphics g(dc.GetSafeHdc());
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    int margin = 8;
    int cx = rcClient.Width();
    int y  = margin;

    Gdiplus::Font fontSmall(L"맑은 고딕", 8.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
    Gdiplus::Font fontBody (L"맑은 고딕", 9.5f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
    Gdiplus::SolidBrush grayBrush (Gdiplus::Color(255, 150, 150, 150));
    Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255, 255));
    Gdiplus::SolidBrush blackBrush(Gdiplus::Color(255, 30, 30, 30));
    Gdiplus::SolidBrush blueBrush (Gdiplus::Color(255, 37, 99, 235));
    Gdiplus::Pen        borderPen (Gdiplus::Color(255, 220, 220, 220), 1.0f);

    // 메시지가 비어있으면 기본 AI 인사
    std::vector<ChatMsg> view = m_messages;
    if (view.empty())
    {
        view.push_back({ false,
            L"데이터를 요약했어요. 무엇이 궁금하세요?" });
    }

    // 클리핑: 채팅 영역(상단 m_chatAreaH 픽셀)만 그리기
    Gdiplus::Region clip(Gdiplus::RectF(0, 0, (float)cx, (float)m_chatAreaH));
    g.SetClip(&clip);

    // 스크롤 적용 — 아래로 누적되며 m_chatScrollY 만큼 위로 밀림 (0 = 최신 보임)
    y -= m_chatScrollY;

    int bottomLimit = m_chatAreaH + m_chatScrollY + 1000;  // 클리핑이 처리

    for (size_t i = 0; i < view.size() && y < bottomLimit; ++i)
    {
        const auto& m = view[i];
        int bubbleW = (cx - margin * 2) * 2 / 3;
        int left, top = y, right;
        int r = 8;

        if (m.isUser)
        {
            left  = cx - margin - bubbleW;
            right = cx - margin;
        }
        else
        {
            left  = margin;
            right = margin + bubbleW;
        }

        // 텍스트 measure → 가변 높이 (말풍선이 잘리지 않도록)
        Gdiplus::StringFormat sf;
        sf.SetAlignment(m.isUser ? Gdiplus::StringAlignmentFar : Gdiplus::StringAlignmentNear);
        sf.SetLineAlignment(Gdiplus::StringAlignmentNear);
        sf.SetFormatFlags(0);  // 줄바꿈 허용 (NoWrap 해제)
        Gdiplus::RectF measureRc(0, 0,
            (Gdiplus::REAL)(right - left - r*2 - 8), 1000.f);
        Gdiplus::RectF resultRc;
        g.MeasureString(m.text.c_str(), -1, &fontBody, measureRc, &sf, &resultRc);
        int textH = (int)resultRc.Height + 8;
        int bubbleH = std::max(38, textH + 8);

        if (m.isUser)
        {
            Gdiplus::PointF ptNa((Gdiplus::REAL)(right - 18), (Gdiplus::REAL)(top - 13));
            g.DrawString(L"나", -1, &fontSmall, ptNa, &grayBrush);
        }
        else
        {
            Gdiplus::PointF ptAI((Gdiplus::REAL)(left + 4), (Gdiplus::REAL)(top - 13));
            g.DrawString(L"AI", -1, &fontSmall, ptAI, &grayBrush);
        }

        Gdiplus::GraphicsPath path;
        path.AddArc(left,        top,           r*2, r*2, 180, 90);
        path.AddArc(right - r*2, top,           r*2, r*2, 270, 90);
        path.AddArc(right - r*2, top + bubbleH - r*2, r*2, r*2,   0, 90);
        path.AddArc(left,        top + bubbleH - r*2, r*2, r*2,  90, 90);
        path.CloseFigure();

        if (m.isUser)
            g.FillPath(&blueBrush, &path);
        else
        {
            g.FillPath(&whiteBrush, &path);
            g.DrawPath(&borderPen, &path);
        }

        Gdiplus::RectF rcText(
            (Gdiplus::REAL)(left + r),
            (Gdiplus::REAL)(top + 4),
            (Gdiplus::REAL)(right - left - r*2),
            (Gdiplus::REAL)(bubbleH - 8));
        g.DrawString(m.text.c_str(), -1, &fontBody, rcText, &sf,
                     m.isUser ? &whiteBrush : &blackBrush);

        y += bubbleH + 6;
    }

    // 전체 컨텐츠 높이 갱신 (스크롤 한계 계산용)
    m_chatContentH = y + m_chatScrollY;
    g.ResetClip();
}

void CQueryInputDockPane::AddUserMessage(LPCWSTR text)
{
    if (!text) return;
    m_messages.push_back({ true, std::wstring(text) });
    // 새 메시지 추가 시 자동으로 최신(맨 아래)으로 스크롤
    if (GetSafeHwnd())
    {
        m_chatScrollY = std::max(0, m_chatContentH - m_chatAreaH);
        Invalidate();
        UpdateWindow();  // 즉시 paint — push 직후 화면 반영 보장
    }
}

void CQueryInputDockPane::AddAiMessage(LPCWSTR text)
{
    if (!text) return;
    m_messages.push_back({ false, std::wstring(text) });
    if (GetSafeHwnd())
    {
        m_chatScrollY = std::max(0, m_chatContentH - m_chatAreaH);
        Invalidate();
        UpdateWindow();  // 즉시 paint — AI 말풍선이 다른 child 갱신에 가려지지 않도록 강제
    }
}

void CQueryInputDockPane::SetSubmitEnabled(bool enabled)
{
    if (m_btnSubmit.GetSafeHwnd())
    {
        m_btnSubmit.EnableWindow(enabled ? TRUE : FALSE);
        m_btnSubmit.SetWindowText(enabled ? _T("→ 분석 실행") : _T("분석 중..."));
    }
    if (m_edQuery.GetSafeHwnd())
        m_edQuery.EnableWindow(enabled ? TRUE : FALSE);
}

// 채팅 영역 위에서 휠 굴리면 대화 기록 스크롤
BOOL CQueryInputDockPane::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint pt)
{
    CPoint ptClient = pt;
    ScreenToClient(&ptClient);
    if (ptClient.y >= m_chatAreaH) return FALSE;  // 입력칸 영역은 통과

    int step = 32 * (zDelta / WHEEL_DELTA);
    m_chatScrollY -= step;  // 휠 위쪽(양수) → 위로 스크롤(이전 메시지)

    int maxScroll = std::max(0, m_chatContentH - m_chatAreaH);
    if (m_chatScrollY < 0) m_chatScrollY = 0;
    if (m_chatScrollY > maxScroll) m_chatScrollY = maxScroll;

    Invalidate();
    return TRUE;
}

void CQueryInputDockPane::OnLButtonDown(UINT nFlags, CPoint point)
{
    // 채팅 영역 클릭 시 포커스 가져와서 휠 인식
    if (point.y < m_chatAreaH) SetFocus();
    CWnd::OnLButtonDown(nFlags, point);
}

// placeholder 표시/제거 — focus 진입/이탈 + 보낸 직후 호출
void CQueryInputDockPane::ShowPlaceholder()
{
    if (!m_edQuery.GetSafeHwnd()) return;
    m_isPlaceholder = true;
    m_edQuery.SetWindowText(kPlaceholderText);
    m_edQuery.Invalidate();
}

void CQueryInputDockPane::ClearPlaceholder()
{
    if (!m_edQuery.GetSafeHwnd()) return;
    m_isPlaceholder = false;
    m_edQuery.SetWindowText(_T(""));
    m_edQuery.Invalidate();
}

// 클릭/focus 진입 시 placeholder 제거
void CQueryInputDockPane::OnEdSetFocus()
{
    if (m_isPlaceholder) ClearPlaceholder();
}

// focus 이탈 시 빈 칸이면 placeholder 복원
void CQueryInputDockPane::OnEdKillFocus()
{
    CString cur;
    if (m_edQuery.GetSafeHwnd()) m_edQuery.GetWindowText(cur);
    cur.Trim();
    if (cur.IsEmpty()) ShowPlaceholder();
}

// placeholder 상태일 때 회색으로 그리도록 텍스트 색 변경
HBRUSH CQueryInputDockPane::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH br = CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
    if (pWnd && pWnd->GetSafeHwnd() == m_edQuery.GetSafeHwnd() &&
        nCtlColor == CTLCOLOR_EDIT && m_isPlaceholder)
    {
        pDC->SetTextColor(RGB(150, 150, 150));   // 회색 placeholder
    }
    return br;
}

// m_edQuery 가 focus 일 때 Ctrl+C/V/X/A 가 메인 프레임 accelerator 에
// 가로채이는 것을 막고 Edit 컨트롤이 직접 처리하도록 보장.
BOOL CQueryInputDockPane::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP || pMsg->message == WM_CHAR)
    {
        if (m_edQuery.GetSafeHwnd() &&
            ::GetFocus() == m_edQuery.GetSafeHwnd())
        {
            ::TranslateMessage(pMsg);
            ::DispatchMessage(pMsg);
            return TRUE;
        }
    }
    return CWnd::PreTranslateMessage(pMsg);
}

void CQueryInputDockPane::OnBtnSubmit()
{
    CString query;
    m_edQuery.GetWindowText(query);
    query.Trim();
    if (query.IsEmpty() || m_isPlaceholder || query == kPlaceholderText)
    {
        AfxMessageBox(_T("질문을 입력하세요."));
        return;
    }

    // 채팅 큐에 사용자 질문 누적
    AddUserMessage(query);

    // 메인 프레임으로 질문 전달 (정적 버퍼로 PostMessage 안전 보장)
    static CStringW s_lastQuery;
    s_lastQuery = query;

    CWnd* pFrame = GetParentFrame();
    if (pFrame)
        pFrame->PostMessage(deepmetria::WM_USER_QUERY_SUBMIT,
                            0, reinterpret_cast<LPARAM>(s_lastQuery.GetString()));

    // 보낸 직후 입력칸 비우기 — focus 가 있으면 빈 칸 그대로, 없으면 placeholder 복원
    if (::GetFocus() == m_edQuery.GetSafeHwnd())
    {
        m_isPlaceholder = false;
        m_edQuery.SetWindowText(_T(""));
        m_edQuery.Invalidate();
    }
    else
    {
        ShowPlaceholder();
    }
}

