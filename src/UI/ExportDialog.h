// ExportDialog.h - 내보내기 모달 다이얼로그
#pragma once
#include "resource.h"
#include "DataSource.h"
#include "Dashboard.h"

class CExportDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CExportDialog)
public:
    explicit CExportDialog(
        const deepmetria::DataSource& ds,
        const deepmetria::Dashboard&  dash,
        int selectedVizIdx = -1,
        CWnd* pParent = nullptr);
    virtual ~CExportDialog();

    enum { IDD = IDD_EXPORT };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    afx_msg void OnBtnBrowse();
    afx_msg void OnBtnSave();
    DECLARE_MESSAGE_MAP()

private:
    const deepmetria::DataSource& m_ds;
    const deepmetria::Dashboard&  m_dash;

    CStatic   m_stcPreview;
    CEdit     m_edPath;
    CButton   m_btnBrowse;
    CComboBox m_cmbResolution;

    // 선택 형식 (0=PNG, 1=BMP, 2=JPG, 3=PDF)
    int m_formatSel = 0;

    // 호출 시점의 선택된 시각화 인덱스 (-1이면 선택 없음)
    int m_selectedVizIdx = -1;
};
