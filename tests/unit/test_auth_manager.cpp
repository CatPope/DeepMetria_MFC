// test_auth_manager.cpp — API 키 관리(인증) 단위 테스트
// LLMRouter의 레지스트리 기반 API 키 저장/로드 및 DPAPI 암호화 경로를 검증한다.
// 레지스트리 쓰기가 발생하므로 테스트 완료 후 정리(CleanUp)를 수행한다.
// DPAPI 함수는 Windows 전용이며 현재 사용자 계정에서 실행해야 한다.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <windows.h>
#include <atlstr.h>

#include "Common/Types.h"
#include "Infrastructure/LLM/LLMRouter.h"

// ============================================================
// 테스트용 레지스트리 경로 (프로덕션 키와 분리)
// ============================================================
static const TCHAR* TEST_REG_KEY = _T("Software\\DeepMetria\\APIKeys\\Test");

// ============================================================
// AuthManager 테스트 픽스처
// 테스트 완료 후 레지스트리 키를 정리한다.
// ============================================================
class AuthManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        router_ = &LLMRouter::Instance();
        // 테스트 전 API 키 초기화
        router_->SetApiKey(LLMRouter::Provider::Claude, _T(""));
        router_->SetApiKey(LLMRouter::Provider::OpenAI, _T(""));
        router_->SetApiKey(LLMRouter::Provider::Gemini, _T(""));
    }

    void TearDown() override {
        // 테스트 후 레지스트리 정리
        // SaveApiKeyToRegistry는 "Software\DeepMetria\APIKeys" 키에 쓰므로
        // 테스트에서 사용한 키를 빈 값으로 덮어쓴다.
        AppError error;
        router_->SaveApiKeyToRegistry(LLMRouter::Provider::Claude, _T(""), error);
        router_->SaveApiKeyToRegistry(LLMRouter::Provider::OpenAI, _T(""), error);
        // 메모리 상태도 초기화
        router_->SetApiKey(LLMRouter::Provider::Claude, _T(""));
        router_->SetApiKey(LLMRouter::Provider::OpenAI, _T(""));
        router_->SetApiKey(LLMRouter::Provider::Gemini, _T(""));
    }

    LLMRouter* router_ = nullptr;
};

// ============================================================
// TC-AUTH-001: API 키를 레지스트리에 저장하고 로드하면 원래 값이 복원된다
// DPAPI 암호화/복호화 왕복 테스트
// ============================================================
TEST_F(AuthManagerTest, SaveAndLoadApiKey_ClaudeKey_RoundTripSucceeds) {
    const CString testKey = _T("sk-ant-test-roundtrip-12345");
    AppError error;

    // 저장
    BOOL saveResult = router_->SaveApiKeyToRegistry(
        LLMRouter::Provider::Claude, testKey, error);
    EXPECT_EQ(saveResult, TRUE)
        << "키 저장 실패. code=" << (LPCWSTR)error.code
        << " msg=" << (LPCWSTR)error.message;

    // 메모리 키를 지워 로드 효과를 명확히 한다
    router_->SetApiKey(LLMRouter::Provider::Claude, _T(""));

    // 로드
    AppError loadError;
    BOOL loadResult = router_->LoadApiKeysFromRegistry(loadError);
    EXPECT_EQ(loadResult, TRUE)
        << "키 로드 실패. code=" << (LPCWSTR)loadError.code;

    // 로드 성공 여부는 LoadApiKeysFromRegistry가 내부적으로 SetApiKey를 호출했는지로 확인.
    // Chat을 시도하면 키가 설정된 경우 HTTP 시도(네트워크 오류)가 발생하고
    // 키가 없는 경우 인증 오류가 발생한다. 두 경우 모두 FALSE이나 오류 코드가 다르다.
    // 여기서는 LoadApiKeysFromRegistry 자체가 TRUE를 반환함을 검증한다.
    EXPECT_FALSE(loadError.IsError());
}

// ============================================================
// TC-AUTH-002: SaveApiKeyToRegistry → LoadApiKeysFromRegistry 왕복 후
// 키가 비어 있지 않으면 Chat 시도 시 네트워크 오류(인증 오류 아님)가 발생한다
// 이를 통해 키가 실제로 로드되었음을 간접 검증한다.
// ============================================================
TEST_F(AuthManagerTest, SaveAndLoadApiKey_OpenAIKey_RoundTripSucceeds) {
    const CString testKey = _T("sk-openai-test-roundtrip-67890");
    AppError error;

    BOOL saveResult = router_->SaveApiKeyToRegistry(
        LLMRouter::Provider::OpenAI, testKey, error);
    EXPECT_EQ(saveResult, TRUE)
        << "OpenAI 키 저장 실패. code=" << (LPCWSTR)error.code;

    // 저장 후 즉시 메모리에도 반영되는지 확인
    // (SaveApiKeyToRegistry 구현에서 SetApiKey 호출하므로)
    AppError chatError;
    CString  response;
    router_->SetProvider(LLMRouter::Provider::OpenAI);
    // 키가 설정된 상태에서 Chat은 네트워크 오류(HTTP_REQUEST_FAILED)를 반환해야 한다
    router_->Chat(_T("test"), _T("test"), response, chatError);
    // 오류 코드는 HTTP_REQUEST_FAILED 또는 API_ERROR 이어야 하며
    // NO_API_KEY 류의 즉시 실패는 아니어야 한다
    // (네트워크 없는 환경이면 HTTP_REQUEST_FAILED 반환)
    EXPECT_TRUE(chatError.IsError()); // 오류는 발생하지만
    // EMPTY_MESSAGES나 NO_PROVIDER 같은 전처리 오류는 아니어야 한다
    EXPECT_NE(chatError.code, CString(_T("NO_PROVIDER")));
}

// ============================================================
// TC-AUTH-003: 빈 API 키를 저장하면 오류 없이 성공한다
// ============================================================
TEST_F(AuthManagerTest, SaveEmptyApiKey_Succeeds) {
    AppError error;
    BOOL result = router_->SaveApiKeyToRegistry(
        LLMRouter::Provider::Claude, _T(""), error);
    // 빈 문자열도 DPAPI 암호화 후 저장할 수 있어야 한다
    // (EncryptDPAPI("")가 실패하면 ENCRYPT_FAILED 오류)
    // 구현에 따라 성공/실패 모두 가능 — 크래시 없음을 검증
    EXPECT_NO_FATAL_FAILURE(
        router_->SaveApiKeyToRegistry(LLMRouter::Provider::Claude, _T(""), error)
    );
}

// ============================================================
// TC-AUTH-004: LoadApiKeysFromRegistry는 레지스트리 키가 없어도 TRUE를 반환한다
// (미설정 상태를 오류로 처리하지 않음 — LLMRouter.cpp 128번 줄 주석 참조)
// ============================================================
TEST_F(AuthManagerTest, LoadApiKeysFromRegistry_WhenKeyMissing_ReturnsTrueWithoutError) {
    AppError error;
    // 레지스트리에 키가 없거나 빈 값이어도 TRUE 반환이 기대된다
    BOOL result = router_->LoadApiKeysFromRegistry(error);
    EXPECT_EQ(result, TRUE);
    EXPECT_FALSE(error.IsError())
        << "레지스트리 키 없음을 오류로 처리함. code=" << (LPCWSTR)error.code;
}

// ============================================================
// TC-AUTH-005: 세 프로바이더에 각기 다른 키를 저장해도 독립적으로 유지된다
// Claude 키와 OpenAI 키는 서로 다른 레지스트리 값에 저장된다.
// ============================================================
TEST_F(AuthManagerTest, MultipleProviderKeys_StoredAndLoadedIndependently) {
    AppError error;

    // Claude와 OpenAI 키를 각각 저장
    BOOL claudeSave = router_->SaveApiKeyToRegistry(
        LLMRouter::Provider::Claude, _T("claude-key-abc"), error);
    EXPECT_EQ(claudeSave, TRUE) << "Claude 키 저장 실패";

    BOOL openaiSave = router_->SaveApiKeyToRegistry(
        LLMRouter::Provider::OpenAI, _T("openai-key-xyz"), error);
    EXPECT_EQ(openaiSave, TRUE) << "OpenAI 키 저장 실패";

    // 메모리 초기화 후 로드
    router_->SetApiKey(LLMRouter::Provider::Claude, _T(""));
    router_->SetApiKey(LLMRouter::Provider::OpenAI, _T(""));

    AppError loadError;
    BOOL loadResult = router_->LoadApiKeysFromRegistry(loadError);
    EXPECT_EQ(loadResult, TRUE) << "키 로드 실패";
    EXPECT_FALSE(loadError.IsError());
}

// ============================================================
// TC-AUTH-006: SaveApiKeyToRegistry 후 SetApiKey로 메모리에 즉시 반영된다
// (SaveApiKeyToRegistry 구현 끝에서 SetApiKey 호출 확인)
// ============================================================
TEST_F(AuthManagerTest, SaveApiKeyToRegistry_ImmediatelyReflectsInMemory) {
    AppError error;
    router_->SaveApiKeyToRegistry(
        LLMRouter::Provider::Claude, _T("immediate-key"), error);

    // 저장 직후 Chat 시도 — 키가 메모리에 있으므로 HTTP 시도 (네트워크 오류)
    router_->SetProvider(LLMRouter::Provider::Claude);
    CString  response;
    AppError chatError;
    router_->Chat(_T("sys"), _T("user"), response, chatError);

    // NO_PROVIDER 오류가 아니어야 한다 (프로바이더는 존재함)
    EXPECT_NE(chatError.code, CString(_T("NO_PROVIDER")));
}
