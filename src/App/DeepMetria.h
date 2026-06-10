// DeepMetria.h - 메인 헤더. CWinAppEx 파생 응용 프로그램 클래스.
#pragma once
#ifndef __AFXWIN_H__
    #error "PCH에 대해 이 파일을 포함하기 전에 'stdafx.h'를 포함하세요."
#endif

#include "resource.h"
#include "LLMRouter.h"

class CDeepMetriaApp : public CWinAppEx
{
public:
    CDeepMetriaApp();

    virtual BOOL InitInstance() override;
    virtual int  ExitInstance() override;

    afx_msg void OnAppAbout();

    DECLARE_MESSAGE_MAP()

public:
    // GDI+ 토큰
    ULONG_PTR m_gdiplusToken = 0;

    // 전역 LLM Router (InitInstance에서 생성)
    std::shared_ptr<deepmetria::LLMRouter> m_llmRouter;
};

extern CDeepMetriaApp theApp;
