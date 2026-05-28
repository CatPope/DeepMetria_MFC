// test_settings_manager.cpp — CSettingsManager 단위 테스트
// 레지스트리 기반 설정 저장/로드 및 DPAPI 암호화 경로를 검증한다.
// 테스트 완료 후 레지스트리 정리(TearDown)를 수행한다.
// DPAPI 함수는 Windows 전용이며 현재 사용자 계정에서 실행해야 한다.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <windows.h>
#include <atlstr.h>

#include "Common/Types.h"
#include "Infrastructure/Settings/CSettingsManager.h"

// ============================================================
// 테스트용 레지스트리 경로 (프로덕션 키와 분리)
// ============================================================
static const TCHAR* TEST_REG_KEY = _T("Software\\DeepMetria\\Settings\\Test");

// ============================================================
// SettingsManager 테스트 픽스처
// SetUp: 싱글턴 상태 초기화
// TearDown: 레지스트리 키 삭제 및 싱글턴 상태 초기화
// ============================================================
class SettingsManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mgr_ = &CSettingsManager::Instance();
        // 테스트 전 메모리 상태 초기화
        mgr_->SetProvider(_T("Claude"));
        mgr_->SetModel(_T(""));
        mgr_->SetApiKey(_T("Claude"),  _T(""));
        mgr_->SetApiKey(_T("OpenAI"),  _T(""));
        mgr_->SetApiKey(_T("Gemini"),  _T(""));
    }

    void TearDown() override {
        // 테스트 레지스트리 키 삭제 (프로덕션 키에 영향 없음)
        RegDeleteKey(HKEY_CURRENT_USER, TEST_REG_KEY);

        // 프로덕션 키에서 테스트 데이터 제거 (SaveToRegistry가 사용하는 경로)
        // 빈 값으로 덮어써서 잔류 암호문을 제거한다
        mgr_->SetProvider(_T("Claude"));
        mgr_->SetModel(_T(""));
        mgr_->SetApiKey(_T("Claude"),  _T(""));
        mgr_->SetApiKey(_T("OpenAI"),  _T(""));
        mgr_->SetApiKey(_T("Gemini"),  _T(""));

        AppError err;
        mgr_->SaveToRegistry(err);
    }

    CSettingsManager* mgr_ = nullptr;
};

// ============================================================
// TC-SETTINGS-001: 프로바이더 저장/로드 왕복
// SetProvider → SaveToRegistry → 메모리 초기화 → LoadFromRegistry
// 로드 후 GetProvider()가 원래 값을 반환해야 한다.
// ============================================================
TEST_F(SettingsManagerTest, Provider_SaveAndLoad_RoundTripSucceeds) {
    mgr_->SetProvider(_T("OpenAI"));

    AppError saveErr;
    BOOL saved = mgr_->SaveToRegistry(saveErr);
    EXPECT_EQ(saved, TRUE)
        << "저장 실패. code=" << (LPCWSTR)saveErr.code
        << " msg=" << (LPCWSTR)saveErr.message;

    // 메모리 초기화
    mgr_->SetProvider(_T("Claude"));
    EXPECT_EQ(mgr_->GetProvider(), CString(_T("Claude")));

    AppError loadErr;
    BOOL loaded = mgr_->LoadFromRegistry(loadErr);
    EXPECT_EQ(loaded, TRUE)
        << "로드 실패. code=" << (LPCWSTR)loadErr.code;
    EXPECT_FALSE(loadErr.IsError());

    EXPECT_EQ(mgr_->GetProvider(), CString(_T("OpenAI")))
        << "프로바이더 왕복 실패";
}

// ============================================================
// TC-SETTINGS-002: 모델 저장/로드 왕복
// SetModel → SaveToRegistry → 메모리 초기화 → LoadFromRegistry
// 로드 후 GetModel()이 원래 값을 반환해야 한다.
// ============================================================
TEST_F(SettingsManagerTest, Model_SaveAndLoad_RoundTripSucceeds) {
    mgr_->SetModel(_T("gpt-4o"));

    AppError saveErr;
    BOOL saved = mgr_->SaveToRegistry(saveErr);
    EXPECT_EQ(saved, TRUE)
        << "저장 실패. code=" << (LPCWSTR)saveErr.code;

    // 메모리 초기화
    mgr_->SetModel(_T(""));
    EXPECT_TRUE(mgr_->GetModel().IsEmpty());

    AppError loadErr;
    BOOL loaded = mgr_->LoadFromRegistry(loadErr);
    EXPECT_EQ(loaded, TRUE);
    EXPECT_FALSE(loadErr.IsError());

    EXPECT_EQ(mgr_->GetModel(), CString(_T("gpt-4o")))
        << "모델 왕복 실패";
}

// ============================================================
// TC-SETTINGS-003: API 키 DPAPI 암호화 왕복
// EncryptDPAPI → DecryptDPAPI 후 원문이 복원되어야 한다.
// ============================================================
TEST_F(SettingsManagerTest, ApiKey_DpapiEncryptDecrypt_RoundTripSucceeds) {
    const CString plainText = _T("sk-ant-test-dpapi-roundtrip-12345");

    CString cipher = CSettingsManager::EncryptDPAPI(plainText);
    EXPECT_FALSE(cipher.IsEmpty())
        << "DPAPI 암호화 실패";
    EXPECT_NE(cipher, plainText)
        << "암호화 후 평문과 달라야 함";

    CString decrypted = CSettingsManager::DecryptDPAPI(cipher);
    EXPECT_EQ(decrypted, plainText)
        << "DPAPI 복호화 후 원문과 다름";
}

// ============================================================
// TC-SETTINGS-004: 복수 프로바이더 독립 저장
// Claude/OpenAI/Gemini 키를 각각 저장하면 독립적으로 로드된다.
// ============================================================
TEST_F(SettingsManagerTest, MultipleProviders_StoredAndLoadedIndependently) {
    mgr_->SetApiKey(_T("Claude"),  _T("claude-key-abc"));
    mgr_->SetApiKey(_T("OpenAI"),  _T("openai-key-xyz"));
    mgr_->SetApiKey(_T("Gemini"),  _T("gemini-key-def"));

    AppError saveErr;
    BOOL saved = mgr_->SaveToRegistry(saveErr);
    EXPECT_EQ(saved, TRUE)
        << "저장 실패. code=" << (LPCWSTR)saveErr.code;

    // 메모리 초기화
    mgr_->SetApiKey(_T("Claude"),  _T(""));
    mgr_->SetApiKey(_T("OpenAI"),  _T(""));
    mgr_->SetApiKey(_T("Gemini"),  _T(""));

    AppError loadErr;
    BOOL loaded = mgr_->LoadFromRegistry(loadErr);
    EXPECT_EQ(loaded, TRUE);
    EXPECT_FALSE(loadErr.IsError());

    EXPECT_EQ(mgr_->GetApiKey(_T("Claude")),  CString(_T("claude-key-abc")))
        << "Claude 키 왕복 실패";
    EXPECT_EQ(mgr_->GetApiKey(_T("OpenAI")),  CString(_T("openai-key-xyz")))
        << "OpenAI 키 왕복 실패";
    EXPECT_EQ(mgr_->GetApiKey(_T("Gemini")),  CString(_T("gemini-key-def")))
        << "Gemini 키 왕복 실패";
}

// ============================================================
// TC-SETTINGS-005: 레지스트리 키 없을 때 기본값 반환
// LoadFromRegistry가 레지스트리 키 부재 시 TRUE를 반환하고
// 기본 프로바이더("Claude")를 유지해야 한다.
// ============================================================
TEST_F(SettingsManagerTest, LoadFromRegistry_WhenKeyMissing_ReturnsTrueWithDefaults) {
    // 테스트 환경에서 프로덕션 레지스트리 키를 삭제하여 미설정 상태 시뮬레이션
    // (실제 키를 삭제하면 다른 테스트에 영향을 줄 수 있으므로
    //  빈 상태로 저장 후 키 자체를 삭제)
    AppError delErr;
    mgr_->SetProvider(_T("Claude"));
    mgr_->SetModel(_T(""));
    mgr_->SetApiKey(_T("Claude"),  _T(""));
    mgr_->SetApiKey(_T("OpenAI"),  _T(""));
    mgr_->SetApiKey(_T("Gemini"),  _T(""));
    mgr_->SaveToRegistry(delErr);

    // 레지스트리 키 자체 삭제
    RegDeleteKey(HKEY_CURRENT_USER, _T("Software\\DeepMetria\\Settings"));

    // 메모리 상태는 초기화된 상태 유지
    AppError loadErr;
    BOOL loaded = mgr_->LoadFromRegistry(loadErr);

    EXPECT_EQ(loaded, TRUE)
        << "레지스트리 키 없음을 오류로 처리함";
    EXPECT_FALSE(loadErr.IsError())
        << "오류 코드: " << (LPCWSTR)loadErr.code;

    // 기본값 유지 확인 (SetUp에서 설정한 "Claude")
    EXPECT_EQ(mgr_->GetProvider(), CString(_T("Claude")));
}

// ============================================================
// TC-SETTINGS-006: 빈 API 키 저장 처리
// 빈 키를 저장해도 크래시 없이 성공하거나 graceful하게 처리한다.
// EncryptDPAPI("")는 빈 문자열을 반환하고 이를 그대로 저장한다.
// ============================================================
TEST_F(SettingsManagerTest, EmptyApiKey_SaveAndLoad_HandledGracefully) {
    mgr_->SetApiKey(_T("Claude"), _T(""));

    AppError saveErr;
    // 크래시 없이 완료되어야 한다
    EXPECT_NO_FATAL_FAILURE({
        BOOL saved = mgr_->SaveToRegistry(saveErr);
        // 빈 키 저장은 성공해야 한다 (EncryptDPAPI("")가 "" 반환 후 그대로 저장)
        EXPECT_EQ(saved, TRUE);
    });

    AppError loadErr;
    EXPECT_NO_FATAL_FAILURE({
        BOOL loaded = mgr_->LoadFromRegistry(loadErr);
        EXPECT_EQ(loaded, TRUE);
    });

    // 빈 키는 로드 후에도 빈 문자열이어야 한다
    EXPECT_TRUE(mgr_->GetApiKey(_T("Claude")).IsEmpty())
        << "빈 키가 로드 후 비어있지 않음";
}
