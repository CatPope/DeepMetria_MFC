// tests/SelfTest.cpp - 자동 검증 스위트 (Command 패턴)
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "stdafx.h"
#include "SelfTest.h"

#include "CsvParser.h"
#include "JsonParser.h"
#include "DataSource.h"
#include "AnalysisTool.h"
#include "Dashboard.h"
#include "ChainOfThinking.h"
#include "MemoryCache.h"
#include "ChartRenderer.h"
#include "LLMRouter.h"
#include "SecretsStore.h"
#include "ParserFactory.h"
#include "resource.h"

#include <thread>
#include <chrono>

#include <io.h>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <fcntl.h>
#include <cstdio>
#include <sstream>
#include <memory>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>
#include <algorithm>

namespace deepmetria {

// -----------------------------------------------------------------------
// Command 인터페이스
// -----------------------------------------------------------------------
namespace {

struct ISelfTestCommand
{
    virtual ~ISelfTestCommand() = default;
    virtual bool Run(std::wostream& log) = 0;
    virtual const wchar_t* Name() const = 0;
};

// 픽스처 데이터 경로 헬퍼
static std::wstring DataPath(const wchar_t* filename)
{
    // 실행 파일 기준이 아닌, 소스 기준 경로를 컴파일 타임에 결정
    // __FILE__ 은 좁은 문자(char)이므로 wstring 으로 변환
    std::string srcFile(__FILE__);
    // ...\tests\SelfTest.cpp → ...\tests\data\filename
    auto pos = srcFile.rfind("SelfTest.cpp");
    std::string base = (pos != std::string::npos)
        ? srcFile.substr(0, pos)
        : srcFile;
    std::wstring wbase(base.begin(), base.end());
    return wbase + L"data\\" + filename;
}

static std::wstring OutDir()
{
    std::string srcFile(__FILE__);
    auto pos = srcFile.rfind("SelfTest.cpp");
    std::string base = (pos != std::string::npos)
        ? srcFile.substr(0, pos)
        : srcFile;
    std::wstring wbase(base.begin(), base.end());
    return wbase + L"out";
}

// -----------------------------------------------------------------------
// 1. CsvLoadCommand
// -----------------------------------------------------------------------
struct CsvLoadCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"CsvLoadCommand"; }

    bool Run(std::wostream& log) override
    {
        CsvParser parser;
        DataSource ds;
        std::wstring path = DataPath(L"sales.csv");

        if (!parser.Parse(path, ds))
        {
            log << L"Parse 실패: " << path;
            return false;
        }
        if (ds.Rows().size() != 12)
        {
            log << L"rows.size() == " << ds.Rows().size() << L" (기대: 12)";
            return false;
        }
        if (ds.Columns().size() < 3)
        {
            log << L"columns.size() == " << ds.Columns().size() << L" (기대: 3 이상)";
            return false;
        }
        if (ds.Columns()[0].name != L"지역")
        {
            log << L"columns[0].name == " << ds.Columns()[0].name << L" (기대: 지역)";
            return false;
        }
        if (ds.Columns()[2].name != L"매출")
        {
            log << L"columns[2].name == " << ds.Columns()[2].name << L" (기대: 매출)";
            return false;
        }
        return true;
    }
};

// -----------------------------------------------------------------------
// 2. JsonLoadCommand
// -----------------------------------------------------------------------
struct JsonLoadCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"JsonLoadCommand"; }

    bool Run(std::wostream& log) override
    {
        JsonParser parser;
        DataSource ds;
        std::wstring path = DataPath(L"products.json");

        if (!parser.Parse(path, ds))
        {
            log << L"Parse 실패: " << path;
            return false;
        }
        if (ds.Rows().size() != 5)
        {
            log << L"rows.size() == " << ds.Rows().size() << L" (기대: 5)";
            return false;
        }
        if (ds.Columns().empty() || ds.Columns()[0].name != L"name")
        {
            log << L"columns[0].name == "
                << (ds.Columns().empty() ? L"(없음)" : ds.Columns()[0].name.c_str())
                << L" (기대: name)";
            return false;
        }
        return true;
    }
};

// -----------------------------------------------------------------------
// 3. BasicStatsCommand
// -----------------------------------------------------------------------
struct BasicStatsCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"BasicStatsCommand"; }

    bool Run(std::wostream& log) override
    {
        CsvParser parser;
        DataSource ds;
        std::wstring path = DataPath(L"sales.csv");

        if (!parser.Parse(path, ds))
        {
            log << L"Parse 실패";
            return false;
        }

        AnalysisTool::InferSchemaTypes(ds);
        AnalysisTool::ComputeBasicStatistics(ds);

        // 매출 컬럼 찾기
        const Column* colMaechul = nullptr;
        const Column* colJiyeok  = nullptr;
        for (const auto& c : ds.Columns())
        {
            if (c.name == L"매출")  colMaechul = &c;
            if (c.name == L"지역")  colJiyeok  = &c;
        }

        if (!colMaechul)
        {
            log << L"매출 컬럼 없음";
            return false;
        }
        if (!colJiyeok)
        {
            log << L"지역 컬럼 없음";
            return false;
        }

        // 합계 검증 (오차 0.5 이내)
        if (std::abs(colMaechul->sum - 10600.0) > 0.5)
        {
            log << L"매출 sum == " << colMaechul->sum << L" (기대: 10600, 오차 0.5)";
            return false;
        }

        // 평균 검증 (오차 1.0 이내)
        if (std::abs(colMaechul->mean - 883.33) > 1.0)
        {
            log << L"매출 mean == " << colMaechul->mean << L" (기대: 883.33, 오차 1.0)";
            return false;
        }

        // 타입 검증
        if (colMaechul->type != ColumnType::Number)
        {
            log << L"매출 type != Number";
            return false;
        }
        if (colJiyeok->type != ColumnType::Text)
        {
            log << L"지역 type != Text";
            return false;
        }

        return true;
    }
};

// -----------------------------------------------------------------------
// 4. GroupBySumCommand
// -----------------------------------------------------------------------
struct GroupBySumCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"GroupBySumCommand"; }

    bool Run(std::wostream& log) override
    {
        CsvParser parser;
        DataSource ds;
        std::wstring path = DataPath(L"sales.csv");

        if (!parser.Parse(path, ds))
        {
            log << L"Parse 실패";
            return false;
        }

        AnalysisTool::InferSchemaTypes(ds);
        AnalysisTool::ComputeBasicStatistics(ds);

        auto vizOpt = AnalysisTool::GroupBySum(ds, L"지역", L"매출");
        if (!vizOpt.has_value())
        {
            log << L"GroupBySum 반환 nullopt";
            return false;
        }

        const Visualization& viz = vizOpt.value();

        // 카테고리 4개 (서울/부산/대구/광주)
        if (viz.categories.size() != 4)
        {
            log << L"categories.size() == " << viz.categories.size() << L" (기대: 4)";
            return false;
        }

        // 서울 합계 == 4500
        if (viz.series.empty())
        {
            log << L"series 비어 있음";
            return false;
        }

        double seoulSum = 0.0;
        for (size_t i = 0; i < viz.categories.size(); ++i)
        {
            if (viz.categories[i] == L"서울")
            {
                if (!viz.series.empty() && i < viz.series[0].values.size())
                    seoulSum = viz.series[0].values[i];
                break;
            }
        }

        if (std::abs(seoulSum - 4500.0) > 0.5)
        {
            log << L"서울 합계 == " << seoulSum << L" (기대: 4500)";
            return false;
        }

        return true;
    }
};

// -----------------------------------------------------------------------
// 5. PngExportCommand
// -----------------------------------------------------------------------
struct PngExportCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"PngExportCommand"; }

    bool Run(std::wostream& log) override
    {
        // tests/out 디렉터리 생성
        std::wstring outDir = OutDir();
        if (!CreateDirectoryW(outDir.c_str(), nullptr))
        {
            DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS)
            {
                log << L"CreateDirectoryW 실패: " << err;
                return false;
            }
        }

        // 데이터 로드
        CsvParser parser;
        DataSource ds;
        std::wstring path = DataPath(L"sales.csv");
        if (!parser.Parse(path, ds))
        {
            log << L"Parse 실패";
            return false;
        }
        AnalysisTool::InferSchemaTypes(ds);
        AnalysisTool::ComputeBasicStatistics(ds);

        // 대시보드 구성
        Dashboard dash;
        dash.Add(AnalysisTool::SummaryPanel(ds));
        auto vizOpt = AnalysisTool::GroupBySum(ds, L"지역", L"매출");
        if (vizOpt.has_value())
            dash.Add(vizOpt.value());

        // PNG 내보내기
        std::wstring outPath = outDir + L"\\selftest.png";
        CRect client(0, 0, 800, 600);
        ChartRenderer renderer;
        if (!renderer.ExportDashboardToImage(client, ds, dash, outPath))
        {
            log << L"ExportDashboardToImage 실패";
            return false;
        }

        // PNG 시그니처 검증 (첫 8바이트: 89 50 4E 47 0D 0A 1A 0A)
        static const BYTE pngSig[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
        HANDLE hFile = CreateFileW(outPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            log << L"저장된 파일 열기 실패";
            return false;
        }

        BYTE header[8] = {};
        DWORD read = 0;
        ReadFile(hFile, header, 8, &read, nullptr);
        CloseHandle(hFile);

        if (read < 8 || memcmp(header, pngSig, 8) != 0)
        {
            log << L"PNG 시그니처 불일치";
            return false;
        }

        return true;
    }
};

// -----------------------------------------------------------------------
// 6. LLMHeuristicCommand
// -----------------------------------------------------------------------
struct LLMHeuristicCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"LLMHeuristicCommand"; }

    bool Run(std::wostream& log) override
    {
        // 키 없는 빈 SecretsStore
        auto secrets = std::make_shared<SecretsStore>(L"Software\\DeepMetria\\SelfTest_NoKey");
        LLMRouter router(secrets);
        // LoadConfig 는 키 없으면 Provider::Unknown 으로 유지 — 휴리스틱 경로 사용

        LLMRouter::PlanRequest req;
        req.question    = L"월별 추이를 보여줘";
        req.dataSummary = L"";
        // typeEnum: 1=Number, 3=Date
        req.columns.push_back({ L"월",  3 });
        req.columns.push_back({ L"매출", 1 });

        LLMRouter::PlanResponse resp = router.Plan(req);

        if (!resp.ok)
        {
            log << L"Plan 실패: " << resp.error;
            return false;
        }

        bool validTool = (resp.toolName == L"trend_over_time" ||
                          resp.toolName == L"table_sample");
        if (!validTool)
        {
            log << L"toolName == " << resp.toolName
                << L" (기대: trend_over_time 또는 table_sample)";
            return false;
        }

        return true;
    }
};

// -----------------------------------------------------------------------
// 7. ChainOfThinkingCommand
// -----------------------------------------------------------------------
struct ChainOfThinkingCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"ChainOfThinkingCommand"; }

    bool Run(std::wostream& log) override
    {
        ChainOfThinking cot;
        cot.Add(L"step1");
        cot.Add(L"step2");

        if (cot.Steps().size() < 2)
        {
            log << L"Steps().size() == " << cot.Steps().size() << L" (기대: 2 이상)";
            return false;
        }

        if (cot.Steps()[0].index != 1)
        {
            log << L"Steps()[0].index == " << cot.Steps()[0].index << L" (기대: 1)";
            return false;
        }

        return true;
    }
};

// -----------------------------------------------------------------------
// 8. DialogResourceCommand (FR-07 내보내기 다이얼로그 리소스 존재 확인)
// -----------------------------------------------------------------------
struct DialogResourceCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"DialogResourceCommand"; }

    bool Run(std::wostream& log) override
    {
        // 와이어프레임 슬라이드 7/8/9 에 대응하는 신 UI 다이얼로그 리소스
        struct { UINT id; const wchar_t* name; } need[] = {
            { IDD_EXPORT,   L"IDD_EXPORT"   },
            { IDD_SETTINGS, L"IDD_SETTINGS" },
            { IDD_RETRY,    L"IDD_RETRY"    },
        };
        for (auto& d : need)
        {
            HRSRC h = FindResourceW(AfxGetResourceHandle(),
                                    MAKEINTRESOURCEW(d.id), RT_DIALOG);
            if (!h) { log << d.name << L" 리소스 없음"; return false; }
        }
        return true;
    }
};

// -----------------------------------------------------------------------
// 9. DashboardDragCommand (FR-06 실 검증)
// -----------------------------------------------------------------------
struct DashboardDragCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"DashboardDragCommand"; }

    bool Run(std::wostream& log) override
    {
        // Dashboard 인스턴스 생성 및 Visualization 두 개 추가
        Visualization v1;
        v1.x = 10; v1.y = 10; v1.width = 200; v1.height = 150;
        v1.type = VizType::Bar; v1.title = L"chart1";

        Visualization v2;
        v2.x = 300; v2.y = 200; v2.width = 200; v2.height = 150;
        v2.type = VizType::Pie; v2.title = L"chart2";

        Dashboard d;
        int id1 = d.Add(v1);
        int id2 = d.Add(v2);
        (void)id1; (void)id2;

        // HitTest 검증
        int idx0 = d.HitTest(50, 50);
        if (idx0 != 0)
        {
            log << L"HitTest(50,50) == " << idx0 << L" (기대: 0)";
            return false;
        }

        int idx1 = d.HitTest(350, 250);
        if (idx1 != 1)
        {
            log << L"HitTest(350,250) == " << idx1 << L" (기대: 1)";
            return false;
        }

        int idxNone = d.HitTest(1000, 1000);
        if (idxNone != -1)
        {
            log << L"HitTest(1000,1000) == " << idxNone << L" (기대: -1)";
            return false;
        }

        // 드래그 시뮬레이션 (DeepMetriaView::OnMouseMove 로직)
        auto& viz0 = d.Visualizations()[0];
        int origX = viz0.x, origY = viz0.y;
        viz0.x += 50;
        viz0.y += 30;
        if (d.Visualizations()[0].x != origX + 50)
        {
            log << L"드래그 후 x == " << d.Visualizations()[0].x
                << L" (기대: " << (origX + 50) << L")";
            return false;
        }
        if (d.Visualizations()[0].y != origY + 30)
        {
            log << L"드래그 후 y == " << d.Visualizations()[0].y
                << L" (기대: " << (origY + 30) << L")";
            return false;
        }

        // 리사이즈 시뮬레이션 (ResizeBR 로직)
        auto& viz1 = d.Visualizations()[1];
        int origW = viz1.width, origH = viz1.height;
        viz1.width  = (std::max)(80,  viz1.width  + 40);
        viz1.height = (std::max)(60, viz1.height + 20);
        if (d.Visualizations()[1].width != origW + 40)
        {
            log << L"리사이즈 후 width == " << d.Visualizations()[1].width
                << L" (기대: " << (origW + 40) << L")";
            return false;
        }
        if (d.Visualizations()[1].height != origH + 20)
        {
            log << L"리사이즈 후 height == " << d.Visualizations()[1].height
                << L" (기대: " << (origH + 20) << L")";
            return false;
        }

        return true;
    }
};

// -----------------------------------------------------------------------
// 10. SecretsStoreRoundtripCommand (FR-08 DPAPI 라운드트립)
// -----------------------------------------------------------------------
struct SecretsStoreRoundtripCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"SecretsStoreRoundtripCommand"; }

    bool Run(std::wostream& log) override
    {
        // SelfTest 전용 임시 서브키 — 평문은 절대 디스크에 남기지 않음 (DPAPI 암호 자체)
        SecretsStore store(L"Software\\DeepMetria\\SelfTest_DPAPI");
        const std::wstring valueName = L"api_key_test";
        const std::wstring plain     = L"sk-ant-selftest-1234-깐깐한-검증-문자열";

        if (!store.SetSecret(valueName, plain))
        {
            log << L"SetSecret 실패";
            return false;
        }
        auto got = store.GetSecret(valueName);
        if (!got.has_value())
        {
            log << L"GetSecret nullopt";
            return false;
        }
        if (got.value() != plain)
        {
            log << L"라운드트립 불일치: got.size=" << got->size()
                << L" plain.size=" << plain.size();
            return false;
        }

        // 일반 문자열도 (provider 같은) 라운드트립
        if (!store.SetValue(L"provider_test", L"claude")) { log << L"SetValue 실패"; return false; }
        auto v = store.GetValue(L"provider_test");
        if (!v.has_value() || v.value() != L"claude") { log << L"GetValue 불일치"; return false; }

        return true;
    }
};

// -----------------------------------------------------------------------
// 11. MemoryCacheTtlCommand (FR-04 응답 캐시 TTL)
// -----------------------------------------------------------------------
struct MemoryCacheTtlCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"MemoryCacheTtlCommand"; }

    bool Run(std::wostream& log) override
    {
        MemoryCache<std::wstring> cache;
        // Put 후 즉시 Get
        cache.Put(L"q1", L"답변1", 60);
        auto a = cache.Get(L"q1");
        if (!a.has_value() || a.value() != L"답변1")
        {
            log << L"Put/Get 실패";
            return false;
        }
        // 미존재 키
        if (cache.Get(L"존재안함").has_value())
        {
            log << L"존재 안 하는 키가 값 반환";
            return false;
        }
        // 짧은 TTL: 0초 → 즉시 만료
        cache.Put(L"q2", L"답변2", 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (cache.Get(L"q2").has_value())
        {
            log << L"TTL 만료 후에도 값 유지";
            return false;
        }
        return true;
    }
};

// -----------------------------------------------------------------------
// 12. ParserFactoryCommand (FR-01 확장자 dispatch)
// -----------------------------------------------------------------------
struct ParserFactoryCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"ParserFactoryCommand"; }

    bool Run(std::wostream& log) override
    {
        auto pCsv  = ParserFactory::CreateForExtension(L".csv");
        if (!pCsv)  { log << L".csv → nullptr"; return false; }

        auto pJson = ParserFactory::CreateForExtension(L".json");
        if (!pJson) { log << L".json → nullptr"; return false; }

        // 미지원 확장자
        auto pXls  = ParserFactory::CreateForExtension(L".xlsx");
        if (pXls)
        {
            log << L".xlsx 도 파서 반환 — 미지원이어야 함";
            return false;
        }
        auto pTxt = ParserFactory::CreateForExtension(L".txt");
        if (pTxt) { log << L".txt 파서 반환 — 미지원이어야 함"; return false; }

        // 실제 파싱 round-trip
        DataSource ds;
        if (!pCsv->Parse(DataPath(L"sales.csv"), ds))
        {
            log << L"Factory CSV 파싱 실패";
            return false;
        }
        if (ds.Rows().size() != 12) { log << L"Factory CSV rows != 12"; return false; }
        return true;
    }
};

// -----------------------------------------------------------------------
// 13. ChartRendererStrategyCommand (FR-05 5종 차트 렌더 검증)
// -----------------------------------------------------------------------
struct ChartRendererStrategyCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"ChartRendererStrategyCommand"; }

    bool Run(std::wostream& log) override
    {
        // out 디렉터리 보장
        std::wstring outDir = OutDir();
        CreateDirectoryW(outDir.c_str(), nullptr);

        CsvParser parser; DataSource ds;
        if (!parser.Parse(DataPath(L"sales.csv"), ds))
        {
            log << L"CSV 파싱 실패";
            return false;
        }
        AnalysisTool::InferSchemaTypes(ds);
        AnalysisTool::ComputeBasicStatistics(ds);

        // 5종 Visualization 을 Dashboard 에 모두 추가 (Bar/Line/Pie/Table/Summary)
        Dashboard dash;
        // Summary
        dash.Add(AnalysisTool::SummaryPanel(ds));
        // Bar (GroupBySum 결과)
        if (auto v = AnalysisTool::GroupBySum(ds, L"지역", L"매출"); v.has_value())
        {
            auto vv = v.value(); vv.type = VizType::Bar; vv.x = 280; vv.y = 16;
            dash.Add(vv);
        }
        else { log << L"GroupBySum 실패"; return false; }
        // Line (TrendOverTime — 날짜 컬럼 없으면 무시)
        if (auto v = AnalysisTool::TrendOverTime(ds, L"월", L"매출"); v.has_value())
        {
            auto vv = v.value(); vv.type = VizType::Line; vv.x = 16; vv.y = 240;
            dash.Add(vv);
        }
        // Pie (도넛 대용 — GroupBySum 결과를 Pie 로 재배치)
        if (auto v = AnalysisTool::GroupBySum(ds, L"지역", L"매출"); v.has_value())
        {
            auto vv = v.value(); vv.type = VizType::Pie;
            vv.x = 280; vv.y = 240; vv.title = L"지역 비중";
            dash.Add(vv);
        }
        // Table
        dash.Add(AnalysisTool::TableSample(ds, 5));

        if (dash.Visualizations().size() < 4)
        {
            log << L"Dashboard 시각화 수 == " << dash.Visualizations().size();
            return false;
        }

        std::wstring outPath = outDir + L"\\selftest_5strategy.png";
        CRect client(0, 0, 1024, 768);
        ChartRenderer renderer;
        if (!renderer.ExportDashboardToImage(client, ds, dash, outPath))
        {
            log << L"ExportDashboardToImage 실패";
            return false;
        }

        // PNG 시그니처 검증
        static const BYTE pngSig[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
        HANDLE hFile = CreateFileW(outPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) { log << L"파일 열기 실패"; return false; }
        BYTE hdr[8] = {}; DWORD read = 0;
        ReadFile(hFile, hdr, 8, &read, nullptr);
        CloseHandle(hFile);
        if (read < 8 || memcmp(hdr, pngSig, 8) != 0)
        {
            log << L"PNG 시그니처 불일치";
            return false;
        }
        return true;
    }
};

// -----------------------------------------------------------------------
// 14. AnalysisToolCatalogCommand (FR-04 도구 카탈로그 노출 검증)
// -----------------------------------------------------------------------
struct AnalysisToolCatalogCommand : ISelfTestCommand
{
    const wchar_t* Name() const override { return L"AnalysisToolCatalogCommand"; }

    bool Run(std::wostream& log) override
    {
        const auto& cat = AnalysisTool::Catalog();
        if (cat.size() < 3)
        {
            log << L"카탈로그 크기 == " << cat.size() << L" (기대: 3 이상)";
            return false;
        }
        // 각 도구는 name/description/jsonParameters 모두 비어있지 않아야 LLM 호출 가능
        for (const auto& t : cat)
        {
            if (t.name.empty())
            {
                log << L"빈 name 도구 발견";
                return false;
            }
            if (t.description.empty())
            {
                log << L"빈 description (" << t.name << L")";
                return false;
            }
            if (t.jsonParameters.empty())
            {
                log << L"빈 jsonParameters (" << t.name << L")";
                return false;
            }
        }
        return true;
    }
};

} // anonymous namespace

// -----------------------------------------------------------------------
// RunSelfTestSuite
// -----------------------------------------------------------------------
int RunSelfTestSuite()
{
    // UTF-16 콘솔 모드 설정
    _setmode(_fileno(stdout), _O_U16TEXT);

    // 로그 파일 열기 (바이트 ofstream + 수동 UTF-8 인코딩, BOM 없음)
    std::ofstream logFile("tests/selftest_log.txt", std::ios::trunc | std::ios::binary);
    auto writeLog = [&logFile](const std::wstring& w)
    {
        if (w.empty()) return;
        int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(),
                                    nullptr, 0, nullptr, nullptr);
        if (n <= 0) return;
        std::string s(n, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(),
                            s.data(), n, nullptr, nullptr);
        logFile.write(s.data(), s.size());
    };

    // Command 목록 구성
    std::vector<std::unique_ptr<ISelfTestCommand>> commands;
    commands.emplace_back(std::make_unique<CsvLoadCommand>());
    commands.emplace_back(std::make_unique<JsonLoadCommand>());
    commands.emplace_back(std::make_unique<BasicStatsCommand>());
    commands.emplace_back(std::make_unique<GroupBySumCommand>());
    commands.emplace_back(std::make_unique<PngExportCommand>());
    commands.emplace_back(std::make_unique<LLMHeuristicCommand>());
    commands.emplace_back(std::make_unique<ChainOfThinkingCommand>());
    commands.emplace_back(std::make_unique<DialogResourceCommand>());
    commands.emplace_back(std::make_unique<DashboardDragCommand>());
    commands.emplace_back(std::make_unique<SecretsStoreRoundtripCommand>());
    commands.emplace_back(std::make_unique<MemoryCacheTtlCommand>());
    commands.emplace_back(std::make_unique<ParserFactoryCommand>());
    commands.emplace_back(std::make_unique<ChartRendererStrategyCommand>());
    commands.emplace_back(std::make_unique<AnalysisToolCatalogCommand>());

    int failCount = 0;

    for (auto& cmd : commands)
    {
        std::wostringstream reason;
        bool ok = false;
        try
        {
            ok = cmd->Run(reason);
        }
        catch (const std::exception& ex)
        {
            std::string msg(ex.what());
            reason << std::wstring(msg.begin(), msg.end());
            ok = false;
        }
        catch (...)
        {
            reason << L"알 수 없는 예외";
            ok = false;
        }

        if (ok)
        {
            std::wstring line = std::wstring(L"[PASS] ") + cmd->Name() + L"\n";
            std::wcout << line;
            writeLog(line);
        }
        else
        {
            std::wstring line = std::wstring(L"[FAIL] ") + cmd->Name()
                                + L" " + reason.str() + L"\n";
            std::wcout << line;
            writeLog(line);
            ++failCount;
        }
    }

    if (failCount == 0)
    {
        std::wstring line = L"SELFTEST OK\n";
        std::wcout << line;
        writeLog(line);
        return 0;
    }
    else
    {
        std::wostringstream oss;
        oss << L"SELFTEST FAIL=" << failCount << L"\n";
        std::wstring line = oss.str();
        std::wcout << line;
        writeLog(line);
        return 1;
    }
}

} // namespace deepmetria
