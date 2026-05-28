#pragma once

// 분석 플로우 상태 관리
// Architecture §3, §5 / DetailedSpec §4 참조

#include "../../Common/Types.h"

// ============================================================
// 분석 상태 열거형
// ============================================================
enum class AnalysisFlowState {
    Idle,           // 대기 중
    Planning,       // LLM 분석 계획 수립 중 (1단계)
    Executing,      // 분석 도구 실행 중 (2단계)
    Interpreting,   // LLM 인사이트 해석 중 (3단계)
    Visualizing,    // 차트 유형 결정 중 (4단계)
    Done,           // 완료
    Error           // 오류 발생
};

// ============================================================
// 분석 플로우 데이터 구조체
// ============================================================
struct AnalysisFlow {
    AnalysisFlowState state;        // 현재 플로우 상태
    CString           question;     // 사용자 자연어 질문
    CString           plan;         // LLM이 생성한 분석 계획 (JSON)
    CString           toolName;     // 선택된 분석 도구 이름
    CString           toolParams;   // 도구 실행 파라미터 (JSON)
    CString           rawResult;    // 도구 실행 결과 (JSON)
    CString           insight;      // LLM이 생성한 인사이트 텍스트
    ChartConfig       chartConfig;  // 결정된 차트 설정
    AppError          lastError;    // 마지막 오류 정보
    CString           fallbackNotice; // 사용량 한도 초과로 모델 전환 시 사용자 안내 문구

    AnalysisFlow()
        : state(AnalysisFlowState::Idle)
    {}

    // 상태 문자열 반환 (UI 표시용)
    CString GetStateLabel() const;

    // 오류 상태로 전환
    void SetError(const AppError& err);
};
