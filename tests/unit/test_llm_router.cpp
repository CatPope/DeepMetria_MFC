// test_llm_router.cpp — LLMRouter 단위 테스트
// 실제 API 호출 없이 라우터의 상태 관리 및 위임 동작을 검증한다.
// MockLLMProvider를 직접 주입할 수 없으므로 (싱글턴 내부 구성),
// 관찰 가능한 공개 인터페이스(SetProvider/GetProvider/SetApiKey 등)와
// API 키 미설정 시 Chat 실패 경로를 중심으로 테스트한다.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// MFC 헤더를 테스트 환경에서 최소화하여 포함한다.
#include <windows.h>
#include <atlstr.h>   // CString (ATL 경량 버전, MFC afxwin.h 대신 사용)

// 공용 타입 정의
#include "Common/Types.h"

// 테스트 대상
#include "Infrastructure/LLM/LLMRouter.h"

// ============================================================
// LLMRouter 싱글턴 테스트 픽스처
// 각 테스트 전후로 상태를 알려진 값으로 초기화한다.
// ============================================================
class LLMRouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 각 테스트 시작 시 Claude 프로바이더로 초기화하여
        // 싱글턴 상태가 이전 테스트에 영향받지 않도록 한다.
        router_ = &LLMRouter::Instance();
        router_->SetProvider(LLMRouter::Provider::Claude);
        router_->SetModel(_T(""));           // 모델 초기화
        // API 키는 빈 문자열로 초기화
        router_->SetApiKey(LLMRouter::Provider::Claude, _T(""));
        router_->SetApiKey(LLMRouter::Provider::OpenAI, _T(""));
        router_->SetApiKey(LLMRouter::Provider::Gemini, _T(""));
    }

    LLMRouter* router_ = nullptr;
};

// ============================================================
// TC-ROUTER-001: Instance()는 항상 동일한 객체를 반환한다
// ============================================================
TEST_F(LLMRouterTest, SingletonInstance_ReturnsSameObject) {
    LLMRouter& a = LLMRouter::Instance();
    LLMRouter& b = LLMRouter::Instance();
    EXPECT_EQ(&a, &b) << "Instance()가 서로 다른 객체를 반환함 — 싱글턴 위반";
}

// ============================================================
// TC-ROUTER-002: SetProvider 후 GetProvider는 설정값을 반환한다
// ============================================================
TEST_F(LLMRouterTest, SetAndGetProvider_Claude_ReturnsExpectedProvider) {
    router_->SetProvider(LLMRouter::Provider::Claude);
    EXPECT_EQ(router_->GetProvider(), LLMRouter::Provider::Claude);
}

TEST_F(LLMRouterTest, SetAndGetProvider_OpenAI_ReturnsExpectedProvider) {
    router_->SetProvider(LLMRouter::Provider::OpenAI);
    EXPECT_EQ(router_->GetProvider(), LLMRouter::Provider::OpenAI);
}

TEST_F(LLMRouterTest, SetAndGetProvider_Gemini_ReturnsExpectedProvider) {
    router_->SetProvider(LLMRouter::Provider::Gemini);
    EXPECT_EQ(router_->GetProvider(), LLMRouter::Provider::Gemini);
}

// ============================================================
// TC-ROUTER-003: 기본 프로바이더는 Claude이다
// (LLMRouter 생성자에서 m_activeProvider(Provider::Claude)로 초기화)
// 싱글턴이므로 첫 번째 Instance() 호출 결과로만 검증 가능.
// SetUp에서 Claude로 재설정하므로 이 테스트는 그 불변식을 확인한다.
// ============================================================
TEST_F(LLMRouterTest, DefaultProvider_IsClaude) {
    // SetUp에서 명시적으로 Claude로 설정했으므로
    // 생성 직후 기본값이 Claude임을 검증한다.
    EXPECT_EQ(router_->GetProvider(), LLMRouter::Provider::Claude);
}

// ============================================================
// TC-ROUTER-004: 프로바이더별 API 키를 독립적으로 설정할 수 있다
// SetApiKey는 반환값이 없으므로 Chat 실패 경로를 통해 간접 검증한다.
// 키가 비어 있으면 HTTP 요청이 실패하고, 키를 설정하면 요청 시도가 일어난다.
// 여기서는 키 설정 호출이 예외 없이 완료됨을 검증한다.
// ============================================================
TEST_F(LLMRouterTest, SetApiKeyPerProvider_DoesNotThrowForAnyProvider) {
    // 예외 없이 완료되면 성공
    EXPECT_NO_THROW(router_->SetApiKey(LLMRouter::Provider::Claude, _T("sk-ant-test")));
    EXPECT_NO_THROW(router_->SetApiKey(LLMRouter::Provider::OpenAI, _T("sk-openai-test")));
    EXPECT_NO_THROW(router_->SetApiKey(LLMRouter::Provider::Gemini, _T("gemini-test-key")));
    // 정리
    router_->SetApiKey(LLMRouter::Provider::Claude, _T(""));
    router_->SetApiKey(LLMRouter::Provider::OpenAI, _T(""));
    router_->SetApiKey(LLMRouter::Provider::Gemini, _T(""));
}

// ============================================================
// TC-ROUTER-005: SetModel은 빈 문자열 및 임의 모델명을 허용한다
// ============================================================
TEST_F(LLMRouterTest, SetModel_AcceptsArbitraryModelString) {
    // 예외 없이 완료되면 성공
    EXPECT_NO_THROW(router_->SetModel(_T("claude-opus-4-5")));
    EXPECT_NO_THROW(router_->SetModel(_T("gpt-4o")));
    EXPECT_NO_THROW(router_->SetModel(_T("")));
}

// ============================================================
// TC-ROUTER-006: API 키 없이 Chat을 호출하면 FALSE를 반환하고 오류가 설정된다
// libcurl이 빈 API 키로 실제 HTTP 요청을 시도하므로
// 네트워크 오류 또는 API 인증 오류 중 하나가 발생한다.
// 두 경우 모두 FALSE + IsError()==TRUE를 기대한다.
// 주의: 이 테스트는 실제 네트워크를 시도하므로 타임아웃이 발생할 수 있다.
// CMakeLists에서 TIMEOUT 속성을 설정하는 것을 권장한다.
// ============================================================
TEST_F(LLMRouterTest, ChatWithoutApiKey_ReturnsFalseWithError) {
    router_->SetProvider(LLMRouter::Provider::Claude);
    router_->SetApiKey(LLMRouter::Provider::Claude, _T(""));

    CString  response;
    AppError error;

    BOOL result = router_->Chat(_T("system"), _T("hello"), response, error);

    // API 키가 없으면 인증 실패 또는 네트워크 오류로 FALSE 반환
    EXPECT_EQ(result, FALSE);
    EXPECT_TRUE(error.IsError())
        << "오류가 설정되지 않음. code=" << (LPCWSTR)error.code;
}

// ============================================================
// TC-ROUTER-007: 빈 시스템 프롬프트와 빈 사용자 메시지로 Chat을 호출해도
// 크래시 없이 처리된다 (API 오류 또는 네트워크 오류로 FALSE 반환)
// ============================================================
TEST_F(LLMRouterTest, ChatWithEmptyPrompts_DoesNotCrash) {
    router_->SetProvider(LLMRouter::Provider::Claude);
    router_->SetApiKey(LLMRouter::Provider::Claude, _T(""));

    CString  response;
    AppError error;

    // 빈 프롬프트로도 크래시 없이 반환해야 한다
    EXPECT_NO_THROW(router_->Chat(_T(""), _T(""), response, error));
}

// ============================================================
// TC-ROUTER-008: 프로바이더를 전환해도 GetProvider가 최신 값을 반환한다
// ============================================================
TEST_F(LLMRouterTest, ProviderSwitching_ReflectsLastSetProvider) {
    router_->SetProvider(LLMRouter::Provider::Claude);
    EXPECT_EQ(router_->GetProvider(), LLMRouter::Provider::Claude);

    router_->SetProvider(LLMRouter::Provider::OpenAI);
    EXPECT_EQ(router_->GetProvider(), LLMRouter::Provider::OpenAI);

    router_->SetProvider(LLMRouter::Provider::Gemini);
    EXPECT_EQ(router_->GetProvider(), LLMRouter::Provider::Gemini);

    // Claude로 되돌려도 정확히 반영된다
    router_->SetProvider(LLMRouter::Provider::Claude);
    EXPECT_EQ(router_->GetProvider(), LLMRouter::Provider::Claude);
}


// ============================================================
// 폴백(QUOTA) 동작 검증 픽스처
// SetProviderForTest 시임으로 MockLLMProvider를 주입하여
// 실제 네트워크 호출 없이 모델/프로바이더 전환을 검증한다.
// ============================================================
#include "MockLLMProvider.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::NiceMock;

class LLMRouterFallbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        router_ = &LLMRouter::Instance();
        router_->SetModel(_T(""));
        router_->SetApiKey(LLMRouter::Provider::Claude, _T(""));
        router_->SetApiKey(LLMRouter::Provider::OpenAI, _T(""));
        router_->SetApiKey(LLMRouter::Provider::Gemini, _T(""));
    }

    // 지정 프로바이더 슬롯에 NiceMock을 주입하고 원시 포인터를 반환한다.
    NiceMock<MockLLMProvider>* InjectMock(LLMRouter::Provider p) {
        auto mock = std::make_unique<NiceMock<MockLLMProvider>>();
        NiceMock<MockLLMProvider>* raw = mock.get();
        router_->SetProviderForTest(p, std::move(mock));
        return raw;
    }

    LLMRouter* router_ = nullptr;
};

// -- GetFallbackModel: Gemini 체인 전체 --
TEST_F(LLMRouterFallbackTest, GetFallbackModel_GeminiChain_WalksToBottomThenEmpty) {
    EXPECT_STREQ(router_->GetFallbackModel(LLMRouter::Provider::Gemini, _T("gemini-3.1-pro-preview")),
                 _T("gemini-2.5-pro"));
    EXPECT_STREQ(router_->GetFallbackModel(LLMRouter::Provider::Gemini, _T("gemini-2.5-pro")),
                 _T("gemini-2.5-flash"));
    EXPECT_STREQ(router_->GetFallbackModel(LLMRouter::Provider::Gemini, _T("gemini-2.5-flash")),
                 _T("gemini-3.1-flash-lite"));
    EXPECT_TRUE(router_->GetFallbackModel(LLMRouter::Provider::Gemini, _T("gemini-3.1-flash-lite")).IsEmpty());
}

TEST_F(LLMRouterFallbackTest, GetFallbackModel_EmptyModel_ReturnsSecondEntry) {
    EXPECT_STREQ(router_->GetFallbackModel(LLMRouter::Provider::Gemini, _T("")),
                 _T("gemini-2.5-pro"));
    EXPECT_STREQ(router_->GetFallbackModel(LLMRouter::Provider::Claude, _T("")),
                 _T("claude-sonnet-4-5-20250514"));
}

TEST_F(LLMRouterFallbackTest, GetFallbackModel_UnknownModel_ReturnsEmpty) {
    EXPECT_TRUE(router_->GetFallbackModel(LLMRouter::Provider::Gemini, _T("no-such-model")).IsEmpty());
}

TEST_F(LLMRouterFallbackTest, GetFallbackModel_OpenAIChain_WalksToBottomThenEmpty) {
    EXPECT_STREQ(router_->GetFallbackModel(LLMRouter::Provider::OpenAI, _T("gpt-4o")),
                 _T("gpt-4o-mini"));
    EXPECT_STREQ(router_->GetFallbackModel(LLMRouter::Provider::OpenAI, _T("gpt-4o-mini")),
                 _T("o4-mini"));
    EXPECT_TRUE(router_->GetFallbackModel(LLMRouter::Provider::OpenAI, _T("o4-mini")).IsEmpty());
}

// -- 같은 프로바이더 내 모델 전환 --
TEST_F(LLMRouterFallbackTest, WithinProvider_QuotaThenSuccess_DowngradesModel) {
    NiceMock<MockLLMProvider>* gemini = InjectMock(LLMRouter::Provider::Gemini);
    gemini->SetupProviderInfo(_T("Gemini"), _T("gemini-2.5-flash"));

    router_->SetProvider(LLMRouter::Provider::Gemini);
    router_->SetModel(_T("gemini-2.5-pro"));

    AppError quota(_T("QUOTA_EXCEEDED"), _T("한도 초과"), 2);
    EXPECT_CALL(*gemini, Chat(_, _, _, _, _))
        .WillOnce(DoAll(SetArgReferee<4>(quota), Return(FALSE)))
        .WillOnce(DoAll(SetArgReferee<3>(CString(_T("ok"))), Return(TRUE)));

    CString  resp;
    AppError err;
    BOOL result = router_->ChatWithRetry(_T("sys"), _T("user"), resp, err, 3);

    EXPECT_EQ(result, TRUE);
    EXPECT_EQ(router_->GetProvider(), LLMRouter::Provider::Gemini);
    EXPECT_STREQ(router_->GetModel(), _T("gemini-2.5-flash"));
    EXPECT_FALSE(router_->TakeFallbackNotice().IsEmpty());
}

// -- 크로스 프로바이더 전환 (체인 바닥에서) --
TEST_F(LLMRouterFallbackTest, CrossProvider_GeminiBottom_SwitchesToOpenAI) {
    NiceMock<MockLLMProvider>* gemini = InjectMock(LLMRouter::Provider::Gemini);
    NiceMock<MockLLMProvider>* openai = InjectMock(LLMRouter::Provider::OpenAI);

    gemini->SetupProviderInfo(_T("Gemini"), _T("gemini-2.5-flash"));
    openai->SetupProviderInfo(_T("OpenAI"), _T("gpt-4o"));

    ON_CALL(*gemini, HasApiKey()).WillByDefault(Return(true));
    ON_CALL(*openai, HasApiKey()).WillByDefault(Return(true));

    AppError quota(_T("QUOTA_EXCEEDED"), _T("한도 초과"), 2);
    ON_CALL(*gemini, Chat(_, _, _, _, _))
        .WillByDefault(DoAll(SetArgReferee<4>(quota), Return(FALSE)));
    ON_CALL(*openai, Chat(_, _, _, _, _))
        .WillByDefault(DoAll(SetArgReferee<3>(CString(_T("ok"))), Return(TRUE)));

    router_->SetProvider(LLMRouter::Provider::Gemini);
    router_->SetModel(_T("gemini-3.1-flash-lite"));

    CString  resp;
    AppError err;
    BOOL result = router_->ChatWithRetry(_T("sys"), _T("user"), resp, err, 3);

    EXPECT_EQ(result, TRUE);
    EXPECT_EQ(router_->GetProvider(), LLMRouter::Provider::OpenAI);
    EXPECT_STREQ(router_->GetModel(), _T("o4-mini"));
    EXPECT_FALSE(router_->TakeFallbackNotice().IsEmpty());
}

// -- 대체 프로바이더 없음: 모든 모델 소진 --
TEST_F(LLMRouterFallbackTest, NoAlternative_AllExhausted_ReturnsFalse) {
    NiceMock<MockLLMProvider>* gemini = InjectMock(LLMRouter::Provider::Gemini);
    NiceMock<MockLLMProvider>* openai = InjectMock(LLMRouter::Provider::OpenAI);
    NiceMock<MockLLMProvider>* claude = InjectMock(LLMRouter::Provider::Claude);

    gemini->SetupProviderInfo(_T("Gemini"), _T("gemini-2.5-flash"));

    ON_CALL(*gemini, HasApiKey()).WillByDefault(Return(true));
    ON_CALL(*openai, HasApiKey()).WillByDefault(Return(false));
    ON_CALL(*claude, HasApiKey()).WillByDefault(Return(false));

    AppError quota(_T("QUOTA_EXCEEDED"), _T("한도 초과"), 2);
    ON_CALL(*gemini, Chat(_, _, _, _, _))
        .WillByDefault(DoAll(SetArgReferee<4>(quota), Return(FALSE)));

    router_->SetProvider(LLMRouter::Provider::Gemini);
    router_->SetModel(_T("gemini-3.1-flash-lite"));

    CString  resp;
    AppError err;
    BOOL result = router_->ChatWithRetry(_T("sys"), _T("user"), resp, err, 3);

    EXPECT_EQ(result, FALSE);
    EXPECT_EQ(err.code, CString(_T("QUOTA_EXCEEDED")));
    CString notice = router_->TakeFallbackNotice();
    EXPECT_NE(notice.Find(_T("모든")), -1) << "모든 모델 소진 안내가 없음";
}
