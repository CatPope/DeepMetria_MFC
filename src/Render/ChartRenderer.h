// Render/ChartRenderer.h - GDI+ 기반 대시보드 렌더
#pragma once
#include "DataSource.h"
#include "Dashboard.h"

namespace Gdiplus { class Graphics; }

namespace deepmetria {

class ChartRenderer
{
public:
    // 전체 대시보드를 클라이언트 사각형에 렌더
    void RenderDashboard(Gdiplus::Graphics& g, const CRect& client,
                         const DataSource& ds, const Dashboard& dash);

    // 대시보드를 메모리 비트맵에 그린 뒤 path 로 저장 (PNG/BMP/JPEG)
    bool ExportDashboardToImage(const CRect& client,
                                const DataSource& ds,
                                const Dashboard& dash,
                                const std::wstring& path);

private:
    void DrawCard(Gdiplus::Graphics& g, const Visualization& v);

    static int GetEncoderClsid(const WCHAR* mime, CLSID* clsid);
};

} // namespace deepmetria
