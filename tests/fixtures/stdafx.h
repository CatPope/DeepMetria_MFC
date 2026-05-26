#pragma once
// stdafx.h — 테스트 환경용 PCH 대체 스텁
// src/stdafx.h 는 MFC(afxwin.h)를 포함하므로 테스트 빌드에서 사용 불가.
// fixtures/ 디렉터리는 CMakeLists의 COMMON_INCLUDES에서 SRC_ROOT보다
// 먼저 나열되어 있으므로 이 파일이 src/stdafx.h 보다 우선 탐색된다.
//
// 포함 항목:
//   - Windows/ATL 핵심 헤더 (CString, BOOL 등 MFC 타입 제공)
//   - STL 표준 헤더
//   - 프로젝트 공용 타입 (Common/Types.h)
//
// 포함하지 않는 항목:
//   - afxwin.h / afxext.h / afxcmn.h (MFC 전용, 테스트 환경 불필요)
//   - GDI+ (gdiplus.h) — 분석/캐시 구현에서 미사용

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif

#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif

// Windows 핵심 헤더 (MFC 없이 CString 기반 타입 제공)
#include <windows.h>
#include <atlstr.h>   // CString (ATL — MFC 없이 독립 사용 가능)

// STL
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <functional>
#include <algorithm>
#include <ctime>

// 프로젝트 공용 타입 — afxwin.h 없이도 사용 가능한 구조체/typedef
#include "Common/Types.h"
