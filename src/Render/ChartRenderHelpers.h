// Render/ChartRenderHelpers.h - 차트 전략 공용 인라인 헬퍼
#pragma once
#include <string>
#include <gdiplus.h>

namespace deepmetria {

inline Gdiplus::Color FromArgb24(DWORD rgb, BYTE a = 255)
{
    return Gdiplus::Color(a,
        static_cast<BYTE>((rgb >> 16) & 0xFF),
        static_cast<BYTE>((rgb >> 8)  & 0xFF),
        static_cast<BYTE>( rgb        & 0xFF));
}

inline void DrawLabel(Gdiplus::Graphics& g, const std::wstring& s,
                      Gdiplus::REAL x, Gdiplus::REAL y,
                      Gdiplus::REAL w, Gdiplus::REAL h,
                      Gdiplus::Color color, int sizePx = 12, bool bold = false,
                      Gdiplus::StringAlignment align = Gdiplus::StringAlignmentNear)
{
    Gdiplus::FontFamily ff(L"Segoe UI");
    Gdiplus::Font font(&ff, static_cast<Gdiplus::REAL>(sizePx),
                       bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular,
                       Gdiplus::UnitPixel);
    Gdiplus::SolidBrush br(color);
    Gdiplus::StringFormat sf;
    sf.SetAlignment(align);
    sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::RectF r(x, y, w, h);
    g.DrawString(s.c_str(), -1, &font, r, &sf, &br);
}

inline void DrawValue(Gdiplus::Graphics& g, double v,
                      Gdiplus::REAL x, Gdiplus::REAL y,
                      Gdiplus::REAL w, Gdiplus::REAL h,
                      Gdiplus::Color color)
{
    wchar_t buf[64];
    if (std::abs(v) >= 1.0) swprintf_s(buf, L"%.1f", v);
    else                    swprintf_s(buf, L"%.3f", v);
    DrawLabel(g, buf, x, y, w, h, color, 10);
}

} // namespace deepmetria
