//{{NO_DEPENDENCIES}}
// resource.h - DeepMetria MFC 리소스 ID
#pragma once

#define IDR_MAINFRAME                   128
#define IDR_DeepMetriaTYPE              129
#define IDD_ABOUTBOX                    130
#define IDD_EXPORT                      131
#define IDD_SETTINGS                    132
#define IDD_RETRY                       133

// 기존 레거시 IDD (DialogResourceCommand SelfTest 유지용 — 절대 변경 금지)
#define IDD_QUERY_INPUT                 200
#define IDC_QUERY_EDIT                  1001
#define IDC_QUERY_SUBMIT                1002
#define IDC_PROVIDER_COMBO              1003

#define IDC_STATIC                      (-1)

// Export 다이얼로그 컨트롤
#define IDC_EXPORT_PREVIEW              1010
#define IDC_EXPORT_PATH                 1011
#define IDC_EXPORT_BROWSE               1012
#define IDC_EXPORT_RADIO_SINGLE         1013
#define IDC_EXPORT_RADIO_ALL            1014
#define IDC_EXPORT_FORMAT_PNG           1015
#define IDC_EXPORT_FORMAT_BMP           1016
#define IDC_EXPORT_FORMAT_JPG           1017
#define IDC_EXPORT_FORMAT_PDF           1018
#define IDC_EXPORT_RESOLUTION           1019

// Settings 다이얼로그 컨트롤
#define IDC_SETTINGS_CATEGORY           1020
#define IDC_SETTINGS_MODEL              1021
#define IDC_SETTINGS_APIKEY             1022
#define IDC_SETTINGS_PROVIDER           1023
#define IDC_SETTINGS_APPLY              1024
#define IDC_SETTINGS_TEST               1025

// Retry 다이얼로그 컨트롤
#define IDC_RETRY_RETRY                 1030
#define IDC_RETRY_SWITCH                1031

// 기존 AI/분석 명령 ID (SelfTest, test_runner.py 직접 참조 — 절대 변경 금지)
#define ID_AI_ANALYZE                   32771
#define ID_AI_SUMMARIZE                 32772
#define ID_VIEW_DASHBOARD               32773
#define ID_EXPORT_PNG                   32774
#define ID_FORMAT_VISUALIZATION         32775

// 보기 메뉴
#define ID_VIEW_DATA_SUMMARY            32776
#define ID_VIEW_QUERY_INPUT             32777

// 분석 메뉴
#define ID_QUERY_INPUT                  32778
#define ID_VIEW_CHART                   32779
#define ID_ANALYSIS_AUTO_SUMMARY        32780

// 내보내기
#define ID_FILE_EXPORT                  32781
#define ID_EXPORT_IMAGE                 32782
#define ID_EXPORT_DASHBOARD             32783

// 리본 바 탭 ID
#define ID_VIEW_SUMMARY                 32784
#define ID_VIEW_QUERY                   32785
#define ID_VIEW_FORMAT                  32786
#define ID_APP_SETTINGS                 32787
#define ID_VIEW_RIBBON                  32788

// 상태바 인디케이터
#define ID_INDICATOR_AI                 59142
#define ID_INDICATOR_PROGRESS           59143
#define ID_INDICATOR_LLM                59144

// Next default values for new objects
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        201
#define _APS_NEXT_COMMAND_VALUE         32800
#define _APS_NEXT_CONTROL_VALUE         1100
#define _APS_NEXT_SYMED_VALUE           300
#endif
#endif
