#pragma once

// DeepMetriaDoc.h — CDocument 파생 문서 클래스
// 현재 로드된 데이터 테이블과 시각화 정보를 보유한다.

#include "../Common/Types.h"

// ============================================================
// CDeepMetriaDoc 클래스 선언
// ============================================================
class CDeepMetriaDoc : public CDocument
{
    DECLARE_DYNCREATE(CDeepMetriaDoc)

protected:
    CDeepMetriaDoc();

public:
    virtual ~CDeepMetriaDoc() = default;

    // ---- 데이터 접근 ----
    const DataTable&                     GetDataTable()       const { return m_dataTable; }
    const DataSummary&                   GetDataSummary()     const { return m_dataSummary; }
    const std::vector<VisualizationInfo>& GetVisualizations() const { return m_visualizations; }

    // ---- 파일 로드 ----
    // filePath: 전체 경로. Parser를 호출하여 DataTable을 채운다.
    BOOL LoadFile(const CString& filePath, AppError& outError);

    // ---- 시각화 관리 ----
    void AddVisualization(const VisualizationInfo& viz);
    void RemoveVisualization(const CString& vizId);
    void ClearVisualizations();

    // ---- CDocument 오버라이드 ----
    virtual BOOL OnNewDocument() override;
    virtual void Serialize(CArchive& ar) override;
    virtual void DeleteContents() override;

protected:
    DataTable                    m_dataTable;        // 현재 로드된 데이터
    DataSummary                  m_dataSummary;      // 데이터 요약 정보
    std::vector<VisualizationInfo> m_visualizations; // 현재 대시보드의 시각화 목록

    DECLARE_MESSAGE_MAP()
};
