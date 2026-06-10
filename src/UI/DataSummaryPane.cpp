// DataSummaryPane.cpp - 좌측 데이터 요약 패널 구현 (CWnd 직접 child)
#include "stdafx.h"
#include "DataSummaryPane.h"
#include "Messages.h"
#include <unordered_set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CDataSummaryPane, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
END_MESSAGE_MAP()

CDataSummaryPane::CDataSummaryPane() = default;
CDataSummaryPane::~CDataSummaryPane() = default;

BOOL CDataSummaryPane::Create(CWnd* pParent, UINT id)
{
    return CWnd::Create(
        nullptr, _T(""),
        WS_CHILD,
        CRect(0, 0, 1, 1),
        pParent, id);
}

int CDataSummaryPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 기본 통계 레이블
    m_lblRowCount.Create(_T("행 수"),   WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0,0,0,0), this);
    m_lblColCount.Create(_T("열 수"),   WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0,0,0,0), this);
    m_lblMissing .Create(_T("결측치"), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0,0,0,0), this);
    m_lblPeriod  .Create(_T("날짜 컬럼"), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0,0,0,0), this);

    // 데이터 로드 전엔 "-" — mock 값 절대 표시하지 않음
    m_valRowCount.Create(_T("-"), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0,0,0,0), this);
    m_valColCount.Create(_T("-"), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0,0,0,0), this);
    m_valMissing .Create(_T("-"), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0,0,0,0), this);
    m_valPeriod  .Create(_T("-"), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0,0,0,0), this);

    // 컬럼 스키마 리스트 (행은 DataSource 로드 시 채움)
    m_listSchema.Create(
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOSORTHEADER | WS_BORDER,
        CRect(0,0,0,0), this, 1010);
    m_listSchema.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listSchema.InsertColumn(0, _T("타입"), LVCFMT_LEFT,  36);
    m_listSchema.InsertColumn(1, _T("컬럼"), LVCFMT_LEFT,  90);
    m_listSchema.InsertColumn(2, _T("메타"), LVCFMT_LEFT,  80);

    return 0;
}

void CDataSummaryPane::LayoutChildren(int cx, int cy)
{
    if (cx <= 0 || cy <= 0) return;

    int margin = 8;
    int y = margin;

    // 섹션 B: 기본 통계 2x2 카드 (레이블 + 값)
    int cardW = (cx - margin * 3) / 2;
    if (cardW < 1) cardW = 1;
    int cardH = 38;
    int lblH  = 14;
    int valH  = 20;

    // 행 수
    m_lblRowCount.SetWindowPos(nullptr, margin,           y,         cardW, lblH, SWP_NOZORDER);
    m_valRowCount.SetWindowPos(nullptr, margin,           y + lblH,  cardW, valH, SWP_NOZORDER);
    // 열 수
    m_lblColCount.SetWindowPos(nullptr, margin*2+cardW,   y,         cardW, lblH, SWP_NOZORDER);
    m_valColCount.SetWindowPos(nullptr, margin*2+cardW,   y + lblH,  cardW, valH, SWP_NOZORDER);
    y += cardH + margin / 2;

    // 결측치
    m_lblMissing.SetWindowPos(nullptr, margin,           y,         cardW, lblH, SWP_NOZORDER);
    m_valMissing.SetWindowPos(nullptr, margin,           y + lblH,  cardW, valH, SWP_NOZORDER);
    // 기간
    m_lblPeriod.SetWindowPos(nullptr, margin*2+cardW,   y,         cardW, lblH, SWP_NOZORDER);
    m_valPeriod.SetWindowPos(nullptr, margin*2+cardW,   y + lblH,  cardW, valH, SWP_NOZORDER);
    y += cardH + margin;

    // 섹션 C: 컬럼 스키마 리스트 (나머지 높이)
    int listH = cy - y - margin;
    if (listH < 60) listH = 60;
    m_listSchema.SetWindowPos(nullptr, margin, y, cx - margin * 2, listH, SWP_NOZORDER);
}

void CDataSummaryPane::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    LayoutChildren(cx, cy);
}

void CDataSummaryPane::OnPaint()
{
    CPaintDC dc(this);
    CRect rc;
    GetClientRect(&rc);
    dc.FillSolidRect(&rc, RGB(0xFF, 0xFF, 0xFF));

    // 값 레이블 폰트 굵게
    LOGFONT lf = {};
    _tcscpy_s(lf.lfFaceName, _T("맑은 고딕"));
    lf.lfHeight  = -MulDiv(11, dc.GetDeviceCaps(LOGPIXELSY), 72);
    lf.lfWeight  = FW_BOLD;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;

    CFont boldFont;
    boldFont.CreateFontIndirect(&lf);

    // 값 컨트롤에 폰트 적용
    m_valRowCount.SetFont(&boldFont, FALSE);
    m_valColCount.SetFont(&boldFont, FALSE);
    m_valMissing .SetFont(&boldFont, FALSE);
    m_valPeriod  .SetFont(&boldFont, FALSE);
}

void CDataSummaryPane::SetSummary(const deepmetria::DataSource& ds)
{
    if (!GetSafeHwnd()) return;

    CString s;

    // 행 수
    s.Format(_T("%d"), static_cast<int>(ds.Rows().size()));
    m_valRowCount.SetWindowText(s);

    // 열 수
    s.Format(_T("%d"), static_cast<int>(ds.Columns().size()));
    m_valColCount.SetWindowText(s);

    // 결측치 비율 계산
    long long totalCells = 0, totalMissing = 0;
    for (const auto& col : ds.Columns())
    {
        totalCells   += col.rowCount;
        totalMissing += col.missing;
    }
    if (totalCells > 0)
    {
        double missingPct = static_cast<double>(totalMissing) / static_cast<double>(totalCells) * 100.0;
        s.Format(_T("%.1f%%"), missingPct);
    }
    else
    {
        s = _T("0.0%");
    }
    m_valMissing.SetWindowText(s);

    // 날짜 컬럼 개수 (도메인 가정 없이 단순 카운트)
    int dateCols = 0;
    for (const auto& col : ds.Columns())
        if (col.type == deepmetria::ColumnType::Date) ++dateCols;
    s.Format(_T("%d개"), dateCols);
    m_valPeriod.SetWindowText(s);

    // 컬럼 리스트 갱신
    UpdateListCtrl(ds);

    Invalidate();
}

void CDataSummaryPane::UpdateListCtrl(const deepmetria::DataSource& ds)
{
    m_listSchema.DeleteAllItems();
    int idx = 0;

    // TEXT 컬럼 unique 값 수 계산을 위한 준비
    const auto& rows = ds.Rows();

    for (size_t ci = 0; ci < ds.Columns().size(); ++ci)
    {
        const auto& col = ds.Columns()[ci];

        LPCTSTR badge = _T("?");
        switch (col.type)
        {
        case deepmetria::ColumnType::Date:   badge = _T("D"); break;
        case deepmetria::ColumnType::Text:   badge = _T("A"); break;
        case deepmetria::ColumnType::Number: badge = _T("#"); break;
        default:                             badge = _T("?"); break;
        }

        int i = m_listSchema.InsertItem(idx++, badge);
        m_listSchema.SetItemText(i, 1, CString(col.name.c_str()));

        // 메타: NUMBER → mean, TEXT → unique 수 + "종", DATE → "날짜"
        CString meta;
        if (col.type == deepmetria::ColumnType::Number)
        {
            if (col.mean != 0.0)
                meta.Format(_T("μ %.1f"), col.mean);
            else if (col.sum != 0.0)
                meta.Format(_T("합 %.0f"), col.sum);
            else
                meta = _T("0");
        }
        else if (col.type == deepmetria::ColumnType::Text)
        {
            // unique 카운트
            std::unordered_set<std::wstring> uniq;
            for (const auto& row : rows)
            {
                if (ci < row.size() && !row[ci].empty())
                    uniq.insert(row[ci]);
            }
            meta.Format(_T("%d종"), static_cast<int>(uniq.size()));
        }
        else if (col.type == deepmetria::ColumnType::Date)
        {
            meta = _T("날짜");
        }
        m_listSchema.SetItemText(i, 2, meta);
    }
}
