// SettingsDialog.cpp - 설정 모달 구현
#include "stdafx.h"
#include "SettingsDialog.h"
#include "DeepMetria.h"
#include "SecretsStore.h"
#include "LLMRouter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CSettingsDialog, CDialogEx)

BEGIN_MESSAGE_MAP(CSettingsDialog, CDialogEx)
    ON_LBN_SELCHANGE(IDC_SETTINGS_CATEGORY, &CSettingsDialog::OnCategorySelChange)
    ON_CBN_SELCHANGE(IDC_SETTINGS_PROVIDER, &CSettingsDialog::OnProviderSelChange)
    ON_BN_CLICKED(IDC_SETTINGS_APPLY,       &CSettingsDialog::OnBtnApply)
    ON_BN_CLICKED(IDC_SETTINGS_TEST,        &CSettingsDialog::OnBtnTestConnection)
END_MESSAGE_MAP()

CSettingsDialog::CSettingsDialog(CWnd* pParent)
    : CDialogEx(IDD_SETTINGS, pParent)
{
}

CSettingsDialog::~CSettingsDialog() = default;

void CSettingsDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SETTINGS_CATEGORY, m_lstCategory);
    DDX_Control(pDX, IDC_SETTINGS_PROVIDER, m_cmbProvider);
    DDX_Control(pDX, IDC_SETTINGS_MODEL,    m_cmbModel);
    DDX_Control(pDX, IDC_SETTINGS_APIKEY,   m_edApiKey);
}

BOOL CSettingsDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 카테고리 — LLM 설정 단일
    m_lstCategory.AddString(_T("LLM 설정"));
    m_lstCategory.SetCurSel(0);

    // 제공자 목록
    m_cmbProvider.AddString(_T("Anthropic Claude"));
    m_cmbProvider.AddString(_T("OpenAI"));
    m_cmbProvider.AddString(_T("Google Gemini"));

    // 현재 라우터의 제공자 + 모델 가져오기
    int providerIdx = 0;
    std::wstring currentModel;
    if (theApp.m_llmRouter)
    {
        using P = deepmetria::LLMRouter::Provider;
        switch (theApp.m_llmRouter->CurrentProvider())
        {
        case P::Claude: providerIdx = 0; break;
        case P::OpenAI: providerIdx = 1; break;
        case P::Gemini: providerIdx = 2; break;
        default:        providerIdx = 0; break;
        }
        currentModel = theApp.m_llmRouter->Model();
    }
    m_cmbProvider.SetCurSel(providerIdx);

    // 모델 콤보 채우기 (제공자별)
    PopulateModelsForProvider(providerIdx);

    // 현재 모델이 있으면 선택, 없으면 첫 번째
    if (!currentModel.empty())
    {
        int idx = m_cmbModel.FindStringExact(0, CString(currentModel.c_str()));
        if (idx == CB_ERR) idx = 0;
        m_cmbModel.SetCurSel(idx);
    }
    else
    {
        m_cmbModel.SetCurSel(0);
    }

    // API 키: 보안상 평문 표시 안 함. 빈 상태 + cue banner.
    m_edApiKey.SetWindowText(_T(""));
    m_edApiKey.SetCueBanner(L"새 API 키를 붙여넣으세요 (비워두면 기존 키 유지)");

    return TRUE;
}

void CSettingsDialog::PopulateModelsForProvider(int providerIdx)
{
    m_cmbModel.ResetContent();
    switch (providerIdx)
    {
    case 0:  // Claude
        m_cmbModel.AddString(_T("claude-3-5-sonnet-20241022"));
        m_cmbModel.AddString(_T("claude-3-5-haiku-20241022"));
        m_cmbModel.AddString(_T("claude-3-opus-20240229"));
        break;
    case 1:  // OpenAI
        m_cmbModel.AddString(_T("gpt-4o"));
        m_cmbModel.AddString(_T("gpt-4o-mini"));
        m_cmbModel.AddString(_T("gpt-4-turbo"));
        break;
    case 2:  // Gemini — ListModels API 확인 결과 (. 대신 - 사용)
        m_cmbModel.AddString(_T("gemini-3.5-flash"));
        m_cmbModel.AddString(_T("gemini-3.1-pro-preview"));
        m_cmbModel.AddString(_T("gemini-3.1-flash-lite"));
        m_cmbModel.AddString(_T("gemini-3-pro-preview"));
        m_cmbModel.AddString(_T("gemini-3-flash-preview"));
        m_cmbModel.AddString(_T("gemini-2.5-pro"));
        m_cmbModel.AddString(_T("gemini-2.5-flash"));
        m_cmbModel.AddString(_T("gemini-2.5-flash-lite"));
        m_cmbModel.AddString(_T("gemini-2.0-flash"));
        m_cmbModel.AddString(_T("gemini-flash-latest"));
        m_cmbModel.AddString(_T("gemini-pro-latest"));
        break;
    default:
        break;
    }
}

void CSettingsDialog::OnCategorySelChange()
{
    // 단일 카테고리이므로 동작 없음
}

void CSettingsDialog::OnProviderSelChange()
{
    // 제공자 변경 → 모델 콤보 재구성
    int idx = m_cmbProvider.GetCurSel();
    if (idx < 0) return;
    PopulateModelsForProvider(idx);
    m_cmbModel.SetCurSel(0);
    // 키 입력란 비움 (제공자가 바뀌면 키도 다름)
    m_edApiKey.SetWindowText(_T(""));
}

void CSettingsDialog::SaveSettings()
{
    int provIdx = m_cmbProvider.GetCurSel();
    std::wstring provider;
    LPCWSTR keyName = nullptr;
    switch (provIdx)
    {
    case 0: provider = L"claude"; keyName = L"ApiKey.Claude"; break;
    case 1: provider = L"openai"; keyName = L"ApiKey.OpenAI"; break;
    case 2: provider = L"gemini"; keyName = L"ApiKey.Gemini"; break;
    default: return;  // 잘못된 선택
    }

    CString model;
    int mIdx = m_cmbModel.GetCurSel();
    if (mIdx >= 0) m_cmbModel.GetLBText(mIdx, model);

    CString apiKey;
    m_edApiKey.GetWindowText(apiKey);
    apiKey.Trim();

    // DPAPI 레지스트리에 저장 (HKCU\Software\DeepMetria\Settings)
    deepmetria::SecretsStore store(L"Software\\DeepMetria\\Settings");
    store.SetValue(L"Provider", provider);
    if (!model.IsEmpty())
        store.SetValue(L"Model", std::wstring(model.GetString()));

    // API 키는 비어있지 않을 때만 저장 (기존 키 보존)
    if (!apiKey.IsEmpty() && keyName)
        store.SetSecret(keyName, std::wstring(apiKey.GetString()));

    // LLMRouter 즉시 재로드 → 메인 프레임 상태바도 새 값으로 갱신됨
    if (theApp.m_llmRouter)
        theApp.m_llmRouter->LoadConfig();
}

void CSettingsDialog::OnOK()
{
    SaveSettings();
    CDialogEx::OnOK();
}

// [적용] — 저장만 하고 다이얼로그 유지
void CSettingsDialog::OnBtnApply()
{
    SaveSettings();
    AfxMessageBox(_T("저장되었습니다."), MB_OK | MB_ICONINFORMATION);
}

// [연결 테스트] — 현재 입력으로 짧은 요청 보내 응답 확인
void CSettingsDialog::OnBtnTestConnection()
{
    // 먼저 현재 입력값을 저장 → 라우터 재로드 → 테스트 호출
    SaveSettings();

    if (!theApp.m_llmRouter)
    {
        AfxMessageBox(_T("라우터가 초기화되지 않았습니다."),
                      MB_OK | MB_ICONERROR);
        return;
    }
    if (!theApp.m_llmRouter->IsConfigured())
    {
        AfxMessageBox(
            _T("제공자/API 키가 설정되지 않았습니다.\n")
            _T("API 키 입력란을 채우고 다시 시도하세요."),
            MB_OK | MB_ICONWARNING);
        return;
    }

    // 매우 짧은 ping 형태의 PlanRequest. 응답이 ok면 성공.
    CWaitCursor wc;
    deepmetria::LLMRouter::PlanRequest req;
    req.question    = L"ping";
    req.dataSummary = L"connectivity test";

    auto resp = theApp.m_llmRouter->Plan(req);
    if (resp.ok)
    {
        AfxMessageBox(_T("● 연결 성공\n\n응답 OK."),
                      MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        CString msg = _T("● 연결 실패\n\n");
        if (!resp.error.empty()) msg += CString(resp.error.c_str());
        else                     msg += _T("(상세 오류 없음)");
        AfxMessageBox(msg, MB_OK | MB_ICONERROR);
    }
}
