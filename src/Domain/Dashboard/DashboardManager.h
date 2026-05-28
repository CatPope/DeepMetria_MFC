#pragma once

// 대시보드 패널 관리
// Architecture §3 / DetailedSpec §1.5, §6 참조

#include "../../Common/Types.h"
#include <vector>

// 전방 선언 (Infrastructure 레이어 — .cpp에서 include)
class SQLiteDB;

// ============================================================
// DashboardManager — 시각화 카드 추가/삭제/저장/로드
// ============================================================
class DashboardManager {
public:
    DashboardManager();
    ~DashboardManager();

    // 시각화 카드 추가 (메모리 + DB)
    BOOL AddVisualization(const VisualizationInfo& viz, AppError& outError);

    // 시각화 카드 제거 (메모리 + DB)
    BOOL RemoveVisualization(const CString& vizId, AppError& outError);

    // 현재 로드된 시각화 목록 반환
    std::vector<VisualizationInfo> GetVisualizations() const;

    // 대시보드 레이아웃 + 시각화 목록을 SQLiteDB에 저장
    BOOL SaveDashboard(const CString& name, AppError& outError);

    // 대시보드를 SQLiteDB에서 로드
    BOOL LoadDashboard(const CString& dashboardId, AppError& outError);

    // 레이아웃 업데이트 (드래그/리사이즈 후 호출)
    BOOL UpdateLayout(const std::vector<LayoutItem>& layout, AppError& outError);

    // 현재 대시보드 ID 반환
    CString GetCurrentDashboardId() const { return m_currentDashboardId; }

    // 대시보드 목록 조회
    std::vector<DashboardInfo> GetDashboardList(AppError& outError);

    // 대시보드 삭제
    BOOL RemoveDashboard(const CString& dashboardId, AppError& outError);

    // 데이터 요약 기반 자동 대시보드 구성
    BOOL AutoConfigLayout(const DataSummary& summary, DashboardDetail& outDashboard, AppError& outError);

private:
    // 레이아웃 벡터를 JSON 문자열로 직렬화
    static CString SerializeLayout(const std::vector<LayoutItem>& layout);

    // JSON 문자열에서 레이아웃 벡터 역직렬화
    static BOOL DeserializeLayout(const CString&          json,
                                  std::vector<LayoutItem>& outLayout);

    // VisualizationInfo를 JSON 문자열로 직렬화 (DB 저장용)
    static CString SerializeVizInfo(const VisualizationInfo& viz);

    std::vector<VisualizationInfo> m_visualizations;  // 현재 로드된 시각화 목록
    std::vector<LayoutItem>        m_layout;           // 현재 레이아웃
    CString                        m_currentDashboardId; // 현재 대시보드 ID
};
