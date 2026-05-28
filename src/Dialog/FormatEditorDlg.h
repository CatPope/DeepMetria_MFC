#pragma once

// FormatEditorDlg.h — 시각화 서식 편집 다이얼로그
// 차트 타입, 크기, 색상을 대화형으로 편집
// Architecture §3 / TC-06-01~05 참조

// afxwin.h와 Types.h는 stdafx.h(PCH)에서 이미 포함됨
#include "../Resources/resource.h"
#include "../Renderer/ChartRenderer.h"

// ============================================================
// CPreviewStatic — 미리보기 전용 서브클래스 (이 파일에서만 사용)
// ============================================================
class CPreviewStatic : public CStatic
{
public:
    CPreviewStatic() {}
    virtual ~CPreviewStatic() {}

    // 렌더링에 사용할 ChartConfig 설정
    void SetConfig(const ChartConfig& config)
    {
        m_config = config;
    }

protected:
    afx_msg void OnPaint()
    {
        CPaintDC dc(this);
        CRect rc;
        GetClientRect(&rc);
        if (rc.IsRectEmpty())
            return;

        // 오프스크린 DC로 깜빡임 방지
        CDC memDC;
        memDC.CreateCompatibleDC(&dc);
        CBitmap bmp;
        bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
        CBitmap* pOldBmp = memDC.SelectObject(&bmp);

        CChartRenderer::Render(&memDC, rc, m_config);

        dc.BitBlt(rc.left, rc.top, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);

        memDC.SelectObject(pOldBmp);
    }

    DECLARE_MESSAGE_MAP()

private:
    ChartConfig m_config;
};

// ============================================================
// CFormatEditorDlg — CDialogEx 파생
// ============================================================
class CFormatEditorDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CFormatEditorDlg)

public:
    enum { IDD = IDD_FORMAT_EDITOR };

    CFormatEditorDlg(const VisualizationInfo& vizInfo, CWnd* pParent = nullptr);
    virtual ~CFormatEditorDlg();

    // 결과 접근
    VisualizationInfo GetUpdatedVizInfo() const { return m_vizInfo; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    // 메시지 핸들러
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnBnClickedColor();
    afx_msg void OnBnClickedApply();
    afx_msg void OnCbnSelchangeChartType();

    DECLARE_MESSAGE_MAP()

private:
    void UpdatePreview();
    void UpdateSizeFromSliders();

    VisualizationInfo m_vizInfo;

    CSliderCtrl    m_sliderWidth;
    CSliderCtrl    m_sliderHeight;
    CEdit          m_editWidth;
    CEdit          m_editHeight;
    CButton        m_btnColor;
    CStatic        m_staticColorPreview;
    CComboBox      m_comboChartType;
    CPreviewStatic m_staticPreview;   // 서브클래스로 교체

    COLORREF    m_currentColor;
};
