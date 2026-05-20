#pragma once

// 내부 분석 도구 함수군 (13개 static 메서드)
// Architecture §5.1 / DetailedSpec §4.1 참조

#include "../../Common/Types.h"
#include <map>
#include <vector>

// ============================================================
// AnalysisTools — LLM 오케스트레이터가 호출하는 C++ 분석 함수
// 모든 결과는 JSON 형식 CString으로 반환
// ============================================================
class AnalysisTools {
public:
    // 기본 통계: 평균, 중앙값, 표준편차, 결측치 수
    static CString BasicStats(const DataTable& data);

    // 그룹 집계: groupCol 기준으로 valueCol을 aggFunc(sum/avg/count/min/max)로 집계
    static CString GroupByAggregate(const DataTable& data,
                                    const CString&   groupCol,
                                    const CString&   valueCol,
                                    const CString&   aggFunc);

    // 시계열 분석: dateCol과 valueCol로 시간 흐름 집계
    static CString TimeSeriesAnalysis(const DataTable& data,
                                      const CString&   dateCol,
                                      const CString&   valueCol);

    // 상관계수 행렬: 지정 수치 컬럼들 간 피어슨 상관계수
    static CString CorrelationMatrix(const DataTable&         data,
                                     const std::vector<CString>& numericCols);

    // 상위 N 행: col 기준 정렬 후 상위 n개 반환
    static CString TopN(const DataTable& data,
                        const CString&   col,
                        int              n,
                        bool             ascending);

    // 빈도 분포: col 값별 출현 횟수 집계
    static CString FrequencyDistribution(const DataTable& data,
                                         const CString&   col);

    // 교차 분석: rowCol × colCol 크로스탭 테이블
    static CString CrossTabulation(const DataTable& data,
                                   const CString&   rowCol,
                                   const CString&   colCol);

    // 이동평균: col에 대해 window 크기 이동평균 계산
    static CString MovingAverage(const DataTable& data,
                                 const CString&   col,
                                 int              window);

    // 백분위수: col에서 지정 백분위값 계산
    static CString Percentile(const DataTable&         data,
                               const CString&           col,
                               const std::vector<double>& percentiles);

    // 날짜 그룹 집계: dateCol을 period(year/month/week/day) 단위로 valueCol 집계
    static CString DateGroupAggregate(const DataTable& data,
                                      const CString&   dateCol,
                                      const CString&   valueCol,
                                      const CString&   period);

    // 행 필터링: col op value 조건으로 필터 후 기본 통계
    static CString Filtering(const DataTable& data,
                              const CString&   col,
                              const CString&   op,
                              const CString&   value);

    // 피벗 테이블: rowCol × colCol 교차에서 valueCol을 aggFunc으로 집계
    static CString PivotTable(const DataTable& data,
                               const CString&   rowCol,
                               const CString&   colCol,
                               const CString&   valueCol,
                               const CString&   aggFunc);

    // 이상치 탐지: col에서 IQR 기반 이상치 탐지 (threshold: IQR 배수)
    static CString OutlierDetection(const DataTable& data,
                                    const CString&   col,
                                    double           threshold);

private:
    // 컬럼 인덱스 조회 (-1: 없음)
    static int FindColumnIndex(const DataTable& data, const CString& colName);

    // 수치 컬럼 값을 double 벡터로 변환 (비수치 값 건너뜀)
    static std::vector<double> ExtractNumericValues(const DataColumn& col);

    // 기본 통계 계산 헬퍼
    static double CalcMean(const std::vector<double>& values);
    static double CalcMedian(std::vector<double> values);
    static double CalcStdDev(const std::vector<double>& values, double mean);

    // JSON 직렬화 헬퍼
    static CString EscapeJsonString(const CString& s);

    // GroupByAggregate 집계 결과 JSON 직렬화 헬퍼
    static CString FormatGroupResults(const std::map<CString, std::vector<double>>& groups,
                                      const CString& aggFunc);

    // CorrelationMatrix 피어슨 상관계수 계산 헬퍼
    static double CalcCorrelation(const std::vector<double>& x, const std::vector<double>& y);

    // PivotTable 집계 결과 JSON 직렬화 헬퍼
    static CString FormatPivotResults(const std::map<CString, std::map<CString, std::vector<double>>>& pivot,
                                      const CString& aggFunc);
};
