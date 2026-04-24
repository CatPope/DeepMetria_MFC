#pragma once

// QueryInputView.h — 자연어 질문 입력 폼 뷰
// 분석 시작, 진행률 표시, 결과 반영
// Architecture §3 / DetailedSpec §3.3 참조

#include "../stdafx.h"
#include "../Common/Types.h"

// 전방 선언
class AnalysisOrchestrator;
class CDashboardView;

// ============================================================
// CQueryInputView — CFormView 파생
// ============================================================
class CQueryInputView : public CFormView
{
    DECLARE_DYNCREATE(CQueryInputView)

public:
    // IDD 리소스 ID (리소스 파일에 정의 필요)
    enum { IDD = IDD_QUERYINPUT_FORM };

    CQueryInputView();
    virtual ~CQueryInputView();

    // 외부에서 주입
    void SetDashboardView(CDashboardView* pDashboardView);
    void SetDataSourceId(const CString& datasourceId);

protected:
    // 컨트롤
    CEdit         m_editQuery;      // 자연어 질문 입력 (멀티라인)
    CButton       m_btnAnalyze;     // "분석 시작" 버튼
    CStatic       m_staticStatus;   // 현재 분석 상태 표시
    CProgressCtrl m_progressBar;    // 분석 진행률 (0~100)

    // 상태
    CString       m_datasourceId;   // 현재 데이터소스 ID
    BOOL          m_bAnalyzing;     // 분석 진행 중 여부
    CDashboardView* m_pDashboardView; // 결과 전달 대상 뷰

    // CFormView 오버라이드
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual void OnInitialUpdate() override;

    // 버튼 핸들러
    afx_msg void OnBnClickedAnalyze();

    // Enter 키 처리
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    // 커스텀 메시지 핸들러
    afx_msg LRESULT OnAnalysisProgress(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAnalysisDone(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    void StartAnalysis();
    void SetAnalyzingState(BOOL bAnalyzing);
    void UpdateStatusText(const CString& text);
};
