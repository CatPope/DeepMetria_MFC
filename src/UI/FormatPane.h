// FormatPane.h - 우측 서식 편집 패널 (CWnd 직접 child)
#pragma once
#include "Visualization.h"

class CFormatPane : public CWnd
{
public:
    CFormatPane();
    virtual ~CFormatPane();

    BOOL Create(CWnd* pParent, UINT id);

    // 선택 시각화 변경 시 호출 — 편집 필드를 viz 값으로 채움
    void SetSelectedViz(int vizIndex, LPCTSTR title);

    // viz 값으로 편집 필드를 채움 (SetSelectedViz 후 MainFrame에서 호출)
    void FillFromViz(const deepmetria::Visualization& viz);

    // MainFrame에서 호출 — 편집 필드 값을 viz에 반영
    void ApplyToViz(deepmetria::Visualization& viz) const;

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBtnApply();
    afx_msg void OnBtnReset();
    afx_msg void OnBtnPickColor();   // 그래프 색상 선택
    DECLARE_MESSAGE_MAP()

private:
    void LayoutChildren(int cx, int cy);

    int     m_vizId = -1;

    // 크기 (px)
    CStatic m_lblSize;
    CEdit   m_edWidth;
    CEdit   m_edHeight;
    CSpinButtonCtrl m_spinWidth;
    CSpinButtonCtrl m_spinHeight;

    // 위치 (px)
    CStatic m_lblPos;
    CEdit   m_edX;
    CEdit   m_edY;

    // 차트 유형
    CStatic   m_lblChartType;
    CComboBox m_cmbChartType;

    // 옵션 토글
    CButton m_chkTitle;
    CButton m_chkGrid;

    // 색상
    CStatic m_lblColor;
    CButton m_btnColor;
    COLORREF m_colorFill = RGB(0x4F, 0x46, 0xE5);  // 현재 선택된 색

    // 하단 버튼
    CButton m_btnApply;
    CButton m_btnReset;
};
