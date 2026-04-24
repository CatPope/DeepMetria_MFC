#pragma once

// MFC 표준 포함 파일
// 자주 사용되지만 거의 변경되지 않는 표준 시스템 포함 파일 또는
// 프로젝트 전용 포함 파일

#ifndef VC_EXTRA_LEAN
#define VC_EXTRA_LEAN
#endif

// 유니코드 문자 집합
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif

// MFC 핵심 및 표준 구성 요소
#include <afxwin.h>         // MFC 핵심 및 표준 구성 요소
#include <afxext.h>         // MFC 확장 (툴바, 상태바, splitter 등)
#include <afxcmn.h>         // 공용 컨트롤에 대한 MFC 지원
#include <afxcontrolbars.h> // 도킹 가능 컨트롤 막대 (MFC 9.0+)

// STL
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <functional>
#include <algorithm>

// GDI+
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// Windows 공통 헤더
#include <windows.h>
