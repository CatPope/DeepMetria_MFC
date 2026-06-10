// Common/Messages.h - WM_USER 메시지 정의
#pragma once
#include <string>

namespace deepmetria {

// WM_USER 메시지 (UI 갱신용 비동기 통지 — Worker 스레드 → UI 스레드)
constexpr UINT WM_USER_ANALYSIS_PROGRESS    = WM_USER + 101;
constexpr UINT WM_USER_SUMMARY_READY        = WM_USER + 102;
constexpr UINT WM_USER_VISUALIZATION_READY  = WM_USER + 103;
constexpr UINT WM_USER_EXPORT_REQUEST       = WM_USER + 104;
constexpr UINT WM_USER_LLM_ERROR            = WM_USER + 105;
constexpr UINT WM_USER_LOAD_SAMPLE          = WM_USER + 107;
constexpr UINT WM_USER_RIBBON_TAB_CHANGED   = WM_USER + 108;
constexpr UINT WM_USER_QUERY_SUBMIT         = WM_USER + 109;
constexpr UINT WM_USER_VIZ_SELECTED         = WM_USER + 110;
constexpr UINT WM_USER_VIZ_FORMAT_APPLIED   = WM_USER + 111;

} // namespace deepmetria
