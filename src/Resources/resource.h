//{{NO_DEPENDENCIES}}
// Microsoft Visual C++에서 생성한 포함 파일.
// DeepMetria.rc에서 사용됩니다.

#pragma once

// 앱 리소스 ID
#define IDR_MAINFRAME               128
#define IDR_TOOLBAR                 129
#define IDR_MENU_MAINFRAME          130

// 다이얼로그 ID
#define IDD_ABOUTBOX                100
#define IDD_SETTINGS                101
#define IDD_EXPORT                  102
#define IDD_FILE_OPEN               103

// 메뉴 커맨드 ID
#define ID_FILE_OPEN_DATA           32771
#define ID_FILE_RECENT_FILES        32772
#define ID_ANALYSIS_NEW_QUERY       32780
#define ID_TOOLS_SETTINGS           32790
#define ID_TOOLS_EXPORT             32791
#define ID_HELP_ABOUT               32800

// 컨트롤 ID
#define IDC_STATIC_VERSION          1001
#define IDC_STATIC_COPYRIGHT        1002
#define IDC_EDIT_QUERY              1010
#define IDC_BUTTON_SEND             1011
#define IDC_LIST_HISTORY            1012
#define IDC_COMBO_LLM_PROVIDER      1020
#define IDC_EDIT_API_KEY            1021
#define IDC_EDIT_MODEL_NAME         1022
#define IDC_COMBO_EXPORT_FORMAT     1030
#define IDC_EDIT_EXPORT_PATH        1031
#define IDC_BUTTON_BROWSE           1032

// QueryInput 폼 다이얼로그 및 컨트롤 ID
// IDD_QUERYINPUT_FORM: 130은 IDR_MENU_MAINFRAME과 충돌하므로 140 사용
#define IDD_QUERYINPUT_FORM         140
// IDC_EDIT_QUERY(1010), IDC_BUTTON_SEND(1011), IDC_LIST_HISTORY(1012)는 기존 정의 재사용
#define IDC_BTN_ANALYZE             1033  // 1011=IDC_BUTTON_SEND 충돌 회피
#define IDC_STATIC_STATUS           1034  // 1012=IDC_LIST_HISTORY 충돌 회피
#define IDC_PROGRESS_BAR            1013

// 상태바 인디케이터 ID
#define ID_INDICATOR_STATUS         0xE700
#define ID_INDICATOR_PROGRESS       0xE701

// 다음 기본 ID 값 (Visual Studio 리소스 편집기 자동 관리)
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE    141
#define _APS_NEXT_COMMAND_VALUE     32810
#define _APS_NEXT_CONTROL_VALUE     1040
#define _APS_NEXT_SYMED_VALUE       104
#endif
