#pragma once

// ExportDialog.h — 차트 내보내기 다이얼로그
// PNG, BMP, CSV 형식 지원, 이미지 크기 설정
// Architecture §3 / DetailedSpec §3.3 참조

// afxwin.h와 Types.h는 stdafx.h(PCH)에서 이미 포함됨
#include "../Resources/resource.h"

// ============================================================
// CExportDialog — CDialogEx 파생
// ============================================================
class CExportDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CExportDialog)

public:
    // IDD 리소스 ID (리소스 파일에 정의 필요)
    enum { IDD = IDD_EXPORT };

    explicit CExportDialog(const VisualizationInfo& vizInfo,
                           CWnd* pParent = nullptr);
    virtual ~CExportDialog();

protected:
    // 컨트롤
    CComboBox       m_comboFormat;   // 내보내기 형식 (PNG, BMP, CSV)
    CEdit           m_editPath;      // 저장 경로
    CButton         m_btnBrowse;     // 폴더 선택
    CSpinButtonCtrl m_spinWidth;     // 이미지 너비 (PNG/BMP)
    CSpinButtonCtrl m_spinHeight;    // 이미지 높이 (PNG/BMP)
    CEdit           m_editWidth;     // 너비 입력 필드
    CEdit           m_editHeight;    // 높이 입력 필드

    // 내보낼 시각화 정보
    VisualizationInfo m_vizInfo;

    // CDialogEx 오버라이드
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    // 버튼 핸들러
    afx_msg void OnBnClickedBrowse();
    afx_msg void OnBnClickedExport();
    afx_msg void OnCbnSelchangeFormat();

    DECLARE_MESSAGE_MAP()

private:
    // 형식에 따른 크기 컨트롤 활성화 여부 갱신
    void UpdateSizeControls();

    // 실제 내보내기 처리
    BOOL ExportAsPng(const CString& path, int width, int height);
    BOOL ExportAsBmp(const CString& path, int width, int height);
    BOOL ExportAsCsv(const CString& path);

    // 선택된 형식 문자열 반환
    CString GetSelectedFormat() const;
};
