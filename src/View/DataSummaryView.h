#pragma once

// DataSummaryView.h — 데이터 파일 컬럼 정보 표시 뷰
// Report 스타일 리스트뷰로 컬럼명, 타입, 고유값 수, 결측치, 최소/최대 표시
// Architecture §3 / DetailedSpec §3.2 참조

// afxwin.h와 Types.h는 stdafx.h(PCH)에서 이미 포함됨
#include <afxcview.h>   // CListView (CCtrlView 파생 Report 스타일 뷰)

// ============================================================
// CDataSummaryView — CListView 파생 (Report 스타일)
// ============================================================
class CDataSummaryView : public CListView
{
    DECLARE_DYNCREATE(CDataSummaryView)

public:
    CDataSummaryView();
    virtual ~CDataSummaryView();

    // 데이터 요약 갱신
    void UpdateSummary(const DataSummary& summary);

protected:
    // 현재 데이터 요약
    DataSummary m_summary;

    // 정렬 상태
    int  m_nSortCol;    // 정렬 중인 열 인덱스
    BOOL m_bSortAsc;    // 오름차순 여부

    // 열 인덱스 상수
    enum ColumnIndex {
        COL_NAME      = 0, // 컬럼명
        COL_TYPE      = 1, // 타입
        COL_UNIQUE    = 2, // 고유값 수
        COL_NULLCOUNT = 3, // 결측치 수
        COL_MIN       = 4, // 최솟값
        COL_MAX       = 5, // 최댓값
        COL_COUNT     = 6
    };

    // CListView 오버라이드
    virtual void OnInitialUpdate() override;
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

    // 메시지 핸들러
    afx_msg void OnHdnItemclick(NMHDR* pNMHDR, LRESULT* pResult);

    // 커스텀 메시지 핸들러
    afx_msg LRESULT OnDataLoaded(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    void SetupColumns();
    void PopulateList();
    void SortByColumn(int col);

    // 정렬 콜백 (static)
    static int CALLBACK SortCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
};
