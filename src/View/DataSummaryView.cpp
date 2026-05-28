#include "stdafx.h"
#include "DataSummaryView.h"
#include "../Document/DeepMetriaDoc.h"

// ============================================================
// IMPLEMENT_DYNCREATE / 메시지 맵
// ============================================================
IMPLEMENT_DYNCREATE(CDataSummaryView, CListView)

BEGIN_MESSAGE_MAP(CDataSummaryView, CListView)
    ON_NOTIFY_REFLECT(HDN_ITEMCLICK, &CDataSummaryView::OnHdnItemclick)
    ON_MESSAGE(WM_DATA_LOADED, &CDataSummaryView::OnDataLoaded)
END_MESSAGE_MAP()

// ============================================================
// 생성 / 소멸
// ============================================================
CDataSummaryView::CDataSummaryView()
    : CListView()
    , m_nSortCol(-1)
    , m_bSortAsc(TRUE)
{
}

CDataSummaryView::~CDataSummaryView()
{
}

// ============================================================
// 초기화
// ============================================================
void CDataSummaryView::OnInitialUpdate()
{
    CListView::OnInitialUpdate();

    // Report 스타일 + 전체 행 선택 + 그리드 라인
    GetListCtrl().ModifyStyle(0,
        LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL);
    GetListCtrl().SetExtendedStyle(
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

    SetupColumns();
}

void CDataSummaryView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    // 문서에서 DataSummary 읽기
    CDeepMetriaDoc* pDoc = dynamic_cast<CDeepMetriaDoc*>(GetDocument());
    if (pDoc)
    {
        const DataSummary& summary = pDoc->GetDataSummary();
        if (!summary.schema.empty())
            m_summary = summary;
    }
    PopulateList();
}

// ============================================================
// 열 설정
// ============================================================
void CDataSummaryView::SetupColumns()
{
    CListCtrl& lc = GetListCtrl();
    lc.DeleteAllItems();

    // 기존 헤더 제거
    while (lc.GetHeaderCtrl() && lc.GetHeaderCtrl()->GetItemCount() > 0)
        lc.DeleteColumn(0);

    struct ColDef { LPCTSTR name; int width; };
    static const ColDef cols[COL_COUNT] = {
        { _T("컬럼명"),   140 },
        { _T("타입"),      80 },
        { _T("고유값 수"), 90 },
        { _T("결측치"),    80 },
        { _T("최솟값"),   100 },
        { _T("최댓값"),   100 },
    };

    for (int i = 0; i < COL_COUNT; ++i)
        lc.InsertColumn(i, cols[i].name, LVCFMT_LEFT, cols[i].width);
}

// ============================================================
// 공개 인터페이스
// ============================================================
void CDataSummaryView::UpdateSummary(const DataSummary& summary)
{
    m_summary = summary;
    PopulateList();
}

// ============================================================
// 리스트 채우기
// ============================================================
void CDataSummaryView::PopulateList()
{
    CListCtrl& lc = GetListCtrl();
    lc.DeleteAllItems();

    for (int i = 0; i < (int)m_summary.schema.size(); ++i)
    {
        const ColumnSchema& col = m_summary.schema[i];

        int idx = lc.InsertItem(i, col.name);
        lc.SetItemText(idx, COL_TYPE,      col.type);

        CString uniqueStr;
        uniqueStr.Format(_T("%d"), col.uniqueCount);
        lc.SetItemText(idx, COL_UNIQUE, uniqueStr);

        CString nullStr;
        nullStr.Format(_T("%d"), col.nullCount);
        lc.SetItemText(idx, COL_NULLCOUNT, nullStr);

        lc.SetItemText(idx, COL_MIN, col.minValue.IsEmpty() ? _T("-") : col.minValue);
        lc.SetItemText(idx, COL_MAX, col.maxValue.IsEmpty() ? _T("-") : col.maxValue);

        // 정렬용 lParam에 인덱스 저장
        lc.SetItemData(idx, (DWORD_PTR)i);
    }

    // 정렬 상태가 있으면 재정렬
    if (m_nSortCol >= 0)
        SortByColumn(m_nSortCol);
}

// ============================================================
// 헤더 클릭 — 정렬
// ============================================================
void CDataSummaryView::OnHdnItemclick(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMHEADER pNMH = reinterpret_cast<LPNMHEADER>(pNMHDR);
    int col = pNMH->iItem;

    if (col == m_nSortCol)
        m_bSortAsc = !m_bSortAsc;
    else
    {
        m_nSortCol = col;
        m_bSortAsc = TRUE;
    }

    SortByColumn(m_nSortCol);
    *pResult = 0;
}

void CDataSummaryView::SortByColumn(int col)
{
    // lParam에 스키마 인덱스를 저장했으므로 텍스트 기준 정렬
    struct SortParam { int col; BOOL bAsc; CListCtrl* pLC; };
    SortParam sp = { col, m_bSortAsc, &GetListCtrl() };

    GetListCtrl().SortItemsEx(
        [](LPARAM i1, LPARAM i2, LPARAM lp) -> int {
            SortParam* p = reinterpret_cast<SortParam*>(lp);
            CString s1, s2;
            p->pLC->GetItemText((int)i1, p->col, s1.GetBuffer(256), 256); s1.ReleaseBuffer();
            p->pLC->GetItemText((int)i2, p->col, s2.GetBuffer(256), 256); s2.ReleaseBuffer();
            int cmp = s1.CompareNoCase(s2);
            return p->bAsc ? cmp : -cmp;
        },
        reinterpret_cast<DWORD_PTR>(&sp));
}

// ============================================================
// 커스텀 메시지 핸들러
// ============================================================
// WM_DATA_LOADED: WPARAM = 0, LPARAM = DataTable 포인터 (소유권 없음)
LRESULT CDataSummaryView::OnDataLoaded(WPARAM wParam, LPARAM lParam)
{
    // DataTable → DataSummary 변환은 DataSummarizer가 처리
    // 여기서는 Document의 현재 Summary를 갱신하도록 OnUpdate 호출
    OnUpdate(nullptr, 0, nullptr);
    return 0;
}
