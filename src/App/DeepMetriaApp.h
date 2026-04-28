#pragma once

// DeepMetriaApp.h — CWinApp 파생 앱 클래스
// 앱 진입점, 초기화/정리 담당

#include "../stdafx.h"

// ============================================================
// CDeepMetriaApp 클래스 선언
// ============================================================
class CDeepMetriaApp : public CWinApp
{
public:
    CDeepMetriaApp();
    virtual ~CDeepMetriaApp() = default;

    // 오버라이드
    virtual BOOL InitInstance() override;
    virtual int  ExitInstance() override;

    // GDI+ 토큰 (InitInstance에서 초기화, ExitInstance에서 해제)
    ULONG_PTR m_gdiplusToken;

    // 메시지 핸들러
    afx_msg void OnAppAbout();

    DECLARE_MESSAGE_MAP()
};

// 전역 앱 인스턴스 (MFC 관례: theApp)
extern CDeepMetriaApp theApp;
