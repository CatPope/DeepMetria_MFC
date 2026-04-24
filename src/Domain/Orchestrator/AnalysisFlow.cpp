#include "stdafx.h"
#include "AnalysisFlow.h"

// ============================================================
// AnalysisFlow 메서드 구현
// ============================================================

CString AnalysisFlow::GetStateLabel() const
{
    switch (state) {
    case AnalysisFlowState::Idle:         return _T("대기 중");
    case AnalysisFlowState::Planning:     return _T("분석 계획 수립 중...");
    case AnalysisFlowState::Executing:    return _T("데이터 분석 중...");
    case AnalysisFlowState::Interpreting: return _T("인사이트 생성 중...");
    case AnalysisFlowState::Visualizing:  return _T("차트 구성 중...");
    case AnalysisFlowState::Done:         return _T("완료");
    case AnalysisFlowState::Error:        return _T("오류 발생");
    default:                              return _T("알 수 없음");
    }
}

void AnalysisFlow::SetError(const AppError& err)
{
    state     = AnalysisFlowState::Error;
    lastError = err;
}
