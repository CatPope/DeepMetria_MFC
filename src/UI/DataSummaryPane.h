// DataSummaryPane.h - 좌측 데이터 요약 패널 (CWnd 직접 child)
#pragma once
#include "DataSource.h"

class CDataSummaryPane : public CWnd
{
public:
    CDataSummaryPane();
    virtual ~CDataSummaryPane();

    BOOL Create(CWnd* pParent, UINT id);

    // Dashboard observer 콜백 — DataSource 갱신 시 호출
    void SetSummary(const deepmetria::DataSource& ds);

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()

private:
    void LayoutChildren(int cx, int cy);
    void UpdateListCtrl(const deepmetria::DataSource& ds);

    // 컬럼 스키마 리스트
    CListCtrl   m_listSchema;

    // 기본 통계 레이블 (4개: 행수/열수/결측/기간)
    CStatic     m_lblRowCount;
    CStatic     m_lblColCount;
    CStatic     m_lblMissing;
    CStatic     m_lblPeriod;

    // 통계 값 레이블
    CStatic     m_valRowCount;
    CStatic     m_valColCount;
    CStatic     m_valMissing;
    CStatic     m_valPeriod;
};
