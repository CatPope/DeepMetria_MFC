// test_llm_retry.cpp — LLM 재시도 로직 TDD 테스트
// TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
//
// DetailedSpec §4.4: "CoT 재시도 1회"
// 예상 동작: Chat 첫 번째 호출 실패 시 동일 파라미터로 1회 재시도하고,
//            두 번째도 실패하면 오류를 반환한다.
//
// 이 테스트들은 ILLMRetry 인터페이스 또는 LLMRouter 에 retry 래퍼가
// 구현될 때까지 DISABLED_ 접두어로 비활성화된다.
// 구현 완료 후 DISABLED_ 를 제거하여 활성화한다.
//
// RED 단계: 이 테스트들은 현재 실패해야 정상이다.
//           (재시도 인터페이스가 없으므로 컴파일 오류 또는 동작 불일치)
//
// 재시도 인터페이스 설계 제안:
//   class ILLMRetry {
//   public:
//     virtual BOOL ChatWithRetry(const CString& systemPrompt,
//                                const CString& userMessage,
//                                CString&       outResponse,
//                                AppError&      outError,
//                                int            maxRetries = 1) = 0;
//   };

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <windows.h>
#include <atlstr.h>

#include "Common/Types.h"
#include "Infrastructure/LLM/LLMRouter.h"
#include "MockLLMProvider.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::InSequence;

// ============================================================
// LLM 재시도 로직 테스트 픽스처
// MockLLMProvider를 사용하여 실패/성공 시퀀스를 제어한다.
// TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
// ============================================================
class LLMRetryTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockProvider_ = std::make_unique<MockLLMProvider>();
    }

    std::unique_ptr<MockLLMProvider> mockProvider_;

    // 네트워크 오류 AppError 생성 헬퍼
    AppError MakeNetworkError() {
        return AppError(_T("HTTP_REQUEST_FAILED"), _T("연결 실패"), 2);
    }

    // API 오류 AppError 생성 헬퍼
    AppError MakeApiError() {
        return AppError(_T("API_ERROR"), _T("API 오류"), 2);
    }
};

// ============================================================
// TC-RETRY-001: 첫 번째 호출 실패 후 두 번째 호출이 성공하면
//               전체 결과는 성공(TRUE)이어야 한다
// TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
// ============================================================
TEST_F(LLMRetryTest, DISABLED_RetryOnFirstFailure_SecondSucceeds_ReturnsSuccess) {
    // TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
    const CString expectedResponse = _T("재시도 성공 응답");
    AppError networkError = MakeNetworkError();

    {
        InSequence seq;

        // 첫 번째 호출: 실패
        EXPECT_CALL(*mockProvider_, Chat(_, _, _, _, _))
            .WillOnce(DoAll(
                SetArgReferee<4>(networkError),
                Return(FALSE)));

        // 두 번째 호출 (재시도): 성공
        EXPECT_CALL(*mockProvider_, Chat(_, _, _, _, _))
            .WillOnce(DoAll(
                SetArgReferee<3>(expectedResponse),
                Return(TRUE)));
    }

    // TODO: ILLMRetry::ChatWithRetry 또는 LLMRouter::Chat(retry=true) 호출
    // 현재 LLMRouter::Chat 은 재시도를 수행하지 않으므로
    // 재시도 래퍼 구현 후 이 부분을 교체한다.
    //
    // 예시:
    //   CString  response;
    //   AppError error;
    //   BOOL result = retryWrapper_->ChatWithRetry(
    //       _T("system"), _T("user"), response, error, 1);
    //   EXPECT_EQ(result, TRUE);
    //   EXPECT_EQ(response, expectedResponse);
    //   EXPECT_FALSE(error.IsError());
    GTEST_SKIP() << "TDD: LLM 재시도 로직 미구현 — 구현 후 활성화";
}

// ============================================================
// TC-RETRY-002: 두 번의 호출 모두 실패하면 오류를 반환한다 (재시도 소진)
// TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
// ============================================================
TEST_F(LLMRetryTest, DISABLED_RetryExhausted_BothCallsFail_ReturnsError) {
    // TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
    AppError networkError = MakeNetworkError();

    {
        InSequence seq;

        // 첫 번째 호출: 실패
        EXPECT_CALL(*mockProvider_, Chat(_, _, _, _, _))
            .WillOnce(DoAll(
                SetArgReferee<4>(networkError),
                Return(FALSE)));

        // 두 번째 호출 (재시도): 실패
        EXPECT_CALL(*mockProvider_, Chat(_, _, _, _, _))
            .WillOnce(DoAll(
                SetArgReferee<4>(networkError),
                Return(FALSE)));
    }

    // TODO: 재시도 래퍼 구현 후 교체
    //   CString  response;
    //   AppError error;
    //   BOOL result = retryWrapper_->ChatWithRetry(
    //       _T("system"), _T("user"), response, error, 1);
    //   EXPECT_EQ(result, FALSE);
    //   EXPECT_TRUE(error.IsError());
    GTEST_SKIP() << "TDD: LLM 재시도 로직 미구현 — 구현 후 활성화";
}

// ============================================================
// TC-RETRY-003: 첫 번째 호출이 성공하면 두 번째 호출은 발생하지 않는다
// TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
// ============================================================
TEST_F(LLMRetryTest, DISABLED_NoRetryOnSuccess_FirstCallSucceeds_NoDuplicateCall) {
    // TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
    const CString expectedResponse = _T("첫 번째 호출 성공");

    // 정확히 1회만 호출되어야 한다
    EXPECT_CALL(*mockProvider_, Chat(_, _, _, _, _))
        .Times(1)
        .WillOnce(DoAll(
            SetArgReferee<3>(expectedResponse),
            Return(TRUE)));

    // TODO: 재시도 래퍼 구현 후 교체
    //   CString  response;
    //   AppError error;
    //   BOOL result = retryWrapper_->ChatWithRetry(
    //       _T("system"), _T("user"), response, error, 1);
    //   EXPECT_EQ(result, TRUE);
    //   EXPECT_EQ(response, expectedResponse);
    GTEST_SKIP() << "TDD: LLM 재시도 로직 미구현 — 구현 후 활성화";
}

// ============================================================
// TC-RETRY-004: 재시도 시 동일한 systemPrompt와 userMessage가 전달된다
// TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
// ============================================================
TEST_F(LLMRetryTest, DISABLED_RetryPreservesContext_SamePromptsOnRetry) {
    // TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
    const CString systemPrompt = _T("당신은 데이터 분석 전문가입니다.");
    const CString userMessage  = _T("이 데이터를 분석해주세요.");
    AppError networkError = MakeNetworkError();
    const CString successResponse = _T("분석 결과입니다.");

    {
        InSequence seq;

        // 첫 번째 호출: 동일 파라미터로 실패
        EXPECT_CALL(*mockProvider_,
            Chat(systemPrompt, userMessage, _, _, _))
            .WillOnce(DoAll(
                SetArgReferee<4>(networkError),
                Return(FALSE)));

        // 재시도: 동일 파라미터로 성공
        EXPECT_CALL(*mockProvider_,
            Chat(systemPrompt, userMessage, _, _, _))
            .WillOnce(DoAll(
                SetArgReferee<3>(successResponse),
                Return(TRUE)));
    }

    // TODO: 재시도 래퍼 구현 후 교체
    //   CString  response;
    //   AppError error;
    //   BOOL result = retryWrapper_->ChatWithRetry(
    //       systemPrompt, userMessage, response, error, 1);
    //   EXPECT_EQ(result, TRUE);
    GTEST_SKIP() << "TDD: LLM 재시도 로직 미구현 — 구현 후 활성화";
}

// ============================================================
// TC-RETRY-005: 네트워크 오류와 API 오류 모두 재시도 대상이다
// TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
// ============================================================
TEST_F(LLMRetryTest, DISABLED_RetryWithDifferentErrorTypes_NetworkAndApiErrorBothRetried) {
    // TDD: LLM 재시도 로직 미구현 — 구현 후 활성화
    const CString successResponse = _T("재시도 성공");

    // 케이스 A: 네트워크 오류 후 재시도 성공
    {
        MockLLMProvider networkMock;
        AppError networkError = MakeNetworkError();
        InSequence seq;

        EXPECT_CALL(networkMock, Chat(_, _, _, _, _))
            .WillOnce(DoAll(SetArgReferee<4>(networkError), Return(FALSE)));
        EXPECT_CALL(networkMock, Chat(_, _, _, _, _))
            .WillOnce(DoAll(SetArgReferee<3>(successResponse), Return(TRUE)));

        // TODO: 네트워크 오류 재시도 검증
    }

    // 케이스 B: API 오류 후 재시도 성공
    {
        MockLLMProvider apiMock;
        AppError apiError = MakeApiError();
        InSequence seq;

        EXPECT_CALL(apiMock, Chat(_, _, _, _, _))
            .WillOnce(DoAll(SetArgReferee<4>(apiError), Return(FALSE)));
        EXPECT_CALL(apiMock, Chat(_, _, _, _, _))
            .WillOnce(DoAll(SetArgReferee<3>(successResponse), Return(TRUE)));

        // TODO: API 오류 재시도 검증
    }

    GTEST_SKIP() << "TDD: LLM 재시도 로직 미구현 — 구현 후 활성화";
}

// ============================================================
// 재시도 로직 구현 가이드
// ============================================================
//
// 구현 방법 A — LLMRouter::Chat 에 재시도 내장:
//   BOOL LLMRouter::Chat(...) {
//       for (int attempt = 0; attempt <= 1; ++attempt) {
//           if (prov->Chat(...)) return TRUE;
//           if (attempt == 0) outError.Clear(); // 첫 번째 오류 초기화 후 재시도
//       }
//       return FALSE;
//   }
//
// 구현 방법 B — 별도 LLMRetryWrapper 클래스 도입:
//   class LLMRetryWrapper {
//   public:
//     LLMRetryWrapper(ILLMProvider* provider, int maxRetries = 1);
//     BOOL ChatWithRetry(const CString& sys, const CString& user,
//                        CString& outResp, AppError& outErr);
//   };
//
// 테스트 활성화 방법:
//   1. 위 중 하나를 구현한다.
//   2. 이 파일의 DISABLED_ 접두어를 모두 제거한다.
//   3. TODO 주석의 placeholder 코드를 실제 호출로 교체한다.
//   4. 빌드 후 테스트를 실행하여 GREEN 확인한다.
