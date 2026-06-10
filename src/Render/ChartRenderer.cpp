// Render/ChartRenderer.cpp
#include "stdafx.h"
#include "ChartRenderer.h"
#include "ChartRenderHelpers.h"
#include "IChartStrategy.h"
#include "BarChartStrategy.h"
#include "LineChartStrategy.h"
#include "PieChartStrategy.h"
#include "TableChartStrategy.h"
#include "SummaryChartStrategy.h"
#include <algorithm>
#include <memory>

using namespace Gdiplus;

namespace deepmetria {

void ChartRenderer::RenderDashboard(Graphics& g, const CRect& client,
                                    const DataSource& ds, const Dashboard& dash)
{
    // 비어 있으면 안내
    if (ds.IsEmpty() && dash.Visualizations().empty())
    {
        SolidBrush br(FromArgb24(0x6B7280));
        Gdiplus::FontFamily ff(L"Segoe UI");
        Gdiplus::Font font(&ff, 18.0f, FontStyleRegular, UnitPixel);
        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        RectF r(0, 0, static_cast<REAL>(client.Width()), static_cast<REAL>(client.Height()));
        g.DrawString(L"파일 > 열기 로 CSV/JSON 을 불러오세요.\n그 다음 'Q' 키를 눌러 자연어 질문을 입력합니다.",
                     -1, &font, r, &sf, &br);
        return;
    }

    // (제거) 자동 "데이터 요약" 카드 — 메인 대시보드에서는 표시하지 않음.
    // 요약 정보는 좌측 사이드바(DataSummaryPane)에 집중.

    for (const auto& v : dash.Visualizations())
    {
        DrawCard(g, v);
        std::unique_ptr<IChartStrategy> s;
        switch (v.type)
        {
        case VizType::Table:   s.reset(new TableChartStrategy());   break;
        case VizType::Bar:     s.reset(new BarChartStrategy());     break;
        case VizType::Line:    s.reset(new LineChartStrategy());    break;
        case VizType::Pie:     s.reset(new PieChartStrategy());     break;
        case VizType::Summary: s.reset(new SummaryChartStrategy()); break;
        }
        if (s) s->Draw(g, v, &ds);
    }
}

void ChartRenderer::DrawCard(Graphics& g, const Visualization& v)
{
    SolidBrush bg(Color(255, 255, 255, 255));
    Pen border(Color(255, 226, 232, 240), 1.0f);
    g.FillRectangle(&bg, v.x, v.y, v.width, v.height);
    g.DrawRectangle(&border, v.x, v.y, v.width, v.height);

    // 타이틀 바 — showTitle 일 때만 헤더 그림. 미체크 시 공란.
    if (v.showTitle)
    {
        SolidBrush titleBg(Color(255, 248, 250, 252));
        g.FillRectangle(&titleBg, v.x, v.y, v.width, 28);
        DrawLabel(g, v.title, static_cast<REAL>(v.x + 10), static_cast<REAL>(v.y),
                 static_cast<REAL>(v.width - 20), 28,
                 FromArgb24(0x111827), 12, true);
    }

    // 우하단 리사이즈 그립 (16x16)
    Pen grip(Color(180, 100, 116, 139), 1.0f);
    for (int i = 0; i < 3; ++i)
        g.DrawLine(&grip,
            v.x + v.width - 4 - i * 4, v.y + v.height - 2,
            v.x + v.width - 2,        v.y + v.height - 4 - i * 4);
}

bool ChartRenderer::ExportDashboardToImage(const CRect& client,
                                           const DataSource& ds,
                                           const Dashboard& dash,
                                           const std::wstring& path)
{
    int w = std::max(64, static_cast<int>(client.Width()));
    int h = std::max(64, static_cast<int>(client.Height()));

    Bitmap bmp(w, h, PixelFormat32bppARGB);
    Graphics g(&bmp);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    SolidBrush bgBrush(FromArgb24(0xF8F9FB));
    g.FillRectangle(&bgBrush, 0, 0, w, h);
    RenderDashboard(g, client, ds, dash);

    // 확장자에 따라 MIME 결정
    std::wstring ext;
    auto dot = path.find_last_of(L'.');
    if (dot != std::wstring::npos) ext = path.substr(dot + 1);
    for (auto& c : ext) c = static_cast<wchar_t>(std::towlower(c));
    const wchar_t* mime = L"image/png";
    if (ext == L"bmp") mime = L"image/bmp";
    else if (ext == L"jpg" || ext == L"jpeg") mime = L"image/jpeg";

    CLSID enc{};
    if (GetEncoderClsid(mime, &enc) < 0) return false;
    return bmp.Save(path.c_str(), &enc, nullptr) == Ok;
}

int ChartRenderer::GetEncoderClsid(const WCHAR* mime, CLSID* clsid)
{
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    std::vector<BYTE> buf(size);
    auto* info = reinterpret_cast<ImageCodecInfo*>(buf.data());
    GetImageEncoders(num, size, info);
    for (UINT i = 0; i < num; ++i)
    {
        if (wcscmp(info[i].MimeType, mime) == 0)
        {
            *clsid = info[i].Clsid;
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace deepmetria
